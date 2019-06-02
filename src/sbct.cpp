
#include "tokenmap.hpp"
#include "core.hpp"

#include <cassert>
#include <algorithm>

// ----------------------------------------------------------------------------

constexpr UL sbct::nil;

sbct::Item::Item()
{
	for (auto& x : leafs)
		x = nil;
}

sbct::Item::Item(UL i, UL p, UC m)
	: Item()
{
	idx=i;
	par=p;
	me=m;
}

bool sbct::has(const BVec& bv) const
{
	const Item* p = &head;
	assert(p);
	for (auto c : bv)
	{
		assert(c < 64);
		UL idx = p->leafs[c];
		if (idx==nil) return false;
		p = &all[idx];
	}
	return true;
}

auto sbct::find(const BVec& bv) const -> findres
{
	const Item* p = &head;
	assert(p);
	UL idx;
	for (auto c : bv)
	{
		assert(c < 64);
		idx = p->leafs[c];
		if (idx==nil) return {false,0};
		p = &all[idx];
	}
	return {true, p->idx};
}

bool sbct::addh(UC c, UL hint)
{
	Item* p = &head;
	p = &all[hint];
	if (p->leafs[c] != nil) return false;
	UL idx = (UL)all.size();
	all.emplace_back(idx, p->idx, c);
	p->leafs[c] = idx;
	return true;
}


bool sbct::add(const BVec& bv)
{
	UL i, n = (UL)bv.size();
	Item* p = &head;
	assert(p);
	for (i = 0; i < (n - 1); ++i)
	{
		UC c = bv[i];
		assert(c < 64);
		UL idx = p->leafs[c];
		if (idx==nil) return false; // up to last should exist
		p = &all[idx];
	}
	UC c = bv.back();
	assert(c < 64);
	if (p->leafs[c] != nil) return false; // should not already be there
	UL idx = (UL)all.size();
	all.emplace_back(idx, p->idx, c);
	p->leafs[c] = idx;
	return true;
}

void sbct::init(UL max, bool fill)
{
	all.clear();
	//if (max>250'000)
	//	all.reserve(250'000);
	//else
		all.reserve(max);
	if (fill)
		for (UC c = 0; c < 64; ++c)
		{
			all.emplace_back(c, nil, c);
			head.leafs[c] = c;
		}
}

UL longest_sequence = 0;

bool sbct::lookup(UL p, BVec& bv)
{
	if (p >= all.size()) return false;
	bv.clear();
	while (p!=nil)
	{
		assert(p<all.size());
		bv.push_back( all[p].me );
		p = all[p].par;
	}
	UL sz = (UL)bv.size();
	if (sz > longest_sequence)
		longest_sequence = sz;
	std::reverse(bv.begin(), bv.end());
	return true;
}

// ----------------------------------------------------------------------------




