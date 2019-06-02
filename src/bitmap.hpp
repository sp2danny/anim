
#pragma once

#include "alib.hpp"

namespace alib
{

struct RGB_Image
{
	unsigned long w, h;
	std::vector<RGB> pix;
	RGB& operator()(int,int);
};

void LoadBMP(RGB_Image& img, std::istream& in);

void SaveBMP(const RGB_Image& img, std::ostream& out);

void FadeToBlack(RGB_Image& img, float amount);

void Resize(const RGB_Image& src, RGB_Image& dst, UL w, UL h);
void Resize(const RGB_Image& src, RGB_Image& dst, float scale);

}

