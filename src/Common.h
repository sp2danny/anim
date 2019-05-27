
#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cstdlib>

struct SDL_Surface;
struct SDL_PixelFormat;
union SDL_Event;

namespace Anim
{
	extern const int SZ, YSZ;
	extern float offs_x, offs_y;

	using std::string;
	using std::vector;
	using std::ostream;
	using std::istream;
	using std::map;

	typedef vector<string> StingList;
	typedef vector<int> IdList;

	struct Pos { int x,y; Pos():x(0),y(0){} Pos(int _x,int _y):x(_x),y(_y){} };

	enum Dir6 : int { d6_beg=0, d6_upright=d6_beg, d6_right, d6_downright, d6_downleft, d6_left, d6_upleft, d6_end };

	const short Degree[d6_end] { 60, 0, 300, 240, 180, 120 };

	inline Dir6 operator++(Dir6& d) { return d = (Dir6) ( 1+(int)d ) ; }

	inline Dir6 CW  (Dir6 d) { return (Dir6) ( ((int)d + 1) % 6); }
	inline Dir6 CCW (Dir6 d) { return (Dir6) ( ((int)d + 5) % 6); }

	struct Hexagon;
	struct Player;
	class  Board;
	class  PlayerBoard;
	struct Orders;
	struct Unit;
	struct City;
	class  UI;
	struct City;
	struct PlayerHex;
	struct Movement;
	class  GameEngine;
	//class  ControllerAPI;
	//class  Controller;

	typedef unsigned char UC;

	//typedef Hexagon* PHexagon;

	typedef unsigned short Tile;
	typedef unsigned short Mask;

	typedef unsigned long Color;

	// masks
	const Mask road = 1;
	const Mask rail = 2;
	const Mask forest = 4;
	const Mask jungle = 8;
	const Mask mine = 16;
	const Mask irri = 32;

	const Mask river_upright   = 64;
	const Mask river_right     = 128;
	const Mask river_downright = 256;

	const Mask bridge = 512;

	// tiles
	const Tile grass  = 1;
	const Tile bonus  = 2;
	const Tile plains = 3;
	const Tile desert = 4;
	const Tile floodp = 5;
	const Tile tundra = 6;
	const Tile hill   = 7;
	const Tile mount  = 8;
	const Tile coast  = 9;
	const Tile sea    = 10;
	const Tile ocean  = 11;

	// tile overlay id's
	enum class TOID {
		road_c, road_r, road_dr, road_dl, road_l, road_ul, 
		forest, pine, jungle,
		irri, mine,
		river_ur, river_r, river_dr,
		goody, camp,
		bridge_r, bridge_dr, bridge_dl, bridge_l, bridge_ul, 
		hang_r_l, hang_ur_dl, hang_ul_dr,
		rail,
		city_1, city_2, city_3,
		grid
	};

	// marker id's
	enum class MID {
		normal, road_to, go_to, rally, 
		unavail, selected,
		bombard, airdrop
	};

}

inline std::string IntToStr(int i) { return std::to_string(i); }
inline std::string IntToStrPad(int i,unsigned n,char p) { std::string s = std::to_string(i); while(s.length()<n) s=p+s; return s; }

template <typename T> inline void ReadBinary( std::istream& istr , T& val )
{
	int i,n = sizeof(T);
	char* p = (char*) &val;

	for(i=0;i<n;++i)
	{
		char c;
		istr.read(&c,1);
		(*p) = c;
		++p;
	}
}

template <typename T> inline void WriteBinary( std::ostream& ostr , const T& val )
{
	int i,n = sizeof(T);
	const char* p = (char*) &val;

	for(i=0;i<n;++i)
	{
		char c;
		c = (*p);
		++p;
		ostr.write(&c,1);
	}
}
