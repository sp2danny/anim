
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cctype>

#include <boost/algorithm/string.hpp>

//#pragma hdrstop

#include "inireader.hpp"

auto Ini::getSeg(std::string seg) -> Segment&
{
	boost::to_lower(seg);
	for (auto& s : segments)
	{
		if (s.name == seg)
			return s;
	}
	segments.push_back({seg,{}});
	return segments.back();
}

auto Ini::getSeg(std::string seg) const -> const Segment&
{
	boost::to_lower(seg);
	for (auto& s : segments)
	{
		if (s.name == seg)
			return s;
	}
	throw "Segment not found";
}

void Ini::read(std::istream& in)
{
	std::string seg;
	std::string str;
	while ( std::getline(in, str) )
	{
		boost::trim(str);
		if (str.empty()) continue;
		if (str[0]=='#') continue;
		auto pos = str.find('[');
		if (pos != std::string::npos)
		{
			auto pos2 = str.find(']');
			seg = str.substr(pos+1, pos2-pos-1);
			boost::to_lower(seg);
			continue;
		}
		pos = str.find('=');
		if (pos == std::string::npos) continue;

		auto k = str.substr(0, pos); boost::trim(k); boost::to_lower(k);
		auto v = str.substr(pos+1);  boost::trim(v);
		getSeg(seg).values[k] = v;
	}
}

std::string Ini::getValue(std::string seg, std::string key, std::string def) const
{
	boost::to_lower(seg);
	boost::to_lower(key);
	for (auto&& s : segments)
	{
		if (s.name == seg)
		{
			auto itr = s.values.find(key);
			if (itr==s.values.end())
				return def;
			return itr->second;
		}
	}
	return def;
}


