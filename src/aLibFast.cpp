
#include <cassert>
#include <utility>
#include <algorithm>
#include <vector>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <experimental/filesystem>
#include <sstream>
#include <map>
#include <cstring>

// #pragma hdrstop

#include "alib.hpp"

namespace fs = std::experimental::filesystem;

template<typename T>
bool read(UC*& data, UC* end, T& val)
{
	if ((data + sizeof(T)) > end) return false;
	val = *(T*)data;
	data += sizeof(T);
	return true;
}

template<typename T>
void write(std::ostream& out, UL& written, const T& val)
{
	char* ptr = (char*)&val;
	UL n = sizeof(T);
	written += n;
	out.write(ptr, n);
}

bool alib::CIS::LoadFast(UC*& data, UC* end)
{
	bool ok = true;

	ok = ok && read(data, end, w);
	ok = ok && read(data, end, h);
	ok = ok && read(data, end, hot.x);
	ok = ok && read(data, end, hot.y);

	has_dither = has_trans = has_colimp = loaded = false;
	depth = 8;

	UL sz = w*h;
	pixeltypes.resize(sz);

	UL ii = 0, nn = 0;
	while (ok && (ii < sz))
	{
		UC xx = 0;
		ok = ok && read(data, end, xx);
		nn = (xx >> 2) + 1;
		PixelType pt = PixelType(xx & 3);
		while (nn--)
		{
			if (ii >= sz) break;
			pixeltypes[ii++] = pt;
		}
	}
	assert( (ii==sz) && (++nn==0) );

	UC uc;
	pixels.resize(sz);
	for (ii = 0; ok && (ii < sz); ++ii)
	{
		auto& pix = pixels[ii];
		pix = {0,0,0,255};
		switch (pixeltypes[ii])
		{
		case normal:
			pix.a = 255;
			ok = ok && read(data, end, pix.h);
			ok = ok && read(data, end, pix.s);
			ok = ok && read(data, end, pix.v);
			break;
		case trans:
			has_trans = true;
			pix = {0,0,0,0};
			break;
		case alpha:
			has_dither = true;
			ok = ok && read(data, end, uc);
			pix = {0,0,uc,128};
			break;
		case colimp:
			has_colimp = true;
			pix.a = 255;
			pix.h = 0;
			ok = ok && read(data, end, pix.s);
			ok = ok && read(data, end, pix.v);
			break;
		default:
			assert(false && "pixeltype error");
		}
	}
	return loaded = ok;
}

UL alib::CIS::SaveFast(std::ostream& out) const
{
	assert(loaded);

	UL pos = 0;

	write(out, pos, w);
	write(out, pos, h);
	write(out, pos, hot.x);
	write(out, pos, hot.y);
	
	UL sz = w*h;
	assert (pixeltypes.size() == sz);

	auto push = [&](PixelType pt, UL cnt) -> void
	{
		assert(cnt);
		while (cnt >= 64)
		{
			UC uc = (63 << 2) | (pt&3);
			write(out, pos, uc);
			cnt -= 64;
		}
		if (cnt)
		{
			UC uc = (--cnt << 2) | (pt&3);
			write(out, pos, uc);
		}
	};

	bool first = true;
	PixelType pt;
	UL cnt, reserve = 0;
	// part one, the pixtype component
	for (const auto& itm : pixeltypes)
	{
		switch (itm)
		{
		case normal:
			reserve += 3;
			break;
		case trans:
			break;
		case alpha:
			reserve += 1;
			break;
		case colimp:
			reserve += 2;
			break;
		default:
			assert(false && "pixeltype error");
		}
		if (first)
		{
			pt = itm;
			cnt = 1;
			first = false;
		}
		else if (itm != pt)
		{
			if (cnt) push(pt, cnt);
			pt = itm; cnt = 1;
		}
		else
		{
			++cnt;
		}
	}
	if (cnt) push(pt, cnt);

	for (UL ii = 0; ii < sz; ++ii)
	{
		const auto& pix = pixels[ii];
		switch (pixeltypes[ii])
		{
		case normal:
			assert(pix.a == 255);
			write(out, pos, pix.h);
			write(out, pos, pix.s);
			write(out, pos, pix.v);
			break;
		case trans:
			assert(has_trans);
			assert((pix == HSVA{0,0,0,0}));
			break;
		case alpha:
			assert(has_dither);
			write(out, pos, pix.v);
			assert(pix.s == 0);
			assert(pix.a == 128);
			break;
		case colimp:
			assert(has_colimp);
			assert(pix.a == 255);
			write(out, pos, pix.s);
			write(out, pos, pix.v);
			break;
		default:
			assert(false && "pixeltype error");
		}
	}

	return pos;
}

// ----------------------------------------------------------------------------

bool alib::BA::LoadFast(UC*& beg, UC* end)
{
	bool ok = true;
	ok = ok && read(beg, end, delay);
	UC uc = 0;
	ok = ok && read(beg, end, uc);
	repeating = (bool)uc;
	ok = ok && read(beg, end, jbf);
	UL sz;
	ok = ok && read(beg, end, sz);
	if (!ok) return false;
	anim.resize(sz);
	for (auto&& a : anim)
	{
		ok = ok && a.LoadFast(beg, end);
	}
	return ok;
}

UL alib::BA::SaveFast(std::ostream& out) const
{
	UL written = 0;
	write(out, written, delay);
	UC uc = repeating;
	write(out, written, uc);
	write(out, written, jbf);
	UL sz = (UL)anim.size();
	write(out, written, sz);
	for (auto&& a : anim)
		written += a.SaveFast(out);
	return written;
}

// ----------------------------------------------------------------------------

bool alib::AD::LoadFast(UC*& beg, UC* end)
{
	bool ok = true;
	UL i, sz;
	ok = ok && read(beg, end, sz);
	if (!ok) return false;
	bad.resize(sz);
	for (i = 0; ok && (i < sz); ++i)
	{
		BAD& b = bad[i];
		ok = ok && read(beg, end, b.dir);
		UC mask = 0;
		ok = ok && read(beg, end, mask);
		b.mirror = mask & 1;
		if (b.mirror)
		{
			b.flipx = (mask >> 1) & 1;
			b.flipy = (mask >> 2) & 1;
			b.rot90 = (mask >> 3) & 1;
			ok = ok && read(beg, end, b.mirrorof);
		} else {
			ok = ok && b.LoadFast(beg, end);
		}
	}
	if (ok) Mirror();
	return ok;
}

UL alib::AD::SaveFast(std::ostream& out) const
{
	UL written = 0;
	UL i, sz = (UL)bad.size();
	write(out, written, sz);
	for (i = 0; i < sz; ++i)
	{
		const BAD& b = bad[i];
		write(out, written, b.dir);
		UC mask;
		if (b.mirror)
		{
			mask = 1;
			if (b.flipx) mask |= 1 << 1;
			if (b.flipy) mask |= 1 << 2;
			if (b.rot90) mask |= 1 << 3;
			write(out, written, mask);
			write(out, written, b.mirrorof);
		} else {
			mask = 0;
			write(out, written, mask);
			written += b.SaveFast(out);
		}
	}
	return written;
}

// ----------------------------------------------------------------------------

#pragma pack(push, 4)
typedef struct FastFileHeader
{
	UL magic;
	UL filesize;
	UL subtype;
	UL depth;
	UL ofs_tab_names;
	UL ofs_tab_pairs;
	UL ofs_tab_ads;
} FFH;


#pragma pack(pop)

enum ALIB_FFH { ST_AC = 1, ST_AD, ST_BA, ST_CIS };

struct NameBuff
{
	UL AddName(const std::string& sv)
	{
		auto itr = map_names.find(sv);
		if (itr!=map_names.end())
			return itr->second;
		UL ii = (UL)vec_names.size();
		vec_names.push_back(sv);
		map_names[sv]=ii;
		return ii;
	}
	UL Lookup(const std::string& sv)
	{
		auto itr = map_names.find(sv);
		if (itr != map_names.end())
			return itr->second;
		throw "error";
	}
	std::string Lookup(UL id)
	{
		if (id >= vec_names.size())
			throw "error";
		return vec_names[id];
	}

	auto begin() const { return map_names.cbegin(); }
	auto end() const { return map_names.cend(); }

	UL write(std::ostream& out);
	void read(std::istream& in);
	UL write_size() const;
	void clear() { vec_names.clear(); map_names.clear(); }
	bool empty() const { return vec_names.empty(); }
private:
	std::vector<std::string> vec_names;
	std::map<std::string, int> map_names;
};

UL NameBuff::write(std::ostream& out)
{
	UL wr = 0, sz = (UL)vec_names.size();
	::write(out, wr, sz);
	for (auto&& str : vec_names)
	{
		char buff[12] = {0};
		std::strncpy(buff, str.c_str(), 12);
		out.write(buff, 12);
	}
	return wr;
}

void NameBuff::read(std::istream& in)
{
	clear();
	UL i, sz;
	in.read((char*)&sz, sizeof(UL));
	for (i = 0; i < sz; ++i)
	{
		char buff[13] = {0};
		in.read(buff, 12);
		std::string s = buff;
		vec_names.push_back(s);
		map_names[s] = i;
	}
}

UL NameBuff::write_size() const
{
	return (UL)vec_names.size()*12 + 4;
}

/*
std::list<NAV> core;
std::map<std::string, NAV*> mappings;
std::string default_anim;
*/

using CADP = const alib::AD*;

UL alib::AC::SaveFast(std::ostream& ofs) const
{
	FFH ffh;
	ffh.magic = 'alib';
	ffh.filesize = 0;
	ffh.subtype = ST_AC;
	ffh.depth = 8;

	UL hdr_sz = sizeof(FFH);

	std::vector<std::string> str;
	for (auto&& n : core)
		str.push_back(n.name);
	for (auto&& n : mappings)
		str.push_back(n.second->name);
	auto beg = str.begin(), end = str.end();
	auto itr = std::find(beg, end, default_anim);
	if (itr != end && itr != beg)
		std::swap(*beg, *itr);
	NameBuff nb;
	for (auto&& s : str)
		nb.AddName(s);

	std::vector<CADP> vad;
	std::vector<std::pair<UL, UL>> pairs;

	for (auto&& nav : core)
	{
		UL name_id = nb.Lookup(nav.name);
		UL i, n = nav.Count();
		for (i = 0; i < n; ++i)
		{
			const AD& ad = nav.Get(i);
			UL anim_id = (UL)vad.size();
			vad.push_back(&ad);
			auto p = std::make_pair(name_id, anim_id);
			if (std::count(pairs.begin(), pairs.end(), p) == 0)
				pairs.push_back(std::move(p));
		}
	}

	auto fnd_ad = [&](CADP p) -> UL
	{
		auto beg = vad.begin(), end = vad.end();
		auto itr = std::find(beg, end, p);
		assert (itr != end);
		return UL(itr - beg);
	};

	for (auto&& m : mappings)
	{
		UL name_id = nb.Lookup(m.first);
		NAV* n = m.second;
		const AD& ad = n->Get(0);
		UL anim_id = fnd_ad(&ad);
		auto p = std::make_pair(name_id, anim_id);
		if (std::count(pairs.begin(), pairs.end(), p) == 0)
			pairs.push_back(std::move(p));
	}

	UL nam_sz = nb.write_size();
	UL pair_sz = (UL)pairs.size() * 8 + 4;

	ffh.ofs_tab_names = hdr_sz;
	ffh.ofs_tab_pairs = hdr_sz + nam_sz;
	ffh.ofs_tab_ads = hdr_sz + nam_sz + pair_sz;

	ofs.write((char*)&ffh, hdr_sz);
	nb.write(ofs);
	UL psz = (UL)pairs.size();
	ofs.write((char*)&psz, 4);
	for (auto&& p : pairs)
	{
		ofs.write((char*)&p.first, 4);
		ofs.write((char*)&p.second, 4);
	}

	UL wr = 0;
	for (auto&& ad : vad)
	{
		wr += ad->SaveFast(ofs);
	}
	[[maybe_unused]] auto fsz = ofs.tellp();
	ofs.seekp(4);
	ffh.filesize = hdr_sz + nam_sz + pair_sz + wr;
	assert(ffh.filesize == +fsz);
	ofs.write((char*)&ffh.filesize, 4);

	return ffh.filesize;
}

UL alib::AC::SaveFast(BVec& vec) const
{
	std::stringstream ss;
	SaveFast(ss);
	auto&& str = ss.str();
	vec.assign(str.begin(), str.end());
	return (UL)vec.size();
}

UL alib::AC::SaveFast(const std::string& fn) const
{
	std::ofstream ofs{fn, std::fstream::binary};
	return SaveFast(ofs);
}

bool alib::AC::LoadFast(std::istream& ifs)
{
	FFH ffh;
	BVec vec;
	NameBuff nb;
	std::vector<AD*> vad;
	std::vector<std::pair<UL, UL>> pairs;

	ifs.read((char*)&ffh, sizeof(FFH));
	auto fsz = ffh.filesize;
	if (ffh.magic != 'alib') return false;
	if (ffh.subtype != ST_AC) return false;

	vec.resize(fsz);
	ifs.seekg(0);
	ifs.read((char*)vec.data(), fsz);

	ifs.seekg(ffh.ofs_tab_names);
	nb.read(ifs);

	UC* data = vec.data() + ffh.ofs_tab_pairs;
	UC* end = vec.data() + vec.size();
	UL i, n, maxa = 0;
	::read(data, end, n);
	for (i = 0; i < n; ++i)
	{
		UL id_n, id_a;
		::read(data, end, id_n);
		::read(data, end, id_a);
		if (id_a > maxa) maxa = id_a;
		auto p = std::make_pair(id_n, id_a);
		if (std::count(pairs.begin(), pairs.end(), p) == 0)
			pairs.push_back(std::move(p));
	}

	core.clear(); mappings.clear();
	if (nb.empty()) return false;

	default_anim = nb.Lookup(0);

	data = vec.data() + ffh.ofs_tab_ads;

	bool ok = true;
	for (i = 0; ok && (i < n); ++i)
	{
		AD ad;
		ok = ok && ad.LoadFast(data, end);
		std::vector<UL> names;
		for (auto&& p : pairs)
		{
			if (p.second == i)
				names.push_back(p.first);
		}
		ok = ok && !names.empty();
		if (ok)
			AddVariant(nb.Lookup(names.front()), std::move(ad));
	}

	return ok;
}

bool alib::AC::LoadFast(BVec& bv)
{
	std::stringstream ss;
	ss.write((char*)bv.data(), bv.size());
	ss.seekg(0);
	return LoadFast(ss);
}

bool alib::AC::LoadFast(const std::string& fn)
{
	FFH ffh;
	BVec vec;
	NameBuff nb;
	std::vector<AD*> vad;
	std::vector<std::pair<UL, UL>> pairs;

	{
		fs::path pth = fn;
		if (!fs::exists(pth)) return false;
		UL fsz = (UL)fs::file_size(pth);
		if (fsz < sizeof(FFH)) return false;
		std::ifstream ifs{fn, std::fstream::binary};
		ifs.read((char*)&ffh, sizeof(FFH));
		if (ffh.magic != 'alib') return false;
		if (ffh.filesize != fsz) return false;
		if (ffh.subtype != ST_AC) return false;

		vec.resize(fsz);
		ifs.seekg(0);
		ifs.read((char*)vec.data(), fsz);

		ifs.seekg(ffh.ofs_tab_names);
		nb.read(ifs);
	}

	UC* data = vec.data() + ffh.ofs_tab_pairs;
	UC* end = vec.data() + vec.size();
	UL i, n, maxa=0;
	::read(data, end, n);
	for (i = 0; i < n; ++i)
	{
		UL id_n, id_a;
		::read(data, end, id_n);
		::read(data, end, id_a);
		if (id_a>maxa) maxa = id_a;
		auto p = std::make_pair(id_n, id_a);
		if (std::count(pairs.begin(), pairs.end(), p) == 0)
			pairs.push_back(std::move(p));
	}

	core.clear(); mappings.clear();
	if (nb.empty()) return false;

	default_anim = nb.Lookup(0);

	data = vec.data() + ffh.ofs_tab_ads;

	bool ok = true;
	for (i = 0; ok && (i < n); ++i)
	{
		AD ad;
		ok = ok && ad.LoadFast(data, end);
		std::vector<UL> names;
		for (auto&& p : pairs)
		{
			if (p.second==i)
				names.push_back(p.first);
		}
		ok = ok && !names.empty();
		if (ok)
			AddVariant(nb.Lookup(names.front()), std::move(ad));
	}

	return ok;
}


#include <chrono>

namespace chr = std::chrono;

void speed_test()
{
	constexpr int N = 5;

	alib::AC ac;
	std::string fn = "C:/tmp/au/Flak";

	auto t1 = chr::high_resolution_clock::now();
	for (int i = 0; i < N; ++i)
	{
		ac.Load(fn+".ac");
	}
	auto t2 = chr::high_resolution_clock::now();
	for (int i = 0; i < N; ++i)
	{
		ac.LoadFast(fn+".fac");
	}
	auto t3 = chr::high_resolution_clock::now();
	for (int i = 0; i < N; ++i)
	{
		std::ifstream ifs{fn + ".pac", std::fstream::binary};
		ifs.seekg(4);
		streamsource ss{ifs};
		decompress_bypass_source dbs{6, ss};
		ac.LoadPacked(6, dbs);
	}
	auto t4 = chr::high_resolution_clock::now();

	std::cout << "AC    : " << chr::duration_cast<chr::milliseconds>(t2 - t1).count() << " ms\n";
	std::cout << "FAC   : " << chr::duration_cast<chr::milliseconds>(t3 - t2).count() << " ms\n";
	std::cout << "PAC   : " << chr::duration_cast<chr::milliseconds>(t4 - t3).count() << " ms\n";

}



