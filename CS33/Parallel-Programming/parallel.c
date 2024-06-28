/* 
 *  Name: Justin Mealey
 *  UID: 005961729
 */

#include <stdlib.h>
#include <omp.h>

#include "utils.h"
#include "parallel.h"



/*
 *  PHASE 1: compute the mean pixel value
 *  This code is buggy! Find the bug and speed it up.
 */
void mean_pixel_parallel(const uint8_t img[][NUM_CHANNELS], int num_rows, int num_cols, double mean[NUM_CHANNELS]) {
    int i, num_pixels;
    double sum0, sum1, sum2;

    num_pixels = num_rows * num_cols;
    sum0 = sum1 = sum2 = 0;

    #pragma omp parallel for num_threads(8) reduction(+:sum0) reduction(+:sum1) reduction(+:sum2)
    for (i = 0; i < num_pixels; i++){//i private
        sum0 += img[i][0];
        sum1 += img[i][1];
        sum2 += img[i][2];
    }
    //mean[ch] = sum/num_pixels;
    mean[0] = sum0/num_pixels;
    mean[1] = sum1/num_pixels;
    mean[2] = sum2/num_pixels;
}



/*
 *  PHASE 2: convert image to grayscale and record the max grayscale value along with the number of times it appears
 *  This code is NOT buggy, just sequential. Speed it up.
 */
void grayscale_parallel(const uint8_t img[][NUM_CHANNELS], int num_rows, int num_cols, uint32_t grayscale_img[][NUM_CHANNELS], uint8_t *max_gray, uint32_t *max_count) {

    int i, num_pixels, ch, gray_ch;
    uint32_t newPixelVal;

    num_pixels = num_rows * num_cols;

    #pragma omp parallel for private(newPixelVal, ch, gray_ch)
    for(i = 0; i < num_pixels; i++){
        newPixelVal = 0;
        for (ch = 0; ch < NUM_CHANNELS; ch++) {
            newPixelVal += img[i][ch];
        }
        newPixelVal /= NUM_CHANNELS;
        for (gray_ch = 0; gray_ch < NUM_CHANNELS; gray_ch++){
            grayscale_img[i][gray_ch] = newPixelVal;
        }
        if (newPixelVal >= *max_gray) {
            #pragma omp critical
            {
            if(newPixelVal == *max_gray)
                (*max_count)+=3;
            else if(newPixelVal > *max_gray){
                *max_count = 3;
                *max_gray = newPixelVal;
            }
            }
        }
    }
}


/*
 *  PHASE 3: perform convolution on image
 *  This code is NOT buggy, just sequential. Speed it up.
 */
void convolution_parallel(const uint8_t padded_img[][NUM_CHANNELS], int num_rows, int num_cols, const uint32_t kernel[], int kernel_size, uint32_t convolved_img[][NUM_CHANNELS]) {
    
    int row, col, ch, kernel_row, kernel_col, kernel_norm, i, conv_rows, conv_cols, kSizeSquared;
    double KnormRecip;

    // compute kernel normalization factor
    kernel_norm = 0;
    kSizeSquared = kernel_size*kernel_size;
    #pragma omp parallel for reduction(+:kernel_norm)
    for(i = 0; i < kSizeSquared; i++) {
        kernel_norm += kernel[i];
    }

    // compute dimensions of convolved image
    conv_rows = num_rows - kernel_size + 1;
    conv_cols = num_cols - kernel_size + 1;

    int temp;
    uint32_t sum;

    KnormRecip = 1.0/kernel_norm;
    // perform convolution
    omp_set_num_threads(omp_get_max_threads());
    #pragma omp parallel for collapse(2) private(col, sum, temp, kernel_col, kernel_row, ch) shared(padded_img, kernel, convolved_img)    
    for (row = 0; row < conv_rows; row++) {
        for (col = 0; col < conv_cols; col++) {
            temp = row*conv_cols + col;
            for(ch = 0; ch < NUM_CHANNELS; ch++){   
                convolved_img[temp][ch] = 0;
                sum = 0;
                for (kernel_row = 0; kernel_row < kernel_size; kernel_row++) {
                    int term1 = (row+kernel_row)*num_cols + col;
                    int term2 = kernel_row * kernel_size;
                    for (kernel_col = 0; kernel_col < kernel_size; kernel_col++) {
                        sum += padded_img[term1 + kernel_col][ch] * kernel[term2 + kernel_col];
                    }
                }
                convolved_img[temp][ch] = sum * KnormRecip;
            }
        }    
    }
}