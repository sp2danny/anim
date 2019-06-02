
//#include "stdafx.h"

#include "tokenmap.hpp"

#include <cassert>

bool mapX::has(const BVec& bv) const
{
	return v2i.count(bv) > 0;
}

auto mapX::find(const BVec& bv) const -> findres
{
	auto itr = v2i.find(bv);
	if (itr == v2i.end())
	{
		return {false, (UL)-1};
	}
	else
	{
		return {true, itr->second};
	}
}

bool mapX::add(const BVec& bv)
{
	UL nxt = size();
	auto res = v2i.try_emplace(bv, nxt);
	if (!res.second)
	{
		return false;
	}
	i2v.push_back(bv);
	return true;
}

bool mapX::lookup(UL idx, BVec& bv)
{
	if (idx >= size())
		return false;
	bv = i2v[idx];
	assert(v2i[bv] == idx);
	return true;
}

void mapX::init(UL max)
{
	i2v.clear();
	v2i.clear();
	i2v.reserve(max);
}

UL mapX::size() const
{
	assert(i2v.size() == v2i.size());
	return (UL)v2i.size();
}

// ----------------------------------------------------------------------------




