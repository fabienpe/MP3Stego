#ifndef FFT_H
#define FFT_H

#include "types.h"

void fft(float x_real[BLKSIZE],
         float x_imag[BLKSIZE],
         float energy[BLKSIZE],
         float phi[BLKSIZE],
         int N);


#endif
