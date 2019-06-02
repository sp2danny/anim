
#include <cassert>
#include <iostream>
#include <fstream>
#include <type_traits>

// --------------------------------------------

using namespace std;


#include "alib.hpp"
#include "bitmap.hpp"

using namespace alib;

typedef unsigned short US;

struct pcxHeader
{
	UC manuf,verinf,encoding,bpp;
	US left_marg,upper_marg,right_marg,lower_marg;
	US hor_dpi,ver_dpi;
	UC pal16[48];
	UC reserved,NCP,NBS;
	US palinfo,hor_scr_sz,ver_scr_sz;
	UC reserver2[54];
};

typedef UC* UCP;

bool pal_is_pl(UC p)
{
	int row = p / 16;
	if(row==0) return true;
	if(row>3) return false;
	return (p%2)==0;
}

bool pal_is_dith(UC p)
{
	return p >= 224;
}

void DBG_LOG(char*)
{
}

vector<RGB> palette;

void MakePal(const char* fn)
{
	RGB_Image img;

	std::ifstream ifs{fn, std::fstream::binary | std::fstream::in};
	LoadBMP(img, ifs);
	int i=0;
	int dx = img.w / 16;
	int dy = img.h / 16;
	for( int x=0; x<16; ++x )
		for( int y=0; y<16; ++y )
		{
			auto pix = img( x*dx + 2, y*dy + 2 );
			palette[i].r = pix.r;
			palette[i].g = pix.g;
			palette[i].b = pix.b;
			++i;
		}
}

// SMOKE SHADOW
UC SMOKE  [16] = {127, 135, 143, 151, 159, 167, 175, 183, 191, 199, 207, 215, 223, 231, 239, 255};
UC SHADOW [16] = {  0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88,  96, 104, 112, 120};

template<typename PAL>
void edit_palette(PAL& pal)
{
	// the latest part of palette is altered
	for (int i = 0; i < 16; ++i)
	{
		pal[i + 224].r = SMOKE  [i];
		pal[i + 224].g = SMOKE  [i];
		pal[i + 224].b = SMOKE  [i];
		pal[i + 240].r = SHADOW [i];
		pal[i + 240].g = SHADOW [i];
		pal[i + 240].b = SHADOW [i];
	}
}

void alib::CIS::LoadPCX(const char* fn)
{
	FILE* fp = fopen(fn,"rb");

	if (!fp) {
		pixels.clear(); pixeltypes.clear();
		w=h=0;
		return;
	}

	pcxHeader hdr;
	fread(&hdr,128,1,fp);

	int height = 1 + hdr.lower_marg - hdr.upper_marg;
	int width  = 1 + hdr.right_marg - hdr.left_marg;
	//if(!hdr.left_marg) --width;
	if(width==469) width=470;
	using std::vector;
	vector<vector<UC>> scanlines;
	scanlines.reserve(height);
	int x,y;
	int nbs = width * (hdr.bpp/8);
	if(nbs%2) ++nbs;

	if (hdr.NBS!=nbs)
		 throw ("error in header");

	for (y=0; y<height; ++y)
	{
		scanlines.emplace_back();
		scanlines.back().reserve(nbs+1);
		x=0;
		while(x<nbs)
		{
			UC c1 = getc(fp);

			if ((c1 & 0xC0) == 0xC0)
			{
				UC c2 = getc(fp);
				c1 &= 0x3F;
				while (c1)
				{
					if(x++>=nbs) throw ("error in image");
					scanlines[y].push_back(c2);
					--c1;
				}
			} else {
				if(x++>=nbs) throw ("error in image");
				scanlines[y].push_back(c1);
			}

			if (feof(fp)) { 
				throw ("unexpected eof"); break;
			}
		}
	}

	if (hdr.verinf>=5)
	{
		palette.resize(256);
		int c = getc(fp);
		assert(c==0x0c);
		for (int i=0; i<256; ++i)
		{
			palette[i].r = getc(fp);
			palette[i].g = getc(fp);
			palette[i].b = getc(fp);
			assert(!feof(fp));
		}
	}

	fclose(fp);

	edit_palette(palette);

	CIS& cis = (*this);

	cis.depth = 6;
	cis.w = 1 + hdr.right_marg - hdr.left_marg;
	cis.h = height;
	cis.hot.x = hdr.left_marg;
	cis.hot.y = hdr.upper_marg;

	int sz = cis.w * cis.h ;
	cis.pixels.clear();
	cis.pixels.reserve(sz);
	cis.pixeltypes.clear();
	cis.pixeltypes.reserve(sz);

	RGBA px;

	cis.has_dither = false;
	cis.has_colimp = false;
	cis.has_trans = false;
	cis.loaded = true;

	#ifndef NO_SFML
	cis.instanciated = false;
	#endif

	for (y=0; y<cis.h; ++y)
	{
		for (x=0; x<cis.w; ++x)
		{
			UC pix = scanlines[y][x];
			if (pix==255)
			{
				cis.pixeltypes.push_back(trans);
				cis.pixels.push_back({0,0,0,0});
				cis.has_trans = true;
			}
			else
			{
				//px = palette[pix];
				px.r = palette[pix].r;
				px.g = palette[pix].g;
				px.b = palette[pix].b;
				px.a = 255;
				if (pal_is_pl(pix))
				{
					cis.pixeltypes.push_back(colimp);
					cis.has_colimp = true;
				}
				else if (pal_is_dith(pix))
				{
					cis.pixeltypes.push_back(alpha);
					cis.has_dither = true;
					px.a = 128;
				}
				else
				{
					cis.pixeltypes.push_back(normal);
				}

				cis.pixels.push_back(RGBA_2_HSVA(px));
			}
		}

	}

	cis.Crop();
}

void alib::CIS::LoadDataPal(UC* data, RGB* pal, short w_, short h_, short hx_, short hy_)
{
	edit_palette(pal);

	CIS& cis = (*this);

	cis.depth = 6;
	cis.w = w_;
	cis.h = h_;
	cis.hot.x = hx_;
	cis.hot.y = hy_;

	int sz = cis.w * cis.h;
	cis.pixels.clear();
	cis.pixels.reserve(sz);
	cis.pixeltypes.clear();
	cis.pixeltypes.reserve(sz);

	RGBA px;

	cis.has_dither = false;
	cis.has_colimp = false;
	cis.has_trans = false;
	cis.loaded = true;
#ifndef NO_SFML
	cis.instanciated = false;
#endif
	for (int y = 0; y < cis.h; ++y)
	{
		for (int x = 0; x < cis.w; ++x)
		{
			UC pix = data[x + y*w];
			if (pix == 255)
			{
				cis.pixeltypes.push_back(trans);
				cis.pixels.push_back({0,0,0,0});
				cis.has_trans = true;
			}
			else
			{
				//px = palette[pix];
				px.r = pal[pix].b;
				px.g = pal[pix].g;
				px.b = pal[pix].r;
				px.a = 255;
				if (pal_is_pl(pix))
				{
					cis.pixeltypes.push_back(colimp);
					cis.has_colimp = true;
				}
				else if (pal_is_dith(pix))
				{
					cis.pixeltypes.push_back(alpha);
					cis.has_dither = true;
					px.a = 128;
				}
				else
				{
					cis.pixeltypes.push_back(normal);
				}
				cis.pixels.push_back(RGBA_2_HSVA(px));
			}
		}

	}
	cis.Crop();
}

void alib::BasicAnim::LoadSequence( const char* fn )
{
	char buff[256];
	int i = 0;
	BasicAnim ba;
	ba.delay = 87;
	ba.jbf = 0;
	ba.repeating = true;
	ba.anim.clear();
	while (true)
	{
		sprintf(buff, "%s_%03d.pcx", fn, i++);
		CIS cis;
		cis.LoadPCX(buff);
		if(!cis.Width()) break;
		ba.anim.push_back(move(cis));
	}
	(*this) = std::move(ba);

	//if constexpr(std::endian::native == std::endian::little)
	//{
	//	cout << "little endian\n";
	//}
}

