
#pragma once

#include <vector>
#include <map>
#include <list>
#include <string>
#include <iosfwd>

#ifndef NO_SFML
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Drawable.hpp>
#endif

#include "core.hpp"
#include "bitstream.hpp"

using namespace std::string_literals;

namespace alib
{

using ::UC;
using ::UL;

using ::BVec;

#pragma pack(push, 1)
struct HSV  { UC h, s, v; };
struct RGB  { UC r, g, b; };
struct HSVA { UC h, s, v, a; };
struct RGBA { UC r, g, b, a; };
#pragma pack(pop)

inline bool operator==(const HSVA& lhs, const HSVA& rhs) { return (lhs.h == rhs.h) || (lhs.s == rhs.s) || (lhs.v == rhs.v) || (lhs.a == rhs.a); }

static_assert( sizeof(RGB)  == 3 );
static_assert( sizeof(HSV)  == 3 );
static_assert( sizeof(HSVA) == 4 );
static_assert( sizeof(RGBA) == 4 );

extern RGBA HSVA_2_RGBA(const HSVA&);
extern HSVA RGBA_2_HSVA(const RGBA&);
extern RGB HSV_2_RGB(const HSV&);
extern HSV RGB_2_HSV(const RGB&);

struct Pos { short x,y; };

struct BasicAnim;
struct CIS;
struct AnimDir;
struct NAV;
struct AnimCollection;

#ifndef NO_SFML
struct AnimReflection;
sf::Clock& GlobalClock();
#endif

// ----------------------------------------------------------------------------

struct CIS
{
	CIS();
	bool Load(const std::string&);
	void Load(std::istream&);
	void LoadOld(std::istream&);
	bool LBS(UC dep, bitsource&);
	void SBT(UC dep, bittarget&) const;

	void SavePacked(UC dep, compress_bypass_target&) const;
	bool LoadPacked(UC dep, decompress_bypass_source&);

	void Crop();
	#ifndef NO_SFML
	void Instance(UC);
	sf::Texture Get(UC=0) const;
	sf::Texture& Get(UC=0);
	AnimReflection Refl(UC hue);
	#endif
	void Clear();
	unsigned short Width() const;
	unsigned short Height() const;
	bool Loaded() const;
	void UnloadBase();
	CIS Flip(bool, bool = false, bool = false) const;
	Pos Hot() const { return hot; }
	Pos& Hot() { return hot; }

	void LoadPCX(const char* fn);

	void LoadDataPal( UC* data, RGB* pal, short w, short y, short hx, short hy );

	void LoadBMP(const char* fn, RGB colorkey, short hx, short hy);

	void SaveBMP(const char* fn, UC hue, RGB colorkey) const;

	enum PixelType { normal = 0, alpha, trans, colimp };

	bool LoadFast(UC*& beg, UC* end);
	UL SaveFast(std::ostream&) const;

private:

	void LoadInternal(std::istream&, bool = false);

	unsigned short w, h;
	Pos hot;
	bool has_dither;
	bool has_trans;
	bool has_colimp;
	bool loaded = false;
	UC depth;

	#ifndef NO_SFML
	typedef std::map<UC, sf::Texture> ImgMap;
	ImgMap::iterator make(UC) const;
	mutable bool instanciated = false;
	mutable ImgMap images;
	#endif

	std::vector<PixelType> pixeltypes;
	std::vector<HSVA> pixels;

};

// ----------------------------------------------------------------------------

struct BasicAnim
{
	bool Load(const std::string&);
	void Load(std::istream&);
	void LoadOld(std::istream&);
	bool LBS(UC dep, bitsource&);
	void SBT(UC dep, bittarget&) const;
	void SavePacked(UC dep, compress_bypass_target&) const;
	bool LoadPacked(UC dep, decompress_bypass_source&);

	void MakeMirror(BasicAnim&, bool, bool, bool);

	void UnloadBase();

	int Size() const { return (int)anim.size(); }

	CIS& Get(int i) { return anim[i]; }

	#ifndef NO_SFML
	AnimReflection Refl(UC);
	sf::Texture& Get(int, UC);
	void Instance(UC hue);
	#endif
	Pos Hot(int idx) const { return anim[idx].Hot(); }

	void LoadSequence(const char* fn);

	void SetDelay(short del) {delay=del;}
	void SetRep(bool r) {repeating=r;}
	void SetJBF(short f) {jbf=f;}
	void Append(CIS cis) { anim.push_back(std::move(cis)); }

	bool LoadFast(UC*& beg, UC* end);
	UL SaveFast(std::ostream&) const;

private:

	void LoadInternal(std::istream&, bool = false);

	short delay = 0;
	bool repeating = false;
	short jbf = 0; // jump back frame
	std::vector<CIS> anim;

friend
	struct AnimReflection;
friend
	struct AnimDir;

};

// ----------------------------------------------------------------------------

struct AnimDir
{
	bool Load(const std::string&);
	void Load(std::istream&);
	void LoadOld(std::istream&);
	bool LBS(UC dep, bitsource&);
	void SBT(UC dep, bittarget&) const;
	void SavePacked(UC dep, compress_bypass_target&) const;
	bool LoadPacked(UC dep, decompress_bypass_source&);

	BasicAnim& Closest(short);

	void Mirror();

	#ifndef NO_SFML
	AnimReflection Refl(short, UC);
	void Instance(UC hue);
	#endif

	void UnloadBase();

	void CreateDir(short dir, bool repeating, short delay, short jbf);
	void AddFrameTo(short dir, CIS&& cis);

	void Clear();

	std::vector<short> AllDirs() const { std::vector<short> ret; for(auto&& b : bad) ret.push_back(b.dir); return ret; }
	BasicAnim& Get(short dir) { return *findexact(dir); }

	void AddDir(short dir, const BasicAnim& ba)
	{
		auto asgn = [](BasicAnim& lhs, const BasicAnim& rhs) { lhs=rhs; };
		bad.emplace_back();
		asgn(bad.back(),ba);
		bad.back().dir = dir;
	}

	bool LoadFast(UC*& beg, UC* end);
	UL SaveFast(std::ostream&) const;

private:
	void LoadInternal(std::istream&, bool = false);

	struct BAD : BasicAnim {
		short dir, mirrorof;
		bool mirror = false, flipx, flipy, rot90;
	};
	std::vector<BAD> bad;

	BAD* findexact(short);

};

// ----------------------------------------------------------------------------

struct NAV
{
	bool Load(const std::string&);
	void Load(std::istream&);
	void LoadOld(std::istream&);
	bool LBS(UC dep, bitsource&);
	void SBT(UC dep, bittarget&) const;
	void SavePacked(UC dep, compress_bypass_target&) const;
	bool LoadPacked(UC dep, decompress_bypass_source&);

	#ifndef NO_SFML
	AnimReflection Refl(short, UC);
	void Instance(UC hue);
	#endif

	void UnloadBase();

	int Count() const { return (int)variants.size(); }
	AnimDir& Get(int idx) { return variants[idx]; }
	const AnimDir& Get(int idx) const { return variants[idx]; }

private:
	std::vector<AnimDir> variants;
	std::string name;
	void LoadInternal(std::istream&, bool = false);

friend
	struct AnimCollection;
};

// ----------------------------------------------------------------------------

struct AnimCollection
{
	void AddVariant(std::string, AnimDir);

	bool Load(const std::string&);
	void Load(std::istream&);
	void LoadOld(std::istream&);
	bool LBS(UC dep, bitsource&);
	void SBT(UC dep, bittarget&) const;
	void SavePacked(UC dep, compress_bypass_target&) const;
	bool LoadPacked(UC dep, decompress_bypass_source&);

	#ifndef NO_SFML
	AnimReflection Refl(std::string, short, UC);
	void Instance(UC hue);
	#endif

	void UnloadBase();

	std::vector<std::string> CoreNames() const;
	std::vector<std::string> AllNames() const;

	NAV& Get(std::string n) { return *mappings[n]; }

	bool LoadFast(const std::string&);
	bool LoadFast(std::istream&);
	bool LoadFast(BVec&);
	UL SaveFast(const std::string&) const;
	UL SaveFast(std::ostream&) const;
	UL SaveFast(BVec&) const;

private:
	std::list<NAV> core;
	std::map<std::string, NAV*> mappings;
	std::string default_anim;

	void LoadInternal(std::istream&, bool = false);
};

// ----------------------------------------------------------------------------

#ifndef NO_SFML

struct AnimReflection : sf::Drawable, sf::Transformable
{
	virtual void draw(sf::RenderTarget&, sf::RenderStates) const override;

	bool Next();
	bool Update(int);
	void Start();
	bool Update();

	void Set(BasicAnim*, UC);
	void Set(CIS*, UC);
	void Set(AnimReflection& ar);
	void ContinueWith(const AnimReflection& ar);

	AnimReflection();
	AnimReflection(BasicAnim*, UC);
	AnimReflection(CIS*, UC);
	int Loopcnt() { return loopcnt; }
private:
	void clr();
	BasicAnim* ba;
	CIS* cis;
	int current;
	int time, last;
	int loopcnt;
	UC hue;
};
typedef AnimReflection Refl;
#endif
// ----------------------------------------------------------------------------

using BA = BasicAnim;
using AD = AnimDir;
using AC = AnimCollection;

}
