
#pragma once

#include <vector>
#include <string>
#include <map>
#include <iosfwd>

using namespace std::string_literals;

struct Segment
{
	std::string name;
	std::map<std::string, std::string> values;
};

struct Ini
{
	std::vector<Segment> segments;

	Segment& getSeg(std::string seg);
	const Segment& getSeg(std::string seg) const;

	void read(std::istream&);
	std::string getValue(std::string seg, std::string key, std::string def = ""s) const;
};


