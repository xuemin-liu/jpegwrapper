// Tests.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "../jpegwrapper/j2k_mem_advance.h"
#include "../bin/dependent_sources/image_matrix_types.h"

#include <iostream>
#include <thread>
#include <fstream>
#include <iostream>
#include <time.h>
#include <random>
#include <algorithm>
#include <map>
#include <algorithm>

using namespace std;

bool openJpeg2kCompress(unsigned char* pInputBuffer, int col, int row, int bits, vector<uint8_t>& pOutputBuffer, int* pOutputLength, bool bLossless, bool bSigned, float rate)
{
	try
	{
		auto bitcount = (bits + 7) / 8 * 8;

		fs_image_matrix mat;

		if (bitcount != 24)
		{
			mat.channels = 1;
			mat.prec = bits;
			mat._bpp = bits;
			mat.color_space = FSC_GRAYSCALE;
		}
		else
		{
			mat.channels = 3;
			mat.prec = 8;
			mat._bpp = 24;
			mat.color_space = FSC_RGB;
		}

		mat.width = col;
		mat.height = row;
		mat.sgnd = bSigned ? 1 : 0;
		mat.align = 0;
		mat.shared = 1; //do not release buffer
		mat.pixels = pInputBuffer;
		auto output = save_j2k_mem(mat, rate * 100);

		output.seek(0);
		*pOutputLength = output.stream_length();
		auto temp  = output.as_vector();
		pOutputBuffer.swap(temp);
		//std::memcpy(pOutputBuffer, output.stream_data(), *pOutputLength);
		return true;
	}
	catch (std::exception& e) {
		return false;;
	}
}

bool openJpeg2kDecompress(unsigned char* pInputBuffer, int inputLength, unsigned char* pOutputBuffer, int& row, int& col)
{
	try
	{
		fs_image_matrix mat = jwp_load_j2k_mem(pInputBuffer, inputLength, FS_JPEG2K_CODEC_JP2);
		col = mat.width;
		row = mat.height;
		int bytes = (mat.prec + 7) / 8;
		std::memcpy(pOutputBuffer, mat.pixels, mat.width * mat.height * mat.channels * bytes);
		return true;
	}
	catch (std::exception const& ex)
	{
		return false;
	}
}

int main()
{
    vector<uint8_t> input(512 * 512 * 3, 0);

    random_device rnd_device;
    mt19937 mersenne_engine(rnd_device());
    uniform_int_distribution<int> dist(0, 4095);
    generate(input.begin(), input.end(), [&dist, &mersenne_engine]() {
        return dist(mersenne_engine); });

	vector<uint8_t> compressed;// (512 * 512 * 3 + 1024, 0);
	vector<uint8_t> decompressed(512 * 512 * 3 + 1024 * 1024, 0);
	int outputlen = 0;
    int row = 512;
    int col = 512;

	openJpeg2kCompress(input.data(), col, row, 24, compressed, &outputlen, true, false, 1);
	openJpeg2kDecompress(compressed.data(), outputlen, decompressed.data(), row, col);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
