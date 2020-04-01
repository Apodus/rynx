#pragma once

// fft
#include <complex>
#include <iostream>
#include <valarray>
#include <cmath>

constexpr float PI = 3.141592653589793238460f;

typedef std::complex<float> Complex;
typedef std::valarray<Complex> CArray;

// implementation stolen from (https://rosettacode.org/wiki/Fast_Fourier_transform)
void fft(CArray& x) {
	uint32_t N = static_cast<uint32_t>(x.size());
	uint32_t k = N;
	uint32_t n;
	float thetaT = 3.14159265358979323846264338328f / N;
	Complex phiT(std::cos(thetaT), -std::sin(thetaT));
	Complex T;
	
	while (k > 1) {
		n = k;
		k >>= 1;
		phiT = phiT * phiT;
		T = 1.0L;
		for (unsigned int l = 0; l < k; l++) {
			for (unsigned int a = l; a < N; a += n) {
				unsigned int b = a + k;
				Complex t = x[a] - x[b];
				x[a] += x[b];
				x[b] = t * T;
			}
			T *= phiT;
		}
	}

	// Decimate
	uint32_t m = uint32_t(std::log2(N));
	for (uint32_t a = 0; a < N; a++) {
		uint32_t b = a;
		// Reverse bits
		b = (((b & 0xaaaaaaaa) >> 1) | ((b & 0x55555555) << 1));
		b = (((b & 0xcccccccc) >> 2) | ((b & 0x33333333) << 2));
		b = (((b & 0xf0f0f0f0) >> 4) | ((b & 0x0f0f0f0f) << 4));
		b = (((b & 0xff00ff00) >> 8) | ((b & 0x00ff00ff) << 8));
		b = ((b >> 16) | (b << 16)) >> (32 - m);
		if (b > a) {
			std::swap(x[a], x[b]);
		}
	}
}

// inverse fft (in-place)
void ifft(CArray& x) {
	// conjugate the complex numbers
	x = x.apply(std::conj);

	// forward fft
	fft(x);

	// conjugate the complex numbers again
	x = x.apply(std::conj);

	// scale the numbers
	x /= float(x.size());
}
