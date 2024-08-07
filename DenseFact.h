#ifndef DENSE_FACT_H
#define DENSE_FACT_H

#include <time.h>
#include "Blas_declaration.h"

double GetTime() {
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  return now.tv_sec + now.tv_nsec * 1e-9;
}

#define max(i, j) ((i) >= (j) ? (i) : (j))
#define min(i, j) ((i) >= (j) ? (j) : (i))

// variables for BLAS calls
const double d_one = 1.0;
const double d_zero = 0.0;
const double d_m_one = -1.0;
const int i_one = 1;
const char LL = 'L';
const char NN = 'N';
const char RR = 'R';
const char TT = 'T';
const char UU = 'U';

#endif