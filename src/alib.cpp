
#include <cassert>
#include <utility>
#include <algorithm>
#include <vector>
#include <fstream>
#include <iostream>
#include <streambuf>

// #pragma hdrstop

#ifndef NO_SFML
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Drawable.hpp>
#endif

#include <boost/algorithm/string.hpp>

#include "bitmap.hpp"
#include "alib.hpp"
#include "bitstream.hpp"

alib::RGBA alib::HSVA_2_RGBA(const HSVA& hsv)
{
	RGBA rgb;
	rgb.a = hsv.a;

	UC reg, rem, p, q, t;

	if (hsv.s == 0)
	{
		rgb.r = hsv.v;
		rgb.g = hsv.v;
		rgb.b = hsv.v;
		return rgb;
	}

	reg = hsv.h / 43;
	rem = (hsv.h - (reg * 43)) * 6;

	p = (hsv.v * (255 - hsv.s)) >> 8;
	q = (hsv.v * (255 - ((hsv.s*rem) >> 8))) >> 8;
	t = (hsv.v * (255 - ((hsv.s * (255 - rem)) >> 8))) >> 8;

	switch (reg)
	{
		case 0:  rgb.r = hsv.v; rgb.g = t;     rgb.b = p;     break;
		case 1:  rgb.r = q;     rgb.g = hsv.v; rgb.b = p;     break;
		case 2:  rgb.r = p;     rgb.g = hsv.v; rgb.b = t;     break;
		case 3:  rgb.r = p;     rgb.g = q;     rgb.b = hsv.v; break;
		case 4:  rgb.r = t;     rgb.g = p;     rgb.b = hsv.v; break;
		default: rgb.r = hsv.v; rgb.g = p;     rgb.b = q;     break;
	}

	return rgb;
}

namespace detail
{
	template<typename T, typename... Args>
	void impl_minmax(std::pair<T, T>&) {}

	template<typename T, typename... Args>
	void impl_minmax(std::pair<T, T>& result, const T& val, const Args&... args)
	{
		if (val < result.first)  result.first  = val;
		if (val > result.second) result.second = val;
		detail::impl_minmax(result, args...);
	}
}

template<typename T, typename... Args>
std::pair<T, T> minmax(const T& val, const Args&... rest)
{
	std::pair<T, T> result{val, val};
	detail::impl_minmax(result, rest...);
	return result;
}

alib::HSVA alib::RGBA_2_HSVA(const RGBA& rgb)
{
	HSVA hsv = {0,0,0,rgb.a};

	auto mm = minmax(rgb.r, rgb.g, rgb.b);
	auto& rgb_min = mm.first;
	auto& rgb_max = mm.second;

	int delta = rgb_max - rgb_min;

	hsv.v = rgb_max;

	if (!hsv.v) return hsv;

	hsv.s = UC(255 * delta / hsv.v);
	if (!hsv.s) return hsv;

	/**/ if (rgb_max == rgb.r) { hsv.h = UC(  0 + 43 * (rgb.g - rgb.b) / delta ); }
	else if (rgb_max == rgb.g) { hsv.h = UC( 85 + 43 * (rgb.b - rgb.r) / delta ); }
	else if (rgb_max == rgb.b) { hsv.h = UC(171 + 43 * (rgb.r - rgb.g) / delta ); }
	else assert(false);

	return hsv;
}

// ----------------------------------------------------------------------------

// ***********
// *** CIS ***
// ***********

alib::CIS::CIS()
{
#ifndef NO_SFML
	instanciated = 
#endif
	loaded = false;
}


template <typename T> inline void ReadBinary(std::istream& istr, T& val)
{
	int i, n = sizeof(T);
	char* p = (char*)&val;

	for (i = 0; i < n; ++i)
	{
		char c;
		istr.read(&c, 1);
		(*p) = c;
		++p;
	}
}

void alib::CIS::LoadOld(std::istream& is)
{
	ReadBinary(is, w);
	ReadBinary(is, h);
	ReadBinary(is, hot.x);
	ReadBinary(is, hot.y);
	depth = 4;
	LoadInternal(is, true);
}

void alib::CIS::Load(std::istream& is)
{
	char buff[5] = {};

	is.read(buff, 4);
	std::string magic = buff;
	if (magic == "CIS2")
	{
		ReadBinary(is, w);
		ReadBinary(is, h);
		ReadBinary(is, hot.x);
		ReadBinary(is, hot.y);
		ReadBinary(is, depth);
		LoadInternal(is);
	}
	else if (boost::to_lower_copy(magic) == "cis3")
	{
		streamsource ss{is};
		LBS(6, ss);
	}
	else
	{
		is.putback(buff[3]).putback(buff[2]).putback(buff[1]).putback(buff[0]);
		LoadOld(is);
	}
}

void alib::CIS::LoadInternal(std::istream& in,bool)
{
	Clear();

	has_dither = has_trans = has_colimp = false;

	int i, sz = w * h;

	pixeltypes.clear();
	pixeltypes.reserve(sz);

	unsigned char c;
	int leftbit = 0;

	for (i = 0; i < sz; ++i)
	{
		if (!leftbit)
		{
			ReadBinary(in, c);
			leftbit = 8;
		}

		PixelType pt = (PixelType)(c & 3);
		leftbit -= 2;
		c = c >> 2;

		pixeltypes.push_back(pt);

		switch (pt)
		{
			case alpha:   has_dither = true; break;
			case trans:   has_trans  = true; break;
			case colimp:  has_colimp = true; break;
			default: break;
		}

	}

	int odd = 0;
	UC cc;

	auto GetC_4 = [&odd, &cc](std::istream& is) -> UC
	{
		if (!odd)
		{
			odd = !odd;
			ReadBinary(is, cc);
			return (cc & 0x0F) << 4;
		}
		else {
			odd = !odd;
			return (cc & 0xF0);
		}
	};

	auto GetC_6 = [&odd, &cc](std::istream& is) -> UC
	{
		UC c2 = cc;
		switch (odd++ % 4)
		{
		case 0:  // 0 + 6
			ReadBinary(is, cc);
			return cc & 0xFC;
		case 1:  // 2 + 4
			ReadBinary(is, cc);
			return ((c2 & 0x03) << 6) | ((cc & 0xF0) >> 2);
		case 2:  // 4 + 2
			ReadBinary(is, cc);
			return ((c2 & 0x0F) << 4) | ((cc & 0xC0) >> 4);
		default: // 6 + 0
			return (c2 & 0x3F) << 2;
		}
	};

	auto GetC_8 = [&odd, &cc](std::istream& is) -> UC
	{
		(void)odd;
		ReadBinary(is, cc);
		return cc;
	};

	auto GetC = [&](std::istream& is) -> UC
	{
		/**/ if (this->depth == 4) return GetC_4(is);
		else if (this->depth == 6) return GetC_6(is);
		else if (this->depth == 8) return GetC_8(is);
		else { assert(false); return 0; }
	};

	pixels.clear();
	pixels.reserve(sz);
	for (i = 0; i < sz; ++i)
	{
		HSVA p = {0,0,0,255};
		switch (pixeltypes[i])
		{
			case alpha:  p.a = 128;  GetC(in);GetC(in);GetC(in); /*need to skip 3 due to bug in writer*/ break;
			case normal: p.h = GetC(in); p.s = GetC(in); p.v = GetC(in); break;
			case colimp:                 p.s = GetC(in); p.v = GetC(in); break;
			case trans:  p.a = 0; break;
		}
		pixels.push_back(p);
	}

	loaded = true;
#ifndef NO_SFML
	instanciated = false;
#endif
}

bool alib::CIS::LBS(UC dep, bitsource& src)
{
	Clear();

	if (!src.have(dep*2))
		return false;
	w = (unsigned short)src.get(2*dep);
	if (!src.have(dep * 2))
		return false;
	h = (unsigned short)src.get(2*dep);
	if (!src.have(dep * 2))
		return false;
	hot.x = (signed short)src.getS(2*dep);
	if (!src.have(dep * 2))
		return false;
	hot.y = (signed short)src.getS(2*dep);
	depth = dep;

	//assert(depth==6);

	has_dither = has_trans = has_colimp = false;

	int i, sz = w * h;

	pixeltypes.clear();
	pixeltypes.reserve(sz);

	for (i = 0; i < sz; ++i)
	{
		if (!src.have(2))
			return false;
		PixelType pt = (PixelType)(src.get(2));
		pixeltypes.push_back(pt);
		switch (pt)
		{
			case alpha:   has_dither = true; break;
			case trans:   has_trans  = true; break;
			case colimp:  has_colimp = true; break;
			default: break;
		}
	}
	//src.get( (sz*2) % dep );

	pixels.clear();
	pixels.reserve(sz);
	for (i = 0; i < sz; ++i)
	{
		HSVA p = {0,0,0,255};
		switch (pixeltypes[i])
		{
		case alpha:
			p.a = 128;
			if (!src.have(depth * 1))
				return false;
			p.v = (UC)src.get(depth) << (8 - depth);
			break;
		case normal:
			if (!src.have(depth * 3))
				return false;
			p.h = (UC)src.get(depth) << (8 - depth);
			p.s = (UC)src.get(depth) << (8 - depth);
			p.v = (UC)src.get(depth) << (8 - depth);
			break;
		case colimp:
			if (!src.have(depth * 2))
				return false;
			p.s = (UC)src.get(depth) << (8 - depth);
			p.v = (UC)src.get(depth) << (8 - depth);
			break;
		case trans:
			p.a = 0;
			break;
		}
		pixels.push_back(p);
	}

	loaded = true;
#ifndef NO_SFML
	instanciated = false;
#endif
	return true;
}

void alib::CIS::SBT(UC dep, bittarget& trg) const
{
	assert(w && h && loaded);

	trg.put(w, dep * 2);
	trg.put(h, dep * 2);
	trg.put((UL)hot.x, dep * 2);
	trg.put((UL)hot.y, dep * 2);

	//depth = dep;

	int i, sz = w * h;

	assert(pixeltypes.size() == sz);

	for (i = 0; i < sz; ++i)
	{
		PixelType pt = pixeltypes[i];
		trg.put(pt, 2);
		switch (pt)
		{
			case alpha:   assert(has_dither); break;
			case trans:   assert(has_trans ); break;
			case colimp:  assert(has_colimp); break;
			default: break;
		}
	}

	//trg.put(0, (sz * 2) % dep);

	assert(pixels.size() == sz);
	for (i = 0; i < sz; ++i)
	{
		HSVA p = pixels[i];
		switch (pixeltypes[i])
		{
		case alpha:
			trg.put(p.v >> (8 - dep), dep);
			break;
		case normal:
			trg.put(p.h >> (8 - dep), dep);
			trg.put(p.s >> (8 - dep), dep);
			trg.put(p.v >> (8 - dep), dep);
			break;
		case colimp:
			trg.put(p.s >> (8 - dep), dep);
			trg.put(p.v >> (8 - dep), dep);
			break;
		case trans:
			break;
		}
	}
}

// normal = 0, alpha, trans, colimp };
namespace alib {

#ifndef NDEBUG
//auto& log = std::cerr;
std::ostream log{nullptr};
#else
std::ostream log{nullptr};
#endif

static const RGB nrm = {200,50,50};
static const RGB alp = {127,127,127};
static const RGB tra = {255,255,255};
static const RGB col = {50,50,200};
void PixTypeSave(int w, int h, const char* lead, int ii, const std::vector<CIS::PixelType>& pts)
{
	RGB_Image img;
	img.w = w; img.h = h;
	int i, n = w*h;
	assert (n == pts.size());
	img.pix.reserve(n);
	for (int y=0; y<h; ++y) for (int x=0; x<w; ++x)
	{
		i = x + (h-y-1)*w;
		switch (pts[i])
		{
		case CIS::normal: img.pix.push_back(nrm); break;
		case CIS::alpha:  img.pix.push_back(alp); break;
		case CIS::trans:  img.pix.push_back(tra); break;
		case CIS::colimp: img.pix.push_back(col); break;
		}
	}
	std::ofstream ofs{"./gfx/"s + lead + std::to_string(ii) + ".bmp"s, std::fstream::binary };
	SaveBMP(img, ofs);
}}

void alib::CIS::SavePacked(UC dep, compress_bypass_target& trg) const
{
	using namespace std;

	assert(w && h && loaded);

	UL WW, HH, HX, HY;
	WW = ((w + (w%2)) / 2) - 1;
	HH = ((h + (h%2)) / 2) - 1;
	assert(WW<1024);
	assert(HH<1024);
	HX = UL(hot.x); HX &= ((1ul<<12) - 1);
	HY = UL(hot.y); HY &= ((1ul<<12) - 1);

	trg.bypass(WW, 10);
	trg.bypass(HH, 10);
	trg.bypass(HX, 12);
	trg.bypass(HY, 12);

	log << "w,h : " << w << "," << h << endl;
	log << "hx,hy : " << hot.x << "," << hot.y << endl;
	log << "W,H : " << WW << "," << HH << endl;

	WW = (WW + 1) * 2;
	HH = (HH + 1) * 2;

	log << "WW,HH : " << WW << "," << HH << endl;

	int sz = w * h;

	assert(pixeltypes.size() == sz);

	struct Item {
		PixelType pt;
		UC hval, sval, vval;
		bool haveh, haves, havev;
	};
	static const bool f = false, t = true;

	UL SZ = WW*HH;

	log << "sz,SZ : " << sz << "," << SZ << endl;

	std::vector<Item> items; items.reserve(SZ);
	for (UL i = 0; i<SZ; ++i)
	{
		PixelType pt;
		HSVA pix;
		int x = i % WW;
		int y = i / WW;
		if ( (x>=w) || (y>=h) )
		{
			items.push_back({trans, 0,0,0, f,f,f});
			continue;
		} else {
			auto idx = y*w + x;
			assert(idx<sz);
			pt = pixeltypes[idx];
			pix = pixels[idx];
			pix.h >>= (8 - dep);
			pix.s >>= (8 - dep);
			pix.v >>= (8 - dep);
		}
		switch (pt)
		{
		case trans:
			assert(has_trans);
			items.push_back({pt, 0,0,0, f,f,f});
			break;
		case alpha:
			assert(has_dither);
			items.push_back({pt, 0, 0, pix.v, f, f, t});
			break;
		case normal:
			items.push_back({pt, pix.h, pix.s, pix.v, t, t, t});
			break;
		case colimp:
			assert(has_colimp);
			items.push_back({pt, 0, pix.s, pix.v, f, t, t});
			break;
		default:
			assert( false && "pixeltype error" );
		}
	}

	assert(items.size() == SZ);
	BVec tokens;
	tokens.reserve(SZ);
	int dbg = 0;

	auto push = [&](PixelType pt, UL cnt) -> void
	{
		assert(cnt);
		while (cnt >= 64)
		{
			trg.bypass( (63ul << 2) | pt , 8 );
			++dbg;
			cnt -= 64;
		}
		if (cnt)
		{
			trg.bypass( (--cnt << 2) | pt, 8);
			++dbg;
		}
	};

	bool first = true;
	PixelType pt;
	UL cnt;
	// part one, the pixtype component
	for (const auto& itm : items)
	{
		if (first)
		{
			pt = itm.pt;
			cnt = 1;
			first = false;
		}
		else if (itm.pt != pt)
		{
			if (cnt) push(pt, cnt);
			pt = itm.pt; cnt=1;
		}
		else
		{
			++cnt;
		}
	}
	if (cnt) push(pt, cnt);

	log << "pushed " << dbg << " rle-tokens for " << SZ << " pixtypes\n";

	//static int sv = 1; PixTypeSave(w, h, "save_", sv++, pixeltypes);

	// part two, the V component
	tokens.clear();
	for (const auto& itm : items)
	{
		if (itm.havev)
			tokens.push_back(itm.vval);
	}
	trg.compress(tokens);
	log << "luma 6 x " << tokens.size() << "\n";

	auto idx = [&](int x, int y) { return x + y*WW; };
	//auto inside = [&](int x, int y) -> bool { return x>=0 && y>=0 && x<w && y<h; };
	auto get = [&](UL x, UL y) -> Item
	{
		assert(x<WW && y<HH);
		//if (inside(x,y))
			return items[idx(x,y)];
		//else
		//	return {trans, 0,0,0, f,f,f};
	};

	UL mr_hh = 0;
	UL mr_hs = 0;
	auto merge_4 = [&](const Item& i1, const Item& i2, const Item& i3, const Item& i4) -> Item
	{
		Item itm;
		itm.haveh = i1.haveh || i2.haveh || i3.haveh || i4.haveh;
		itm.haves = i1.haves || i2.haves || i3.haves || i4.haves;
		if (itm.haveh) ++mr_hh;
		if (itm.haves) ++mr_hs;

		if (itm.haveh)
		{
			UL hh = 0, hc = 0;
			if (i1.haveh) { hh += i1.hval; ++hc; }
			if (i2.haveh) { hh += i2.hval; ++hc; }
			if (i3.haveh) { hh += i3.hval; ++hc; }
			if (i4.haveh) { hh += i4.hval; ++hc; }
			itm.hval = hh / hc;
		} else
			itm.hval = 0;
		if (itm.haves)
		{
			UL ss = 0, sc = 0;
			if (i1.haves) { ss += i1.sval; ++sc; }
			if (i2.haves) { ss += i2.sval; ++sc; }
			if (i3.haves) { ss += i3.sval; ++sc; }
			if (i4.haves) { ss += i4.sval; ++sc; }
			itm.sval = ss / sc;
		}
		else
			itm.sval = 0;

		return itm;
	};

	// preparation for part three and four, reduce HS resolution
	UL downsize = SZ/4; log << "downsize : " << downsize << std::endl;
	std::vector<Item> items_2; items_2.reserve(downsize);
	assert((SZ%4) == 0);
	for (UL i = 0; i < downsize; ++i)
	{
		UL x2 = (i % (WW/2))*2;
		UL y2 = (i / (WW/2))*2;
		items_2.push_back(
			merge_4(
				get(x2+0, y2+0),
				get(x2+1, y2+0),
				get(x2+0, y2+1),
				get(x2+1, y2+1)
			)
		);
	}

	log << "merge result : have hue " << mr_hh << endl;
	log << "merge result : have sat " << mr_hs << endl;

	// part three, the H component
	tokens.clear();
	for (const auto& itm : items_2)
	{
		if (itm.haveh)
			tokens.push_back(itm.hval);
	}
	trg.compress(tokens);
	log << "hue 6 x " << tokens.size() << "\n";

	// part three, the S component
	tokens.clear();
	for (const auto& itm : items_2)
	{
		if (itm.haves)
			tokens.push_back(itm.sval);
	}
	trg.compress(tokens);
	log << "sat 6 x " << tokens.size() << "\n";

}

bool alib::CIS::LoadPacked(UC dep, decompress_bypass_source& src)
{
	using namespace std;

	UL val;
	if (!src.have(10)) return false;
	val = src.get(10);
	log << "W,H : " << val;
	w = (val+1)*2;
	if (!src.have(10)) return false;
	val = src.get(10);
	log << "," << val << endl;
	h = (val+1)*2;
	if (!src.have(12)) return false;
	hot.x = (short)src.getS(12);
	if (!src.have(12)) return false;
	hot.y = (short)src.getS(12);

	UL sz = w * h;
	pixels.resize(sz);
	pixeltypes.resize(sz);

	log << "hx,hy : " << hot.x << "," << hot.y << endl;
	log << "WW,HH : " << w << "," << h << endl;
	log << "sz : " << sz << endl;

	struct Item {
		PixelType pt;
		UC hval, sval, vval;
		bool haveh, haves, havev;
	};

	std::vector<Item> items;
	items.reserve(sz);
	static const bool f = false, t = true;

	has_dither = has_trans = has_colimp = f;

	UL ii = 0, nn = 0, dbg=0;

	while (ii<sz)
	{
		bool ok = src.have(8);
		if (!ok) return false;
		UC xx = src.get(8); ++dbg;
		nn = (xx >> 2) + 1;
		PixelType pt = PixelType(xx & 3);
		while (nn--)
		{
			if (ii>=sz) break;
			pixeltypes[ii++] = pt;
		}
	}
	log << "unpacked " << dbg << " rle tokens into " << ii << " pixtypes";
	if (ii!=sz)
		log << ", should be " << sz;
	log << ".\n";

	assert(ii == sz);

	//static int ld = 1; PixTypeSave(w,h,"load_", ld++, pixeltypes);

	ii=0;
	for (auto& pt : pixeltypes)
	{
		pt = pixeltypes[ii];
		auto& pix = pixels[ii];
		switch (pt)
		{
		case trans:
			has_trans = true;
			pix.a = 0;
			items.push_back({pt, 0,0,0, f,f,f});
			break;
		case alpha:
			has_dither = true;
			pix.a = 128;
			items.push_back({pt, 0,0,0, f,f,t});
			break;
		case normal:
			pix.a = 255;
			items.push_back({pt, 0,0,0, t,t,t});
			break;
		case colimp:
			has_colimp = true;
			pix.a = 255;
			items.push_back({pt, 0,0,0, f,t,t});
			break;
		default:
			assert(false && "pixeltype error");
		}
		++ii;
	}

	ii = nn = 0;
	for (auto& itm : items)
	{
		if (itm.havev)
		{
			[[maybe_unused]] bool ok = src.have(dep);
			assert (ok);
			itm.vval = src.get(dep) << (8-dep);
			pixels[ii].v = itm.vval;
			++nn;
		}
		++ii;
	}
	log << "luma " << nn << " vals\n";

	auto idx = [&](int x, int y) { return x + y * w; };

	UL mr_hh = 0;
	UL mr_hs = 0;

	auto merge_4 = [&](const Item& i1, const Item& i2, const Item& i3, const Item& i4) -> Item
	{
		Item itm;
		itm.haveh = i1.haveh || i2.haveh || i3.haveh || i4.haveh;
		itm.haves = i1.haves || i2.haves || i3.haves || i4.haves;
		if (itm.haveh) ++mr_hh;
		if (itm.haves) ++mr_hs;
		return itm;
	};

	// preparation for part three and four, reduce HS resolution
	UL downsize = sz / 4; log << "downsize : " << downsize << std::endl;
	std::vector<Item> items_2; items_2.reserve(downsize);
	for (UL i = 0; i < downsize; ++i)
	{
		int x2 = (i % (w/2)) * 2;
		int y2 = (i / (w/2)) * 2;
		items_2.push_back(
			merge_4(
				items[idx(x2 + 0, y2 + 0)],
				items[idx(x2 + 1, y2 + 0)],
				items[idx(x2 + 0, y2 + 1)],
				items[idx(x2 + 1, y2 + 1)]
			)
		);
	}

	log << "merge result : have hue " << mr_hh << endl;
	log << "merge result : have sat " << mr_hs << endl;

	nn = 0;
	for (UL i = 0; i < downsize; ++i)
	{
		int x2 = (i % (w/2)) * 2;
		int y2 = (i / (w/2)) * 2;
		auto itm = items_2[i];
		if (itm.haveh)
		{
			[[maybe_unused]] bool ok = src.have(dep);
			assert (ok);
			UC hval = src.get(dep) << (8-dep); ++nn;
			items[idx(x2 + 0, y2 + 0)].hval = hval;
			items[idx(x2 + 1, y2 + 0)].hval = hval;
			items[idx(x2 + 0, y2 + 1)].hval = hval;
			items[idx(x2 + 1, y2 + 1)].hval = hval;
		}
	}
	log << "hue " << nn << " vals\n";

	nn = 0;
	for (UL i = 0; i < downsize; ++i)
	{
		int x2 = (i % (w/2)) * 2;
		int y2 = (i / (w/2)) * 2;
		auto itm = items_2[i];
		if (itm.haves)
		{
			[[maybe_unused]] bool ok = src.have(dep);
			assert (ok);
			UC sval = src.get(dep) << (8 - dep); ++nn;
			items[idx(x2 + 0, y2 + 0)].sval = sval;
			items[idx(x2 + 1, y2 + 0)].sval = sval;
			items[idx(x2 + 0, y2 + 1)].sval = sval;
			items[idx(x2 + 1, y2 + 1)].sval = sval;
		}
	}
	log << "sat " << nn << " vals\n";

	for (UL i = 0; i < sz; ++i)
	{
		pixels[i].h = items[i].hval;
		pixels[i].s = items[i].sval;
	}

	loaded = true;
	return true;
}

#ifndef NO_SFML

auto alib::CIS::Refl(UC hue) -> AnimReflection
{
	return {this, hue};
}

auto alib::CIS::make(UC hue) const -> ImgMap::iterator
{
	assert(loaded);

	auto itr = images.find(hue);
	if (itr != images.end()) return itr;
	auto res = images.try_emplace(hue);
	assert(res.second);

	sf::Image img;

	std::vector<RGBA> pix;
	UL i, n = w*h;
	pix.resize(n);

	for (i=0; i<n; ++i)
	{
		RGBA& rgba = pix[i];
		HSVA hsva = pixels[i];
		if (pixeltypes[i] == alpha)
		{
			rgba = RGBA{hsva.v,hsva.v,hsva.v,128};
		}
		else if (pixeltypes[i] == trans)
		{
			rgba = RGBA{0,0,0,0};
		}
		else
		{
			HSVA hsva = pixels[i];
			if (pixeltypes[i] == colimp)
				hsva.h = hue;
			rgba = HSVA_2_RGBA(hsva);
		}
	}

	img.create(w,h,(UC*)pix.data());
	[[maybe_unused]] bool ok = res.first->second.loadFromImage(img);
	assert(ok);

	instanciated = true;
	return res.first;
}

void alib::CIS::Instance(UC h)
{
	make(h);
}

sf::Texture alib::CIS::Get(UC h) const
{
	auto itr = make(h);
	return itr->second;
}

sf::Texture& alib::CIS::Get(UC h)
{
	auto itr = make(h);
	return itr->second;
}
#endif

void alib::CIS::Clear()
{
	pixeltypes.clear();
	pixels.clear();
	loaded = false;
#ifndef NO_SFML
	images.clear();
	instanciated = loaded = false;
#endif
}

unsigned short alib::CIS::Width() const
{
	return (loaded) ? w : 0;
}

unsigned short alib::CIS::Height() const
{
	return (loaded) ? h : 0;
}

bool alib::CIS::Loaded() const
{
	return loaded;
}

void alib::CIS::UnloadBase()
{
	loaded = false;
	pixeltypes.clear();
	pixels.clear();
}

alib::CIS alib::CIS::Flip(bool fx, bool fy, bool rot) const
{
	assert(loaded);

	CIS cis;

	auto& pt = cis.pixeltypes;
	auto& px = cis.pixels;

	if (rot)
	{
		cis.w  = h;
		cis.h  = w;
		cis.hot.x = hot.y;
		cis.hot.y = hot.x;
	}
	else {
		cis.w  = w;
		cis.h = h;
		cis.hot.x = fx ? w - hot.x : hot.x;
		cis.hot.y = fy ? h - hot.y : hot.y;
	}
	cis.has_dither = has_dither;
	cis.has_trans  = has_trans;
	cis.has_colimp = has_colimp;
	cis.loaded       = true;

#ifndef NO_SFML
	cis.instanciated = false;
#endif

	int x, y, i, n = w * h;

	pt.clear(); pt.reserve(n);
	px.clear(); px.reserve(n);

	for (i = 0; i < n; ++i)
	{
		if (rot)
		{
			x = fx ? h - (i%h) - 1 : (i%h);
			y = fy ? w - (i / h) - 1 : (i / h);
		}
		else {
			x = fx ? w - (i%w) - 1 : (i%w);
			y = fy ? h - (i / w) - 1 : (i / w);
		}
		pt.push_back(pixeltypes[x + y * w]);
		px.push_back(pixels[x + y * w]);
	}
	return cis;
}

void alib::CIS::Crop()
{
	const int W = w;
	auto xy = [&W](int x, int y) -> int { return y * W + x; };
	int topcrop = 0;
	while (topcrop < h)
	{
		bool empty = true;
		for (int x = 0; empty && (x < w); ++x)
		{
			if (pixeltypes[xy(x, topcrop)] != trans)
				empty = false;
		}
		if (empty) ++topcrop; else break;
	}
	if (topcrop == h) { w = h = 0; pixeltypes.clear(); pixels.clear(); return; }
	int botcrop = 0;
	while (true)
	{
		bool empty = true;
		for (int x = 0; empty && (x < w); ++x)
		{
			if (pixeltypes[xy(x, h - botcrop - 1)] != trans)
				empty = false;
		}
		if (empty) ++botcrop; else break;
	}
	int lftcrop = 0;
	while (true)
	{
		bool empty = true;
		for (int y = 0; empty && (y < h); ++y)
		{
			if (pixeltypes[xy(lftcrop, y)] != trans)
				empty = false;
		}
		if (empty) ++lftcrop; else break;
	}
	int rghcrop = 0;
	while (true)
	{
		bool empty = true;
		for (int y = 0; empty && (y < h); ++y)
		{
			if (pixeltypes[xy(w - rghcrop - 1, y)] != trans)
				empty = false;
		}
		if (empty) ++rghcrop; else break;
	}
	int new_w = w - lftcrop - rghcrop;
	int new_h = h - topcrop - botcrop;
	assert((new_w > 0) && (new_w <= w) && (new_h > 0) && (new_h <= h));
	std::vector<PixelType> new_pt; new_pt.reserve(new_w*new_h);
	std::vector<HSVA>      new_px; new_px.reserve(new_w*new_h);
	for (int y = 0; y < new_h; ++y) for (int x = 0; x < new_w; ++x)
	{
		int idx = xy(x + lftcrop, y + topcrop);
		new_pt.push_back(pixeltypes[idx]);
		new_px.push_back(pixels[idx]);
	}
	pixeltypes = std::move(new_pt);
	pixels = std::move(new_px); //.clear(); pixels.assign(new_px.begin(), new_px.end());
	w = new_w; h = new_h;
	hot.x -= lftcrop; hot.y -= topcrop;
}

namespace alib {
	bool operator==(const alib::RGB& lhs, const alib::RGB& rhs)
	{
		return
			lhs.r == rhs.r &&
			lhs.g == rhs.g &&
			lhs.b == rhs.b;
	}
}

void alib::CIS::LoadBMP(const char* fn, RGB colorkey, short hx, short hy)
{
	RGB_Image img;
	std::ifstream ifs{fn, std::fstream::binary | std::fstream::in};
	alib::LoadBMP(img, ifs);

	CIS& cis = *this;

	cis.w = (unsigned short)img.w;
	cis.h = (unsigned short)img.h;
	cis.hot = {hx,hy};

	has_dither = has_trans = has_colimp = false;

	int i, sz = cis.w * cis.h;

	cis.pixels.clear();
	cis.pixels.reserve(sz);
	cis.pixeltypes.clear();
	cis.pixeltypes.reserve(sz);

	HSVA tr = RGBA_2_HSVA({0,0,0,0});
	for (i = 0; i < sz; ++i)
	{
		RGB rgb = img.pix[i];
		if (rgb == colorkey)
		{
			cis.pixeltypes.push_back(trans);
			cis.pixels.push_back(tr);
			has_trans = true;
		}
		else
		{
			cis.pixeltypes.push_back(normal);
			cis.pixels.push_back(RGBA_2_HSVA({rgb.r, rgb.g, rgb.b, 255}));
		}
	}

	loaded = true;
}

void alib::CIS::SaveBMP(const char* fn, UC hue, RGB colorkey) const
{
	assert(loaded);
	RGB_Image img;

	img.w = w;
	img.h = h;
	UL sz = w*h;
	img.pix.resize(sz);
	for (auto i = 0ul; i < sz; ++i)
	{
		UL x = i % w;
		UL y = i / w;
		RGB& rgb = img.pix[x + (h-y-1)*w];
		RGBA p;
		switch (pixeltypes[i])
		{
			case normal:
				p = HSVA_2_RGBA(pixels[i]);
				rgb = {p.b, p.g, p.r};
				break;
			case trans:
				rgb = colorkey;
				break;
			case alpha:
				if ((x+y) % 2)
				{
					p = HSVA_2_RGBA(pixels[i]);
					rgb = {p.b, p.g, p.r};
				} else {
					rgb = colorkey;
				}
				break;
			case colimp:
				p = HSVA_2_RGBA({hue,pixels[i].s,pixels[i].v,255});
				rgb = {p.b, p.g, p.r};
				break;
			default:
				assert(false && "pixel-type error");
				break;
		}
	}
	std::ofstream ofs{fn, std::fstream::binary | std::fstream::out};
	alib::SaveBMP(img, ofs);
}

// ----------------------------------------------------------------------------

// *****************
// *** BasicAnim ***
// *****************

void alib::BasicAnim::LoadOld(std::istream& is)
{
	ReadBinary(is, delay);
	short val;
	ReadBinary(is, val);
	repeating = (bool)val;
	jbf = 0;
	LoadInternal(is, true);
}

void alib::BasicAnim::Load(std::istream& is)
{
	char buff[5] = {};
	is.read(buff, 4);
	std::string magic = buff;
	boost::to_lower(magic);
	if (magic == "bap1")
	{
		streamsource ss{is};
		decompress_bypass_source dbs{6, ss};
		LoadPacked(6, dbs);
	}
	else if (magic == "ba_2")
	{
		ReadBinary(is, delay);
		short val;
		ReadBinary(is, val);
		repeating = (bool)(val >> 15);
		jbf = val & 0x7FFF;
		LoadInternal(is);
	}
	else if (magic == "ba_3")
	{
		streamsource ss{is};
		LBS(6, ss);
	}
	else
	{
		is.putback(buff[3]).putback(buff[2]).putback(buff[1]).putback(buff[0]);
		LoadOld(is);
	}
}

void alib::BasicAnim::LoadInternal(std::istream& in, bool)
{
	short val;
	ReadBinary(in, val);
	anim.resize(val);
	for (CIS& cis : anim)
	{
		cis.Load(in);
	}
	//delay = short(delay*0.75f);
}

bool alib::BasicAnim::LBS(UC dep, bitsource& src)
{
	if (!src.have(dep*2)) return false;
	delay = (short)src.get(dep*2);
	if (!src.have(dep * 2)) return false;
	UL val = src.get(dep*2);
	repeating = (bool)(val >> (dep*2-1));
	jbf = val & ((1<<(dep*2-1))-1);
	if (!src.have(dep * 2)) return false;
	val = src.get(dep * 2);
	anim.clear();
	anim.reserve(val);
	while (val--)
	{
		CIS cis;
		bool ok = cis.LBS(dep, src);
		if (!ok) return false;
		anim.push_back(std::move(cis));
	}

	return true;
}

bool alib::BasicAnim::LoadPacked(UC dep, decompress_bypass_source& src)
{
	//src.reset();
	if (!src.have(10)) return false;
	delay = src.get(10);
	if (!src.have(10)) return false;
	repeating = src.get(1);
	jbf = src.get(9);
	if (!src.have(10)) return false;
	UL val = src.get(10);
	anim.resize(val);
	//int ii = 1;
	for (CIS& cis : anim)
	{
		bool ok = cis.LoadPacked(dep, src);
		if (!ok) return false;
		//cis.SaveBMP( ("gfx/img_"s + std::to_string(ii++) + ".bmp"s).c_str(), 55, {255,0,255});
	}
	return true;
}


void alib::BasicAnim::SBT(UC dep, bittarget& trg) const
{
	trg.put(delay, dep*2);
	UL val = repeating ? (1<<(dep*2-1)) : 0;
	val |= jbf;
	trg.put(val, dep*2);
	val = (UL)anim.size();
	trg.put(val, dep*2);
	for (const CIS& cis : anim)
	{
		cis.SBT(dep, trg);
	}
}

void alib::BasicAnim::SavePacked(UC dep, compress_bypass_target& trg) const
{
	//trg.reset();
	trg.bypass(delay, 10);
	trg.bypass(repeating, 1);
	trg.bypass(jbf, 9);
	UL val = (UL)anim.size();
	trg.bypass(val, 10);
	for (const CIS& cis : anim)
	{
		cis.SavePacked(dep, trg);
	}
}

void alib::BasicAnim::MakeMirror(BasicAnim& ba, bool mx, bool my, bool rot)
{
	delay = ba.delay;
	repeating = ba.repeating;
	jbf = ba.jbf;
	anim.clear();
	for (CIS& cis : ba.anim)
		anim.push_back(cis.Flip(mx, my, rot));
}

#ifndef NO_SFML
void alib::BasicAnim::Instance(UC hue)
{
	for (CIS& cis : anim)
		cis.Instance(hue);
}
#endif

void alib::BasicAnim::UnloadBase()
{
	for (CIS& cis : anim)
		cis.UnloadBase();
}

#ifndef NO_SFML
auto alib::BasicAnim::Refl(UC hue) -> AnimReflection
{
	AnimReflection ar(this, hue);
	return ar;
}

sf::Texture& alib::BasicAnim::Get(int frame, UC hue)
{
	return anim[frame].Get(hue);
}
#endif

// ----------------------------------------------------------------------------

// ***************
// *** AnimDir ***
// ***************

#ifndef NO_SFML
void alib::AnimDir::Instance(UC hue)
{
	for (BAD& b : bad)
		b.Instance(hue);
}
#endif

void alib::AnimDir::UnloadBase()
{
	for (BAD& b : bad)
		b.UnloadBase();
}

void alib::AnimDir::Load(std::istream& is)
{
	char buff[5] = {};
	is.read(buff, 4);
	std::string magic = buff;
	boost::to_lower(magic);
	if (magic == "adp1")
	{
		streamsource ss{is};
		decompress_bypass_source dbs{6, ss};
		LoadPacked(6, dbs);
	}
	else if (magic == "ad_2")
	{
		LoadInternal(is);
	}
	else if (magic == "ad_3")
	{
		streamsource ss{is};
		LBS(6, ss);
	}
	else
	{
		is.putback(buff[3]).putback(buff[2]).putback(buff[1]).putback(buff[0]);
		LoadOld(is);
	}
}

void alib::AnimDir::LoadOld(std::istream& is) { LoadInternal(is, true); }

void alib::AnimDir::LoadInternal(std::istream& is, bool old)
{
	short i, n;
	ReadBinary(is, n);
	bad.clear();
	bad.reserve(n);
	for (i = 0; i < n; ++i)
	{
		bad.push_back(BAD());
		BAD& b = bad.back();
		ReadBinary(is, b.dir);
		UC uc;
		ReadBinary(is, uc);
		if (uc)
		{
			b.mirror = true;
			b.flipx = uc & 2;
			b.flipy = uc & 4;
			b.rot90 = uc & 8;
			ReadBinary(is, b.mirrorof);
		}
		else {
			if (old)
				b.LoadOld(is);
			else
				b.Load(is);
			b.mirror = false;
		}
	}
	Mirror();
}

bool alib::AnimDir::LBS(UC dep, bitsource& src)
{
	short i, n;
	if (!src.have(dep*2)) return false;
	n = (short)src.get(dep*2);
	bad.clear();
	bad.reserve(n);
	for (i = 0; i < n; ++i)
	{
		bad.emplace_back();
		BAD& b = bad.back();
		if (!src.have(dep*3)) return false;
		auto d = dep;
		while (d<9) d += dep;
		if (!src.have(d+dep)) return false;
		b.dir = (short)src.get(d);
		UC uc = (UC)src.get(dep);
		if (uc)
		{
			b.mirror = true;
			b.flipx = uc & 2;
			b.flipy = uc & 4;
			b.rot90 = uc & 8;
			if (!src.have(dep * 2)) return false;
			b.mirrorof = (short)src.get(d);
		}
		else {
			bool ok = b.LBS(dep, src);
			if (!ok) return false;
			b.mirror = false;
		}
	}
	Mirror();

	return true;
}

void alib::AnimDir::SBT(UC dep, bittarget& trg) const
{
	UL i, n = (UL)bad.size();
	trg.put(n, dep*2);
	for (i = 0; i < n; ++i)
	{
		const BAD& b = bad[i];
		auto d = dep;
		while (d < 9) d += dep;
		trg.put(b.dir, d);
		UC uc = 0;
		if (b.mirror)
		{
			uc = 1;
			if (b.flipx) uc |= 2;
			if (b.flipy) uc |= 4;
			if (b.rot90) uc |= 8;
			trg.put(uc, dep);
			trg.put(b.mirrorof, d);
		}
		else {
			trg.put(0, dep);
			b.SBT(dep, trg);
		}
	}
}

void alib::AnimDir::SavePacked(UC dep, compress_bypass_target& cbt) const
{
	UL i, n = (UL)bad.size();
	cbt.bypass(n, 10);
	for (i = 0; i < n; ++i)
	{
		const BAD& b = bad[i];
		cbt.bypass(b.dir, 9);
		UC uc = 0;
		if (b.mirror)
		{
			uc = 1;
			if (b.flipx) uc |= 2;
			if (b.flipy) uc |= 4;
			if (b.rot90) uc |= 8;
			cbt.bypass(uc, 4);
			cbt.bypass(b.mirrorof, 9);
		}
		else {
			cbt.bypass(0, 4);
			b.SavePacked(dep, cbt);
		}
	}
}

bool alib::AnimDir::LoadPacked(UC dep, decompress_bypass_source& src)
{
	short i, n;
	if (!src.have(10)) return false;
	n = (short)src.get(10);
	bad.clear();
	bad.reserve(n);
	for (i = 0; i < n; ++i)
	{
		bad.emplace_back();
		BAD& b = bad.back();
		if (!src.have(4+9)) return false;
		b.dir = (short)src.get(9);
		UC uc = (UC)src.get(4);
		if (uc)
		{
			b.mirror = true;
			b.flipx = uc & 2;
			b.flipy = uc & 4;
			b.rot90 = uc & 8;
			if (!src.have(9)) return false;
			b.mirrorof = (short)src.get(9);
		}
		else {
			bool ok = b.LoadPacked(dep, src);
			if (!ok) return false;
			b.mirror = false;
		}
	}
	Mirror();

	return true;
}

void alib::AnimDir::CreateDir(short dir, bool repeating, short delay, short jbf)
{
	if (findexact(dir)) return;
	bad.emplace_back();
	BAD& d = bad.back();
	d.dir = dir; d.mirrorof = dir;
	d.repeating = repeating;
	d.mirror = d.flipx = d.flipy = d.rot90 = false;
	d.delay = delay;
	d.jbf = jbf;
}

void alib::AnimDir::AddFrameTo(short dir, CIS&& cis)
{
	BAD* d = findexact(dir);
	assert(d);
	d->anim.push_back( std::move(cis) );
}

void alib::AnimDir::Clear()
{
	bad.clear();
}

auto alib::AnimDir::findexact(short d) -> AnimDir::BAD*
{
	for (BAD& bd : bad)
	{
		if (bd.dir == d) return &bd;
	}
	return 0;
}

#ifndef NO_SFML
alib::AnimReflection alib::AnimDir::Refl(short dir, UC hue)
{
	return Closest(dir).Refl(hue);
}
#endif

template<typename T, typename U>
T& BestFit(short dir, U& vec)
{
	auto dirdiff = [](short d1, short d2) -> short
	{
		short df = std::abs(d1 - d2);
		if (df > 180) df = 360 - 180;
		return df;
	};

	int idx = 0;
	int n = (int)vec.size();
	assert(n);
	int srt = dirdiff(dir, vec[0].dir);
	for (int i = 1; i < n; ++i)
	{
		int s = dirdiff(dir, vec[i].dir);
		if (s < srt)
		{
			srt = s;
			idx = i;
		}
	}
	return vec[idx];
}

alib::BasicAnim& alib::AnimDir::Closest(short dir)
{
	return BestFit<BasicAnim>(dir, bad);
}

void alib::AnimDir::Mirror()
{
	for (BAD& a : bad)
		if (a.mirror)
			a.MakeMirror(Closest(a.mirrorof), a.flipx, a.flipy, a.rot90);
}

// ----------------------------------------------------------------------------

// ***********
// *** NAV ***
// ***********

extern std::string ExtractFileNameOnly(std::string fn);
/*{
	auto p1 = fn.find_last_of("/\\");
	if (p1 == std::string::npos) p1 = 0;
	auto p2 = fn.find_last_of(".");
	if (p2 < p1) p2 = std::string::npos;
	if (p2 == std::string::npos)
		return fn.substr(p1 + 1);
	else
		return fn.substr(p1 + 1, p2 - p1);
}*/

#ifndef NO_SFML
void alib::NAV::Instance(UC hue)
{
	for (AnimDir& ad : variants)
		ad.Instance(hue);
}
#endif

void alib::NAV::UnloadBase()
{
	for (AnimDir& ad : variants)
		ad.UnloadBase();
}

static std::string LoadStr(std::istream& is)
{
	std::string s;
	while (true)
	{
		char c;
		ReadBinary(is, c);
		if (!c) break;
		s += c;
	}
	return s;
}

// static void SaveStr(std::ostream& os, std::string str)
// {
// 	os.write(str.c_str(), str.size() + 1);
// }

void alib::NAV::LoadInternal(std::istream& is, bool old)
{
	name = LoadStr(is);
	char vars;
	ReadBinary(is, vars);
	variants.clear();
	for (int i = 0; i < vars; ++i)
	{
		variants.emplace_back();
		if (old)
			variants.back().LoadOld(is);
		else
			variants.back().Load(is);
	}
}

static char table[/*1<<6*/] = 
{
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'_', '-', '+', '.', ',', ':', ';', '*', '!', '@', '"', '\'', '/', '\\', '(', ')', '=', '[', ']', '{', '}', '&', '%', '#', '$', '|', '~', '?'
};

void string_out_6(const std::string& str, bittarget& trg)
{
	UC len = (UC)std::min<size_t>( str.length(), 63 );
	trg.put(len, 6);
	for (int idx = 0; idx<len; ++idx)
	{
		UC tok;
		for(tok=0; tok<63; ++tok)
		{
			if (table[tok] == str[idx])
				break;
		}
		trg.put(tok, 6);
	}
}

std::string string_in_6(bitsource& src)
{
	if (!src.have(6)) return ""s;
	UC len = (UC)src.get(6);
	std::string ret;
	ret.reserve(len);
	for (int idx = 0; idx < len; ++idx)
	{
		if (!src.have(6)) break;
		UC tok = (UC)src.get(6);
		ret += table[tok];
	}
	return ret;
}

void string_out_8(const std::string& str, bittarget& trg)
{
	//auto len = str.length();
	for (char c : str)
	{
		assert(c);
		trg.put(c,8);
	}
	trg.put(0,8);
}

std::string string_in_8(bitsource& src)
{
	std::string ret;
	while (true)
	{
		char c = (char)src.get(8);
		if (!c) break;
		ret += c;
	}
	return ret;
}

bool alib::NAV::LBS(UC dep, bitsource& src)
{
	name = ((dep==6)?&string_in_6:&string_in_8)(src);
	if (!src.have(dep)) return false;
	UL vars = src.get(dep);
	variants.clear();
	for (UL i = 0; i < vars; ++i)
	{
		variants.emplace_back();
		bool ok = variants.back().LBS(dep, src);
		if (!ok) return false;
	}
	return true;
}

void alib::NAV::SBT(UC dep, bittarget& trg) const
{
	((dep==6)?&string_out_6:&string_out_8)(name, trg);
	UL vars = (UL)variants.size();
	trg.put(vars, dep);
	for (const auto& v : variants)
		v.SBT(dep, trg);
}

void alib::NAV::LoadOld(std::istream& is) { LoadInternal(is, true); }

void alib::NAV::Load(std::istream& is)
{
	char buff[5] = {};
	is.read(buff, 4);
	std::string magic = buff;
	if (magic == "NAV2")
	{
		LoadInternal(is);
	}
	else if (boost::to_lower_copy(magic) == "nav3")
	{
		streamsource ss{is};
		LBS(6, ss);
	}
	else
	{
		is.putback(buff[3]).putback(buff[2]).putback(buff[1]).putback(buff[0]);
		LoadOld(is);
	}
}

#ifndef NO_SFML
auto alib::NAV::Refl(short dir, UC hue) -> AnimReflection
{
	int i = rand() % variants.size();
	AnimReflection ar(&variants[i].Closest(dir), hue);
	return ar;
}
#endif

void alib::NAV::SavePacked(UC dep, compress_bypass_target& cbt) const
{
	bitchannel btc;
	string_out_6(name, btc);
	while (true)
	{
		auto sz = btc.bitcount();
		if (!sz) break;
		if (sz >= 24)
		{
			UL bits = btc.get(24);
			cbt.bypass(bits, 24);
		} else {
			UL bits = btc.get(sz);
			cbt.bypass(bits, sz);
		}
	}

	UL vars = (UL)variants.size();
	cbt.bypass(vars, 5);
	for (const auto& v : variants)
		v.SavePacked(dep, cbt);
}

bool alib::NAV::LoadPacked(UC dep, decompress_bypass_source& src)
{
	name = string_in_6(src);
	if (!src.have(5)) return false;
	UL vars = src.get(5);
	variants.clear();
	for (UL i = 0; i < vars; ++i)
	{
		variants.emplace_back();
		auto& a = variants.back();
		bool ok = a.LoadPacked(dep, src);
		if (!ok) return false;
	}
	return true;
}

// ----------------------------------------------------------------------------

// **********************
// *** AnimCollection ***
// **********************

#ifndef NO_SFML
void alib::AnimCollection::Instance(UC hue)
{
	for (NAV& nav : core)
		nav.Instance(hue);
}

auto alib::AnimCollection::Refl(std::string name, short dir, UC hue) -> AnimReflection
{
	//boost::trim(name);
	auto itr = mappings.find(name);
	auto end = mappings.end();
	if ((itr == end) && default_anim.size())
		itr = mappings.find(default_anim);
	if (itr == end)
		itr = mappings.find("default");
	if (itr == end)
		itr = mappings.find("idle");
	if (itr == end)
		itr = mappings.begin();
	return itr->second->Refl(dir, hue);
}
#endif

void alib::AnimCollection::UnloadBase()
{
	for (NAV& nav : core)
		nav.UnloadBase();
}

void alib::AnimCollection::AddVariant(std::string name, AnimDir ad)
{
	auto itr = mappings.find(name);
	if (itr == mappings.end())
	{
		core.emplace_back();
		NAV* nav = &core.back();
		mappings[name] = nav;
		nav->name = std::move(name);
		nav->variants.clear();
		nav->variants.push_back(std::move(ad));
	}
	else {
		itr->second->variants.push_back(std::move(ad));
	}
}

void alib::AnimCollection::LoadInternal(std::istream& is, bool old)
{
	core.clear();
	mappings.clear();
	char n;
	ReadBinary(is, n);
	//core.reserve(n);
	for (int i = 0; i < n; ++i)
	{
		core.emplace_back();
		NAV* nav = &core.back();
		if (old)
			nav->LoadOld(is);
		else
			nav->Load(is);
		mappings[nav->name] = nav;
	}
	ReadBinary(is, n);
	for (int i = 0; i < n; ++i)
	{
		std::string name = LoadStr(is);
		std::string mapof = LoadStr(is);
		auto itr = mappings.find(mapof);
		assert(itr != mappings.end());
		mappings[name] = itr->second;
	}
}

bool alib::AnimCollection::LBS(UC dep, bitsource& src)
{
	core.clear();
	mappings.clear();
	UL n;
	if (!src.have(dep)) return false;
	n = src.get(dep);
	//core.reserve(n);
	for (UL i = 0; i < n; ++i)
	{
		core.emplace_back();
		NAV* nav = &core.back();
		nav->LBS(dep,src);
		mappings[nav->name] = nav;
	}

	return true;
}

void alib::AnimCollection::SBT(UC dep, bittarget& trg) const
{
	UL n = (UL)core.size();
	trg.put(n, dep);
	for (const auto& nav : core)
	{
		nav.SBT(dep, trg);
	}
}

void alib::AnimCollection::SavePacked(UC dep, compress_bypass_target& cbt) const
{
	UL n = (UL)core.size();
	cbt.bypass(n, 5);
	for (const auto& nav : core)
	{
		nav.SavePacked(dep, cbt);
	}
}

bool alib::AnimCollection::LoadPacked(UC dep, decompress_bypass_source& src)
{
	core.clear();
	mappings.clear();
	UL n;
	if (!src.have(5)) return false;
	n = src.get(5);
	//core.reserve(n);
	for (UL i = 0; i < n; ++i)
	{
		core.emplace_back();
		NAV* nav = &core.back();
		bool ok = nav->LoadPacked(dep, src);
		if (!ok) return false;
		mappings[nav->name] = nav;
	}

	return true;
}

void alib::AnimCollection::LoadOld(std::istream& is) { LoadInternal(is, true); }

void alib::AnimCollection::Load(std::istream& is)
{
	char buff[5] = {};
	is.read(buff, 4);
	std::string magic = buff;
	boost::to_lower(magic);
	if (magic == "acp1")
	{
		streamsource ss{is};
		decompress_bypass_source dbs{6, ss};
		LoadPacked(6,dbs);
		//auto dbg = dbs.mapx.size();
		//std::cout << dbg << std::endl;
	}
	else if (magic == "ac_2")
	{
		LoadInternal(is);
		default_anim = LoadStr(is);
	}
	else if (magic == "ac_3")
	{
		streamsource ss{is};
		LBS(6, ss);
	}
	else
	{
		is.putback(buff[3]).putback(buff[2]).putback(buff[1]).putback(buff[0]);
		LoadOld(is);
		default_anim.clear();
	}
}

std::vector<std::string> alib::AnimCollection::CoreNames() const
{
	std::vector<std::string> sl;
	for (auto& nav : core)
	{
		sl.push_back(nav.name);
	}
	return sl;
}

std::vector<std::string> alib::AnimCollection::AllNames() const
{
	std::vector<std::string> sl;
	for (auto itr : mappings)
	{
		sl.push_back(itr.first);
	}
	return sl;
}

// ----------------------------------------------------------------------------

#ifndef NO_SFML

// **********************
// *** AnimReflection ***
// **********************

void alib::AnimReflection::draw(sf::RenderTarget& rt, sf::RenderStates st) const
{
	auto tr = this->getTransform();
	sf::Sprite spr;
	Pos hot;
	if (ba)
	{
		auto& tex = ba->Get(current, hue);
		hot = ba->Hot(current);
		spr.setTexture(tex);
	}
	else if (cis)
	{
		auto& tex = cis->Get(hue);
		hot = cis->Hot();
		spr.setTexture(tex);
	}
	spr.setOrigin(hot.x, hot.y);
	st.transform *= tr;
	rt.draw(spr,st);
}

bool alib::AnimReflection::Next()
{
	if (!ba) return false;
	time = 0;
	if (++current > ba->Size())
	{
		current = 0;
		if (!ba->repeating) return false;
	}
	return true;
}

static sf::Clock global_clock;

sf::Clock& alib::GlobalClock()
{
	return global_clock;
}

void alib::AnimReflection::Start()
{
	time = 0;
	last = global_clock.getElapsedTime().asMilliseconds();
}

bool alib::AnimReflection::Update()
{
	int ii = global_clock.getElapsedTime().asMilliseconds();
	bool ok = Update(ii - last);
	last = ii;
	return ok;
}

bool alib::AnimReflection::Update(int ms)
{
	if ((!ba) || (!ba->delay)) return false;
	time += ms;
	while (time >= ba->delay)
	{
		++current;
		time -= ba->delay;
	}
	if (current >= ba->Size())
	{
		loopcnt += (current / ba->Size());
		current %= ba->Size();
		if (!ba->repeating) return false;
	}
	return true;
}

void alib::AnimReflection::Set(BasicAnim* b, UC h)
{
	clr(); ba = b; hue = h;
}

void alib::AnimReflection::Set(CIS* c, UC h)
{
	clr(); cis = c; hue = h;
}

alib::AnimReflection::AnimReflection() { clr(); }
alib::AnimReflection::AnimReflection(BasicAnim* b, UC h) { clr(); Set(b, h); }
alib::AnimReflection::AnimReflection(CIS* c, UC h) { clr(); Set(c, h); }

void alib::AnimReflection::clr()
{
	cis = 0; ba = 0;
	current = time = last = loopcnt = 0;
	hue = 0;
}

#endif


// ----------------------------------------------------------------------------

// ***************
// *** Commons ***
// ***************

void alib::CIS::Load(const std::string& fn) { std::ifstream ifs{fn, std::fstream::binary}; Load(ifs); }
void alib::BA:: Load(const std::string& fn) { std::ifstream ifs{fn, std::fstream::binary}; Load(ifs); }
void alib::AD:: Load(const std::string& fn) { std::ifstream ifs{fn, std::fstream::binary}; Load(ifs); }
void alib::AC:: Load(const std::string& fn) { std::ifstream ifs{fn, std::fstream::binary}; Load(ifs); }


