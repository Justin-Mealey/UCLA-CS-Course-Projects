#pragma once

#include <stdint.h>
#include <unistd.h>

// Initialize IO layer
void init_io();

// IF RETURN VAL > 0, THAT # OF BYTES IS IN BUF
ssize_t input_io(uint8_t* buf, size_t max_length);

// RETURN 0 ON SUCCESSFUL, FULL WRITE, -1 ON ANYTHING ELSE 
void output_io(uint8_t* buf, size_t length);
