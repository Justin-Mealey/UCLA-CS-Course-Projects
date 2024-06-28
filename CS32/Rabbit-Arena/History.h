#ifndef HISTORY_H
#define HISTORY_H

#include "globals.h"

class History {
	int m_grid[MAXROWS][MAXCOLS];
	const int m_rows;
	const int m_cols;
public:
	History(int nRows, int nCols);
	bool record(int r, int c);
	void display() const;
};


#endif
