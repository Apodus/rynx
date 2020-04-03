#pragma once

// fft
#include <complex>
#include <valarray>

namespace rynx
{
	void fft(std::valarray<std::complex<float>>& x); // in-place fast fourier transform
	void ifft(std::valarray<std::complex<float>>& x); // in-place inverse fast fourier transform
}