#include "History.h"
#include "globals.h"
#include <iostream>

using namespace std;

void clearScreen();

History::History(int nRows, int nCols) : m_rows(nRows), m_cols(nCols){
	for (int k = 1; k <= nRows; k++) {
		for (int i = 1; i <= nCols; i++) {
			m_grid[k-1][i-1] = 0;
		}
	}
}

bool History::record(int r, int c) {
	if (r > m_rows || c > m_cols || r < 1 || c < 1)
		return false;
	m_grid[r-1][c-1]++;
	return true;
}

void History::display() const {
	clearScreen();
	char displaygrid[MAXROWS][MAXCOLS];
	for (int i = 1; i <= m_rows; i++) {
		for (int k = 1; k <= m_cols; k++) {
			switch (m_grid[i-1][k-1]) {
				case 0:
					displaygrid[i-1][k-1] = '.'; break;
				case 1:
					displaygrid[i-1][k-1] = 'A'; break;
				case 2:
					displaygrid[i-1][k-1] = 'B'; break;
				case 3:
					displaygrid[i-1][k-1] = 'C'; break;
				case 4:
					displaygrid[i-1][k-1] = 'D'; break;
				case 5:
					displaygrid[i-1][k-1] = 'E'; break;
				case 6:
					displaygrid[i-1][k-1] = 'F'; break;
				case 7:
					displaygrid[i-1][k-1] = 'G'; break;
				case 8:
					displaygrid[i-1][k-1] = 'H'; break;
				case 9:
					displaygrid[i-1][k-1] = 'I'; break;
				case 10:
					displaygrid[i-1][k-1] = 'J'; break;
				case 11:
					displaygrid[i-1][k-1] = 'K'; break;
				case 12:
					displaygrid[i-1][k-1] = 'L'; break;
				case 13:
					displaygrid[i-1][k-1] = 'M'; break;
				case 14:
					displaygrid[i-1][k-1] = 'N'; break;
				case 15:
					displaygrid[i-1][k-1] = 'O'; break;
				case 16:
					displaygrid[i-1][k-1] = 'P'; break;
				case 17:
					displaygrid[i-1][k-1] = 'Q'; break;
				case 18:
					displaygrid[i-1][k-1] = 'R'; break;
				case 19:
					displaygrid[i-1][k-1] = 'S'; break;
				case 20:
					displaygrid[i-1][k-1] = 'T'; break;
				case 21:
					displaygrid[i-1][k-1] = 'U'; break;
				case 22:
					displaygrid[i-1][k-1] = 'V'; break;
				case 23:
					displaygrid[i-1][k-1] = 'W'; break;
				case 24:
					displaygrid[i-1][k-1] = 'X'; break;
				case 25:
					displaygrid[i-1][k-1] = 'Y'; break;
				default: 
					displaygrid[i-1][k-1] = 'Z'; break;
				
			}
		}
	} 
	for (int r = 1; r <= m_rows; r++)
	{
		for (int c = 1; c <= m_cols; c++) {
			cout << displaygrid[r-1][c-1];
		}
		cout << endl;
	}
	cout << endl;
}