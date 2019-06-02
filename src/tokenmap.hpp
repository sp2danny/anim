
#pragma once

#include "core.hpp"

#include <utility>
#include <vector>
#include <list>
#include <map>

struct mapX
{
	mapX(UC toksz) : toksz(toksz) {}

	typedef std::pair<bool, UL> findres;

	std::map<BVec, UL> v2i;
	std::vector<BVec> i2v;

	bool has(const BVec&) const;
	findres find(const BVec&) const;
	bool add(const BVec&);
	bool lookup(UL, BVec&);
	void init(UL max);
	UL size() const;
	const UC toksz;
};

struct map6 : mapX
{
	map6() : mapX(6) {}

};

struct sbct
{
	typedef std::pair<bool, UL> findres;

	bool has(const BVec&) const;
	findres find(const BVec&) const;
	bool add(const BVec&);
	bool addh(UC c, UL hint);
	bool lookup(UL, BVec&);
	void init(UL max, bool = true);
	UL size() const { return (UL)all.size(); }

	static constexpr UL nil = (UL)-1;

	struct Item;
	struct Item
	{
		Item();
		Item(UL idx, UL par, UC me);
		UL idx = 0;
		UL leafs[64];
		UL par = 0;
		UC me;
	};
	Item head{nil,nil,0};
	std::vector<Item> all;
};



