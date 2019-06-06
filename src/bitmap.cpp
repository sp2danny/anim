
//#include "stdafx.h"

#include <cassert>
#include <cstring>
#include <iostream>

#include "bitmap.hpp"

using namespace alib;

#pragma pack(push, 1)

typedef struct {
	char               bfType[2];
	unsigned long      bfSize;
	unsigned short     bfReserved1;
	unsigned short     bfReserved2;
	unsigned long      bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
	unsigned long      biSize;
	signed long        biWidth;
	signed long        biHeight;
	unsigned short     biPlanes;
	unsigned short     biBitCount;
	unsigned long      biCompression;
	unsigned long      biSizeImage;
	signed long        biXPelsPerMeter;
	signed long        biYPelsPerMeter;
	unsigned long      biClrUsed;
	unsigned long      biClrImportant;
} BITMAPINFOHEADER;

#pragma pack(pop)

RGB& alib::RGB_Image::operator()(int x, int y)
{
	assert(x >= 0 && x < int(w));
	assert(y >= 0 && y < int(h));
	return pix[ x + w*y ];
}

void alib::LoadBMP(RGB_Image& img, std::istream& in)
{
	auto fhsz = sizeof(BITMAPFILEHEADER);
	auto ihsz = sizeof(BITMAPINFOHEADER);

	BITMAPFILEHEADER fh;
	in.read((char*)&fh, fhsz);

	assert(fh.bfType[0] == 'B');
	assert(fh.bfType[1] == 'M');

	BITMAPINFOHEADER ih;
	in.read((char*)&ih, ihsz);

	int ofs = int(fh.bfOffBits) - int(fhsz+ihsz);
	if (ofs)
		in.seekg(ofs, in.cur);

	auto w = img.w = ih.biWidth;
	auto h = img.h = ih.biHeight;
	img.pix.resize(w*h);

	assert(ih.biSize == ihsz);
	assert(ih.biBitCount == 24);
	assert(ih.biPlanes == 1);
	assert(ih.biCompression == 0);

	auto stride = w*3;
	while (stride%4) ++stride;
	auto padding = stride - (w * 3);

	for (auto y = 0ul; y < h; ++y)
	{
		for (auto x = 0ul; x < w; ++x)
		{
			RGB rgb;
			if (!in.good())
			{
				std::cerr << std::boolalpha << "bad  " << in.bad()  << std::endl;
				std::cerr << std::boolalpha << "fail " << in.fail() << std::endl;
				std::cerr << std::boolalpha << "eof  " << in.eof()  << std::endl;
			}
			in.read( (char*)&rgb, 3 );
			assert( in && (in.gcount()==3) );
			img.pix[x+y*w] = rgb;
		}
		if (padding)
			in.seekg(padding, in.cur);
	}
}

void alib::SaveBMP(const RGB_Image& img, std::ostream& out)
{

	BITMAPFILEHEADER fh;
	BITMAPINFOHEADER ih;

	fh.bfSize = sizeof(fh) + sizeof(ih);
	fh.bfType[0] = 'B';
	fh.bfType[1] = 'M';
	fh.bfReserved1 = fh.bfReserved2 = 0;
	fh.bfOffBits = sizeof(fh) + sizeof(ih);

	memset( &ih, 0, sizeof(ih) );

	unsigned long w = ih.biWidth  = img.w;
	unsigned long h = ih.biHeight = img.h;

	ih.biSize = sizeof(ih);
	ih.biBitCount = 24;
	ih.biPlanes = 1;
	ih.biCompression = 0;
	
	ih.biXPelsPerMeter = 72;
	ih.biYPelsPerMeter = 72;

	auto stride = w * 3;
	while (stride % 4) ++stride;

	auto padding = stride - (w * 3);

	char buf[] = {0,0,0,0};

	fh.bfSize += stride*h;

	out.write((char*)&fh, sizeof(fh));
	out.write((char*)&ih, sizeof(ih));

	for (auto y = 0ul; y < h; ++y)
	{
		for (auto x = 0ul; x < w; ++x)
		{
			RGB rgb = img.pix[x + y*w];
			out.write((char*)&rgb, 3);
		}
		if (padding)
			out.write(buf, padding);
	}
}

void FadeToBlack(RGB_Image& img, float amount)
{
	if (amount <= 0.0f) return;
	if (amount >= 1.0f)
	{
		for (auto& pix : img.pix)
			pix = RGB{0,0,0};
		return;
	}

	for (auto& pix : img.pix)
	{
		HSV hsv = RGB_2_HSV(pix);
		float v = hsv.v;
		v = v * (1.0f-amount);
		if (v>255) v = v=255;
		hsv.v = (UC)v;
		pix = HSV_2_RGB(hsv);
	}
}

HSV alib::RGB_2_HSV(const RGB& rgb)
{
	HSV hsv;
	unsigned char rgbMin, rgbMax;

	rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
	rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);

	hsv.v = rgbMax;
	if (hsv.v == 0)
	{
		hsv.h = 0;
		hsv.s = 0;
		return hsv;
	}

	hsv.s = 255 * long(rgbMax - rgbMin) / hsv.v;
	if (hsv.s == 0)
	{
		hsv.h = 0;
		return hsv;
	}

	if (rgbMax == rgb.r)
		hsv.h = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
	else if (rgbMax == rgb.g)
		hsv.h = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
	else
		hsv.h = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);

	return hsv;
}

RGB alib::HSV_2_RGB(const HSV& hsv)
{
	RGB rgb;
	unsigned char region, remainder, p, q, t;

	if (hsv.s == 0)
	{
		rgb.r = hsv.v;
		rgb.g = hsv.v;
		rgb.b = hsv.v;
		return rgb;
	}

	region = hsv.h / 43;
	remainder = (hsv.h - (region * 43)) * 6;

	p = (hsv.v * (255 - hsv.s)) >> 8;
	q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
	t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

	switch (region)
	{
	case 0:
		rgb.r = hsv.v; rgb.g = t; rgb.b = p;
		break;
	case 1:
		rgb.r = q; rgb.g = hsv.v; rgb.b = p;
		break;
	case 2:
		rgb.r = p; rgb.g = hsv.v; rgb.b = t;
		break;
	case 3:
		rgb.r = p; rgb.g = q; rgb.b = hsv.v;
		break;
	case 4:
		rgb.r = t; rgb.g = p; rgb.b = hsv.v;
		break;
	default:
		rgb.r = hsv.v; rgb.g = p; rgb.b = q;
		break;
	}

	return rgb;
}

