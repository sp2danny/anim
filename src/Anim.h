
#pragma once

#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>

#include <SFML/Graphics.hpp>

namespace Anim
{
	typedef unsigned char UC;

	struct Pos { int x,y; Pos():x(0),y(0){} Pos(int x,int y):x(x),y(y){} };

	struct HSVA { UC h,s,v,a; };
	struct RGBA { UC r,g,b,a; RGBA(); RGBA(UC,UC,UC); RGBA(UC,UC,UC,UC); };

	RGBA HSVA_2_RGBA(const HSVA&);
	HSVA RGBA_2_HSVA(const RGBA&);

	struct CIS;

	struct Surface 
	{

		void FromCIS(CIS&);
		void FromCIS(CIS&, UC);
		void FromCIS(CIS&, UC, UC);
		void FromBMP(char*);
		void FromBMP(char*, RGBA);

		void Free();

		static Surface Screen();

		int Width()  const { return w; }
		int Height() const { return h; }

		Pos Hot() const;
		void Hot(const Pos&);

		void Create(int,int);

		sf::Texture& SFML() { return texture; }

		Surface() = default;

	private:
		std::int16_t hx,hy,w,h;
		sf::Texture texture;
	};

	struct BasicAnim;

	struct AnimReflection
	{
		bool Next();
		bool Update(int);
		void Start();
		bool Update();
		void Overlay(sf::RenderTarget& rt, int, int);
		void Overlay(sf::RenderTarget& rt, Pos p) { Overlay(rt, p.x,p.y); }
		void Set(BasicAnim*, UC);
		void Set(CIS*, UC);
		void Set(AnimReflection& ar) { (*this)=ar; }
		void Set(AnimReflection&& ar) { (*this)=ar; }
		AnimReflection();
		AnimReflection(BasicAnim*, UC);
		AnimReflection(CIS*, UC);
		int Loopcnt() const { return loopcnt; }
		int Current() const { return current; }
		void Current(int curr) { current=curr; }
	private:
		void clr();
		BasicAnim* ba;
		CIS* cis;
		std::int32_t current;
		std::int32_t time,last;
		std::int32_t loopcnt;
		UC hue;
	};

	struct CIS
	{
		enum PixelType { normal=0, alpha, trans, colimp };

	//private:
		std::uint16_t w,h;
		std::int16_t hx,hy;
		bool has_dither;
		bool has_trans;
		bool has_colimp;
		UC depth;

		std::vector<PixelType> pixeltypes;
		std::vector<HSVA> pixels;

	public:
		void Load(std::istream&);
		void Save(std::ostream& , int=0);
		//void FromImg( SDL_Surface* image, SDL_Surface* transmask, SDL_Surface* dithermask, SDL_Surface* colimpmask );

		void LoadOld(std::istream&);
		void SaveOld(std::ostream&);

		bool LoadExt(std::string);

		CIS CutOut(int,int, int,int);

		AnimReflection Refl(UC hue);

		sf::Texture MakeSurface();
		sf::Texture MakeSurface(UC hue); // dithered
		sf::Texture MakeSurface(UC alp, UC hue); // blended

		void Scale150w(), Scale150h();
		void Scale150() { Scale150w(); Scale150h(); }
		CIS HalfSize();
		void HalfSizeMe() { (*this)=HalfSize(); }
		void Skip3();
		void Crop();
		void MakeWhite();
		void MakeDark();
		void Scale50w(), Scale50h();
		void Scale50() { HalfSizeMe(); }
		void Unimport();

		CIS Flip(bool, bool=false, bool=false);

		Surface& Get(UC hue);

		void Instance(UC hue);
		void FreeData();
		void UnInstance();

		int Width() const { return w; }
		int Height() const { return h; }
		Pos Hot() const { return Pos(hx, hy); }
		void Hot(const Pos& p) { hx=p.x; hy=p.y; }

		std::map<UC,Surface> instance;

		CIS() : w(0), h(0), hx(0), hy(0), depth(4) { instance.clear(); has_dither=has_trans=has_colimp=false; } 

		//void LoadPCX(const char* fn);

	private:
		void LoadInternal( std::istream& );
		void SaveInternal( std::ostream& );
	friend
		struct Surface;

	};
	typedef void (CIS::*ClosureCIS)();

	typedef struct BasicAnim
	{
		BasicAnim() : delay(0), repeating(false), jbf(0) {}
		std::int16_t delay;
		bool repeating;
		std::int16_t jbf; // jump back frame
		std::vector<CIS> anim;
		void Load(std::istream&);
		void Save(std::ostream&);
		void MakeMirror(BasicAnim& , bool, bool, bool);

		template<ClosureCIS exe> void DoAll() { for(CIS& cis:anim) (cis.*exe)(); }

		bool LoadExt(std::string);

		void Instance(UC hue);
		void FreeData();
		void UnInstance();

		int Size() { return anim.size(); }

		AnimReflection Refl(UC);

		Surface& Get(int, UC);

		void LoadOld(std::istream&);
		void SaveOld(std::ostream&);
	private:
		void SaveInternal(std::ostream&);
		void LoadInternal(std::istream&, bool=false);
	} BA;
	typedef void (BasicAnim::*ClosureBA)();

	typedef struct AnimDir
	{
		struct BAD : BasicAnim {
			std::int16_t dir,mirrorof;
			bool mirror,flipx,flipy,rot90;
			BAD() : mirror(false) {}
		};
		std::vector<BAD> bad;

		template<ClosureBA exe> void DoAll() { for(BAD& b:bad) (b.*exe)(); }

		bool LoadExt(std::string);
		void Load(std::istream&);
		void Save(std::ostream&);
		BasicAnim& Closest(short);
		void Mirror();
		AnimReflection Refl(short,UC);

		//int UseAsFont(SDL_Surface*, Pos, UC, std::string);

		void Instance(UC hue);
		void FreeData();
		void UnInstance();

		void LoadOld(std::istream&);
		void SaveOld(std::ostream&);
	private:
		void SaveInternal(std::ostream&, bool=false);
		void LoadInternal(std::istream&, bool=false);

		BAD* findexact(short);

	} AD;
	typedef void (AnimDir::*ClosureAD)();

	struct NAV
	{
		std::vector<AnimDir> variants;
		std::string name;
		bool LoadExt(std::string);
		void Load(std::istream&);
		void Save(std::ostream&);
		AnimReflection Refl(short,UC);

		void Instance(UC hue);
		void FreeData();
		void UnInstance();

		template<ClosureAD exe> void DoAll() { for(AnimDir& ad:variants) (ad.*exe)(); }

		void LoadOld(std::istream&);
		void SaveOld(std::ostream&);
	private:
		void SaveInternal(std::ostream&);
		void LoadInternal(std::istream&, bool=false);

	};
	typedef void (NAV::*ClosureNAV)();

	typedef struct AnimCollection
	{
		void AddVariant(std::string,AnimDir);
		std::vector<NAV*> core;
		std::map<std::string, NAV*> mappings;
		bool LoadExt(std::string);
		void Load(std::istream&);
		void Save(std::ostream&);
		AnimReflection Refl(std::string, short, UC);

		std::string default_anim;

		void Instance(UC hue);
		void UnInstance();
		void FreeData();

		std::vector<std::string> CoreNames();
		std::vector<std::string> AllNames();

		AnimCollection() = default;
		AnimCollection(const AnimCollection&) = default;
		AnimCollection(AnimCollection&&) = default;

		~AnimCollection();

		template<ClosureNAV exe> void DoAll() { for(NAV* nav:core) (nav->*exe)(); }

		void LoadOld(std::istream&);
		void SaveOld(std::ostream&);
	private:
		void SaveInternal(std::ostream&);
		void LoadInternal(std::istream&, bool=false);
	} AC;

}
