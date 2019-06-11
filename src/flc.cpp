
#include <cstdio>
#include <cassert>
#include <iostream>
#include <fstream>

#include "bitmap.hpp"
#include "alib.hpp"

typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;

#pragma pack(push, 1)


typedef struct {
  DWORD size;          /* Size of FLIC including this header */
  WORD  type;          /* File type 0xAF11, 0xAF12, 0xAF30, 0xAF44, ... */
  WORD  frames;        /* Number of frames in first segment */
  WORD  width;         /* FLIC width in pixels */
  WORD  height;        /* FLIC height in pixels */
  WORD  depth;         /* Bits per pixel (usually 8) */
  WORD  flags;         /* Set to zero or to three */
  DWORD speed;         /* Delay between frames */
  WORD  reserved1;     /* Set to zero */
  DWORD created;       /* Date of FLIC creation (FLC only) */
  DWORD creator;       /* Serial number or compiler id (FLC only) */
  DWORD updated;       /* Date of FLIC update (FLC only) */
  DWORD updater;       /* Serial number (FLC only), see creator */
  WORD  aspect_dx;     /* Width of square rectangle (FLC only) */
  WORD  aspect_dy;     /* Height of square rectangle (FLC only) */
  WORD  ext_flags;     /* EGI: flags for specific EGI extensions */
  WORD  keyframes;     /* EGI: key-image frequency */
  WORD  totalframes;   /* EGI: total number of frames (segments) */
  DWORD req_memory;    /* EGI: maximum chunk size (uncompressed) */
  WORD  max_regions;   /* EGI: max. number of regions in a CHK_REGION chunk */
  WORD  transp_num;    /* EGI: number of transparent levels */
  BYTE  reserved2[24]; /* Set to zero */
  DWORD oframe1;       /* Offset to frame 1 (FLC only) */
  DWORD oframe2;       /* Offset to frame 2 (FLC only) */
  BYTE  reserved3[40]; /* Set to zero */
} FLIC_HEADER;

typedef struct {
	DWORD size;          /* Size of the chunk, including subchunks */
	WORD  type;          /* Chunk type: 0xF1FA */
	WORD  chunks;        /* Number of subchunks */
	WORD  delay;         /* Delay in milliseconds */
	short reserved;      /* Always zero */
	WORD  width;        /* Frame width override (if non-zero) */
	WORD  height;       /* Frame height override (if non-zero) */
} FRAME_TYPE;

typedef struct {
	DWORD size;           /* Size of this chunk */
	WORD  type;           /* Chunk type: */
} CHUNK_START;

#pragma pack(pop)

using namespace alib;

namespace
{
	std::vector<RGB> palette{256};
	alib::BA curent_ba;
	alib::CIS cis;
	int w, h;
	int deltas = 0;
	int keys = 0;
	int pals = 0;

	//std::vector<BYTE> prev_frame;
	std::vector<BYTE> curr_frame;
	//int idx(int x, int y) { return y*w + x; }

	int dir  = 0;
	int didx = 0;
	int fri  = 0;
	int maxf = 0;
	int delta;
	bool done;
	int maxdir = 8;

	int hx, hy;

	short dirs[] = { 225, 270, 315, 0, 45, 90, 135, 180 };

	//alib::AD ad;
};


void push_frame(alib::AD& ad)
{
	alib::CIS cis;
	cis.LoadDataPal(curr_frame.data(), palette.data(), w, h, hx, hy);
	if (cis.Width() && cis.Height() && cis.Loaded())
		ad.AddFrameTo(dir,std::move(cis));
}

void ChunkKeyFrame(FILE* fp)
{
	++keys;
	signed char br;
	BYTE uc, pc;
	int idx = 0, sz = w*h;
	for (int y=0; y<h; ++y)
	{
		fread(&pc, 1, 1, fp); // packet count - ignore
		int x=0; idx = y*w;
		while (x<w)
		{
			fread(&br, 1, 1, fp); // byte run
			--pc;
			if (br < 0)
			{
				// literal run
				while (br++)
				{
					fread(&uc, 1, 1, fp);
					curr_frame[idx + x++] = uc;
				}
			}
			else if (br > 0)
			{
				// replicate run
				fread(&uc, 1, 1, fp);
				while (br--)
				{
					curr_frame[idx + x++] = uc;
				}
			}
			else
			{
				while (idx < sz)
					curr_frame[idx++] = 255;
				break;
			}
		}
	}
}


void ChunkDeltaFrame(FILE* fp)
{
	++deltas;

	UL     pos = 0;
	short  lines, l, j, i;
	short  b, color;
	BYTE   skip;
	char   change;

	// Number of lines that should change
	fread(&lines, sizeof(short), 1, fp);

	l = 0;
	while (lines > 0)
	{
		pos = l * w;

		fread(&b, sizeof(short), 1, fp);

		// Number of packets following
		if ((b & 0xC000) == 0x0000) // if 14 bit == 0
		{
			b &= 0x3FFF;   // Number of packets in low 14 bits

			for (j = 0; j < b; j++)
			{
				// Skip unchanged pixels
				fread(&skip, 1, 1, fp);
				pos += skip;

				// Pixels to change
				fread(&change, 1, 1, fp);

				if (change > 0)
				{
					for (i = 0; i < change; i++)
					{
						fread(&color, sizeof(short), 1, fp);
						curr_frame[pos++] = (color & 0x00FF);
						curr_frame[pos++] = ((color >> 8) & 0x00FF);
					}
				}
				else
				{
					change = -change;
					fread(&color, sizeof(short), 1, fp);

					for (i = 0; i < change; i++)
					{
						curr_frame[pos++] = (color & 0x00FF);
						curr_frame[pos++] = ((color >> 8) & 0x00FF);
					}
				}
			}

			lines--;
			l++;
		}
		// Number of lines that we should skip
		else if ((b & 0xC000) == 0xC000)
			l -= b;
		// Color of last pixel in row
		else
			curr_frame[pos++] = b & 0x00FF;
	}

}

void ChunkPalette(FILE* fp)
{
	++pals;
	int pal_index = 0;

	WORD num;
	fread(&num,2,1,fp);
	for (int i = 0; i < num; ++i)
	{
		BYTE skip, copy;
		fread(&skip, 1, 1, fp);
		fread(&copy, 1, 1, fp);
		int n = copy?copy:256;
		pal_index += skip;
		while(n--)
		{
			unsigned char rgb[3];
			fread( rgb, 3, 1, fp );
			palette[pal_index++] = RGB{rgb[2], rgb[1], rgb[0]};
		}
	}
}

#ifdef LOGGING
#define LOG(x) std::cout << x
#else
#define LOG(x) 
#endif

bool ReadFrame(FILE* fp, alib::AD& ad)
{
	FRAME_TYPE ft;
	fread(&ft, sizeof(ft), 1, fp);
	if (ft.type != 0xf1fa) return false;
	assert((ft.type== 0xf1fa) && "error in file");

	auto oldp = ftell(fp);

	auto snc = oldp;
	LOG("        ");
	for (int i = 0; i < ft.chunks; ++i)
	{
		CHUNK_START ch;
		fread(&ch, sizeof(ch), 1, fp);
		snc += ch.size;
		switch (ch.type)
		{
		case 4:
			LOG("(palette chunk)");
			ChunkPalette(fp);
			break;
		case 7:
			LOG("(delta chunk)");
			ChunkDeltaFrame(fp);
			break;
		case 15:
			LOG("(key chunk)");
			ChunkKeyFrame(fp);
			break;
		default:
			assert(false && "error in file");
		}
		fseek(fp, snc, SEEK_SET);
	};
	LOG(" while on dir " << dir << "\n");

	if (fri<maxf)
		push_frame(ad);
	if (fri >= maxf)
	{
		if (didx== maxdir) { done=true; return true; }
		dir = dirs[didx++];
		fri = 0;
		ad.CreateDir(dir, true, delta, 0);
	}
	else
	{
		++fri;
	}

	fseek(fp, oldp+ft.size-sizeof(ft), SEEK_SET);
	return true;
}

bool LoadFLC(const char* fn, alib::AD& ad)
{
	ad.Clear();
	deltas = 0;
	didx = fri = 0;

	FILE* fp = fopen(fn,"rb");

	FLIC_HEADER fh;
	fread( &fh, sizeof(fh), 1, fp );
	//LOG("    FLC type: 0x" << std::hex << fh.type);
	//assert((fh.type==0xaf12) && "error in file");
	if (fh.type != 0xaf12) return false;
	//LOG("  frames: " << std::dec << fh.frames << "\n");

	maxdir = fh.reserved3[8];
	if (maxdir==0) maxdir=8;
	assert(maxdir != 0);
	assert((fh.frames%maxdir) == 0);
	maxf = fh.frames / maxdir;
	delta = fh.speed;

	auto tmp = fh.reserved3[10];
	assert((tmp == 0) || (tmp == maxf));
	if ((tmp == 0) || (fh.reserved3[8] == 0))
	{
		std::cerr << "warning: nonstandard header in " << fn << std::endl;
	}

	int ow = fh.reserved3[16];
	int oh = fh.reserved3[18];

	w = fh.width;
	h = fh.height;

	hx = (ow/2) - fh.reserved3[12];
	hy = (oh/2) - fh.reserved3[14];

	auto sz = w*h;
	curr_frame.resize(sz);

	dir = dirs[didx++];
	fri = 0;
	ad.CreateDir(dir, true, delta, 0);
	done = false;

	while (!done)
	{
		bool ok = ReadFrame(fp, ad);
		if (!ok) return false;
	}

	return true;
}



