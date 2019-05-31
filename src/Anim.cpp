
// --------------------------------------------

#include "Anim.h"

#include <cassert>

template <class C>
constexpr auto ssize(const C& c) -> int
{
	return static_cast<int>(c.size());
}

template<typename T,typename U>
T& BestFit(std::int16_t dir, U& vec)
{
	auto dirdiff = [](std::int16_t d1, std::int16_t d2) -> short
	{
		std::int16_t df = abs(d1-d2);
		while (df>360) df -= 360;
		if (df>180) df = 360 - df;
		return df;
	};

	int idx=0;
	int n = vec.size();
	assert(n);
	int srt = dirdiff(dir, vec[0].dir);
	for (int i=1; i<n; ++i)
	{
		int s = dirdiff(dir, vec[i].dir);
		if (s<srt)
		{
			srt=s;
			idx=i;
		}
	}
	return vec[idx];
}
/*
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
}*/

// ************
// *** RGBA ***
// ************

Anim::RGBA::RGBA()
	: r(0), g(0), b(0), a(0)
{
}

Anim::RGBA::RGBA(UC r, UC g, UC b)
	: r(r), g(g), b(b), a(0)
{
}
	
Anim::RGBA::RGBA(UC r, UC g, UC b, UC a)
	: r(r), g(g), b(b), a(a)
{
}

// ***************
// *** Surface ***
// ***************

auto Anim::Surface::Hot() const -> Anim::Pos
{
	return Pos(hx,hy);
}
void Anim::Surface::Hot(const Pos& p) { hx=p.x; hy=p.y; }

/*
void Anim::Surface::Overlay( SDL_Surface* dst, int x,int y) const
{
	if( surface && dst )
	{
		SDL_Rect dr = { (short)(x-hx), (short)(y-hy), (unsigned short)(surface->w), (unsigned short)(surface->h) };
		SDL_BlitSurface( surface, 0, dst, &dr );
	}
}

void Anim::Surface::Overlay( Surface& dst, int x,int y ) const
{
	if( surface && dst.surface )
	{
		SDL_Rect dr = { (short)(x-hx), (short)(y-hy), (unsigned short)(surface->w), (unsigned short)(surface->h) };
		SDL_BlitSurface( surface, 0, dst.surface, &dr );
	}

}
*/

void Anim::Surface::Create(int ww, int hh)
{
	texture.create(ww, hh);
	w = ww;
	h = hh;
	hx = hy = 0;
}

void Anim::Surface::FromCIS(CIS& cis)
{
	w = cis.Width(); h = cis.Height();
	texture = cis.MakeSurface();
	hx = cis.Hot().x; hy = cis.Hot().y;
}

void Anim::Surface::FromCIS(CIS& cis, UC hue)
{
	w = cis.Width(); h = cis.Height();
	texture = cis.MakeSurface(hue);
	hx = cis.Hot().x; hy = cis.Hot().y;
}

void Anim::Surface::FromCIS(CIS& cis, UC alpha, UC hue)
{
	w = cis.Width(); h = cis.Height();
	texture = cis.MakeSurface(alpha, hue);
	hx = cis.Hot().x; hy = cis.Hot().y;
}

void Anim::Surface::FromBMP(char* bmp)
{
	texture.loadFromFile(bmp);
	w = texture.getSize().x;
	h = texture.getSize().y;
	hx = hy = 0;
}

void Anim::Surface::FromBMP(char* bmp, RGBA ck)
{
	sf::Image img;
	img.loadFromFile(bmp);
	sf::Color col;
	col.r = ck.r;
	col.g = ck.g;
	col.b = ck.b;
	img.createMaskFromColor(col);
	texture.loadFromImage(img);
	w = texture.getSize().x;
	h = texture.getSize().y;
	hx = hy = 0;
}

void Anim::Surface::Free()
{
	texture = sf::Texture{};
}

Anim::Surface Anim::Surface::Screen()
{
	Surface s;
	s.hx = s.hy = 0;
	return s;
}

template <typename T>
inline void ReadBinary(std::istream& istr, T& val)
{
	int i,n = sizeof(T);
	char* p = (char*) &val;

	for (i=0; i<n; ++i)
	{
		char c;
		istr.read(&c,1);
		(*p) = c;
		++p;
	}
}

template<typename T>
inline void WriteBinary(std::ostream& ostr, const T& val)
{
	int i, n = sizeof(T);
	const char* p = (char*) &val;

	for (i=0; i<n; ++i)
	{
		char c;
		c = (*p);
		++p;
		ostr.write(&c,1);
	}
}


// ***********
// *** CIS ***
// ***********

Anim::AnimReflection Anim::CIS::Refl(UC hue)
{
	AnimReflection ar(this, hue);
	return ar;
}

std::string ExtractFileExt(std::string fn)
{
	auto p = fn.find_last_of('.');
	if (p==std::string::npos) return "";
	std::string ret = fn.substr(p+1);
	for (char& c : ret) c = tolower(c);
	return ret;
}

std::string ExtractFileBase(std::string fn)
{
	auto p = fn.find_last_of('.');
	if (p==std::string::npos) return fn;
	std::string ret = fn.substr(0,p);
	//for( char& c : ret ) c=tolower(c);
	return ret;
}

// extern Anim::CIS LoadPCX(const char* fn);

bool Anim::CIS::LoadExt(std::string fn)
{
	std::string ext = ExtractFileExt(fn);
	if (ext=="cis")
	{
		std::ifstream ifs(fn, std::ios::in|std::ios::binary);
		if (ifs.bad()) return false;
		Load(ifs);
		return true;
	}
	//else if(ext=="bmp")
	//{
	//	SDL_Surface* srf = SDL_LoadBMP(fn.c_str());
	//	if(!srf) return false;
	//	SDL_Surface* tm = MakeTransMaskFromImage(srf);
	//	FromImg(srf,tm,0,0);
	//	return true;
	//}
	//else if(ext=="pcx")
	//{
	//	LoadPCX(fn.c_str());
	//	return true;
	//}
	return false;
}
/*
void Anim::CIS::DummySave()
{
	static int ii = 1;
	char buff[256];
	sprintf(buff,"tst/img_%04d.bmp",ii++);
	Surface srf;
	srf.FromCIS(*this,0);
	SDL_SaveBMP( srf.surface, buff );
	srf.Free();
}*/

void Anim::CIS::LoadInternal(std::istream& is)
{
	has_dither = has_trans = has_colimp = false;

	//assert( w < 150 );
	//assert( h < 150 );
	//assert( abs(hx) < 150 );
	//assert( abs(hy) < 150 );

	int i,sz = w*h;

	pixeltypes.clear();
	pixeltypes.reserve(sz);

	unsigned char c;
	int leftbit = 0;

	for (i=0; i<sz; ++i)
	{
		if (!leftbit)
		{
			ReadBinary(is,c);
			leftbit=8;
		}

		PixelType pt = (PixelType) (c&3);
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

	auto GetC_4 = [&odd,&cc](std::istream& is) -> UC
	{
		if (!odd)
		{
			odd=!odd;
			ReadBinary(is,cc);
			return (cc & 0x0F) << 4;
		} else {
			odd=!odd;
			return (cc & 0xF0) ;
		}
	};

	auto GetC_6 = [&odd,&cc](std::istream& is) -> UC
	{
		UC c2 = cc;
		switch(odd++%4)
		{
			case 0:  // 0 + 6
				ReadBinary(is,cc);
				return cc & 0xFC;
			case 1:  // 2 + 4
				ReadBinary(is,cc);
				return ((c2&0x03)<<6) | ((cc&0xF0)>>2);
			case 2:  // 4 + 2
				ReadBinary(is,cc);
				return ((c2&0x0F)<<4) | ((cc&0xC0)>>4);
			default: // 6 + 0
				return (c2&0x3F)<<2;
		}
	};

	auto GetC_8 = [&cc](std::istream& is) -> UC
	{
		ReadBinary(is,cc);
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
	for (i=0; i<sz; ++i)
	{
		HSVA p = { 0,0,0,255 };
		switch (pixeltypes[i])
		{
			case normal: p.h = GetC(is); p.s = GetC(is); p.v = GetC(is); break;
			case colimp:                 p.s = GetC(is); p.v = GetC(is); break;
			case alpha:  p.h = GetC(is); p.s = GetC(is); p.v = GetC(is); p.a = 128; break;
			case trans:  p.a = 0; break;
		}
		pixels.push_back(p);
	}

	#ifdef _DEBUG
	//DummySave();
	#endif
}

void Anim::CIS::SaveInternal(std::ostream& os)
{
	unsigned int i,sz = w*h;

	assert(sz<=pixeltypes.size());
	unsigned char c=0;
	int leftbits = 0;
	for (i=0; i<sz; ++i)
	{
		if (leftbits==8)
		{
			WriteBinary(os, c);
			leftbits = 0;
			c = 0;
		}
		PixelType& pt = pixeltypes[i];
		c = c | ((pt&3) << leftbits);
		leftbits += 2;

		assert((pt&3) == pt);
	}
	if (leftbits!=0) WriteBinary(os, c);

	int odd = 0;
	UC cc;

	auto PutC_4 = [&odd,&cc](std::ostream& os,UC uc) -> void
	{
		if(!odd)
		{
			odd=!odd;
			cc = uc >> 4;
		} else {
			odd=!odd;
			cc |= uc&0xF0;
			WriteBinary(os, cc);
		}
	};
	auto Rem = [&odd,&cc](std::ostream& os) -> void
	{
		if(odd) WriteBinary(os, cc);
	};
	auto PutC_8 = [](std::ostream& os, UC uc) -> void
	{
		WriteBinary(os, uc);
	};
	auto PutC_6 = [&](std::ostream&, UC) -> void
	{
		// later
	};

	auto PutC = [&](std::ostream& os,UC uc) -> void
	{
		switch (this->depth)
		{
			case 4: PutC_4(os,uc); break;
			case 6: PutC_6(os,uc); break;
			case 8: PutC_8(os,uc); break;
			default: assert(false);
		}
	};

	assert(sz<=pixels.size());
	for (i=0; i<sz; ++i)
	{
		PixelType& pt = pixeltypes[i];
		HSVA& p = pixels[i];
		switch (pt)
		{
			case trans:                                                  break;
			case normal: PutC(os, p.h ); PutC(os, p.s ); PutC(os, p.v ); break;
			case alpha:  PutC(os, p.h ); PutC(os, p.s ); PutC(os, p.v ); break;
			case colimp:                 PutC(os, p.s ); PutC(os, p.v ); break;
			default:
				assert(false);
		}
	}
	Rem(os);
}

void Anim::CIS::LoadOld(std::istream& is)
{
	ReadBinary(is, w);
	ReadBinary(is, h);
	ReadBinary(is, hx);
	ReadBinary(is, hy);
	depth = 4;
	LoadInternal(is);
}

void Anim::CIS::SaveOld( std::ostream& os )
{
	WriteBinary(os, w);
	WriteBinary(os, h);
	WriteBinary(os, hx);
	WriteBinary(os, hy);
	depth = 4;
	SaveInternal(os);
}

void Anim::CIS::Load(std::istream& is)
{
	char buff[5] = {};

	is.read(buff,4);
	if (std::string(buff) != "CIS2")
	{
		is.seekg(-4, std::ios_base::cur);
		//is.putback(buff[3]).putback(buff[2]).putback(buff[1]).putback(buff[0]);
		LoadOld(is);
	} else {
		ReadBinary(is, w);
		ReadBinary(is, h);
		ReadBinary(is, hx);
		ReadBinary(is, hy);
		ReadBinary(is, depth);
		LoadInternal(is);
	}
}

void Anim::CIS::Save(std::ostream& os, int dep)
{
	os.write("CIS2" , 4);
	WriteBinary(os, w);
	WriteBinary(os, h);
	WriteBinary(os, hx);
	WriteBinary(os, hy);
	if (dep) depth = dep;
	if (!depth) depth = 4;
	WriteBinary(os, depth);
	SaveInternal(os);
}

Anim::RGBA Anim::HSVA_2_RGBA( const HSVA& hsv )
{
	RGBA rgb;
	rgb.a = hsv.a;

	unsigned char reg, rem, p,q,t;

	if (hsv.s == 0)
	{
		rgb.r = hsv.v;
		rgb.g = hsv.v;
		rgb.b = hsv.v;
		return rgb;
	}

	reg = hsv.h / 43;
	rem = (hsv.h - (reg*43)) * 6;

	p = (hsv.v * (255 - hsv.s)) >> 8;
	q = (hsv.v * (255 - ((hsv.s*rem) >> 8 ))) >> 8;
	t = (hsv.v * (255 - ((hsv.s * (255-rem)) >> 8))) >> 8;

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

using std::min;
using std::max;

Anim::HSVA Anim::RGBA_2_HSVA( const RGBA& rgb )
{
	HSVA hsv = { 0,0,0,rgb.a };

	unsigned char rgb_min = min( rgb.r, min( rgb.g, rgb.b) );
	unsigned char rgb_max = max( rgb.r, max( rgb.g, rgb.b) );
	long delta = rgb_max - rgb_min;

	hsv.v = rgb_max;

	if(!hsv.v) return hsv;

	hsv.s = UC(255 * delta / hsv.v);
	if (!hsv.s) return hsv;

	/**/ if (rgb_max == rgb.r) { hsv.h = UC(  0 + 43 * (rgb.g-rgb.b) / delta); }
	else if (rgb_max == rgb.g) { hsv.h = UC( 85 + 43 * (rgb.b-rgb.r) / delta); }
	else if (rgb_max == rgb.b) { hsv.h = UC(171 + 43 * (rgb.r-rgb.g) / delta); }
	else assert(false);

	return hsv;
}
/*
Anim::RGBA GetPixel( SDL_Surface* surf, int x, int y)
{
	SDL_PixelFormat* fmt = surf->format;
	char* p = (char*) surf->pixels;
	p += y * surf->pitch;
	p += x * fmt->BytesPerPixel;
	Uint32* pix = (Uint32*)p;
	Uint32 temp, pixel = *pix;
	Uint8 red, green, blue, alpha;

	temp=pixel&fmt->Rmask; temp=temp>>fmt->Rshift; temp=temp<<fmt->Rloss; red   =(Uint8)temp;
	temp=pixel&fmt->Gmask; temp=temp>>fmt->Gshift; temp=temp<<fmt->Gloss; green =(Uint8)temp;
	temp=pixel&fmt->Bmask; temp=temp>>fmt->Bshift; temp=temp<<fmt->Bloss; blue  =(Uint8)temp;
	temp=pixel&fmt->Amask; temp=temp>>fmt->Ashift; temp=temp<<fmt->Aloss; alpha =(Uint8)temp;

	return Anim::RGBA(red,green,blue,alpha);
}

SDL_Surface* Anim::MakeSurface(int w,int h,SDL_PixelFormat* pf,int mask)
{
	return
		SDL_CreateRGBSurface(mask,w,h,pf->BitsPerPixel,pf->Rmask,pf->Gmask,pf->Bmask,pf->Amask);
}

SDL_Surface* Anim::MakeSurface(int w,int h,SDL_PixelFormat* pf)
{
	return MakeSurface(w,h,pf,SDL_HWSURFACE);
}

SDL_Surface* Anim::MakeTransMaskFromImage(SDL_Surface* img)
{
	int w = img->w;
	int h = img->h;
	SDL_PixelFormat* pf = SDL_GetVideoSurface()->format;
	SDL_Surface* tm = MakeSurface(w,h,pf);

	SDL_LockSurface(img);
	SDL_LockSurface(tm);
	int x,y;
	int i=0;

	for(y=0;y<h;++y)
	{
		unsigned char* ptr = (unsigned char*) tm->pixels;
		ptr += (tm->pitch) * y;
		unsigned long ck = SDL_MapRGB(pf,255,0,255);
		unsigned long wh = SDL_MapRGB(pf,255,255,255);
		unsigned long bl = SDL_MapRGB(pf,0,0,0);
		for(x=0;x<w;++x)
		{
			Anim::RGBA pp = GetPixel(img,x,y);
			unsigned long pix = SDL_MapRGB(pf,pp.r,pp.g,pp.b);
			if(pix==ck)
			{
				pix=bl;
			} else {
				pix=wh;
			}
			for(int j=0;j<pf->BytesPerPixel;++j)
			{
				(*ptr) = (unsigned char)(pix&255);
				pix = pix >> 8;
				++ptr;
			}
			++i;
		}
	}
	SDL_UnlockSurface(img);
	SDL_UnlockSurface(tm);

	return tm;
}
*/

/*
static int grayscale(Anim::RGBA p)
{
	return (p.r+p.g+p.b) / 3;
}
*/

namespace
{
	struct Pix
	{
		Pix() {}
		Pix(Anim::CIS::PixelType _pt, Anim::HSVA _hsv) : pt(_pt), hsv(_hsv) {}
		Anim::CIS::PixelType pt;
		Anim::HSVA hsv;
	};
	typedef std::vector<Pix> Pixels;

	bool HasMajority(const Pixels& pa, Pix& mean)
	{
		unsigned int count[4] = {0,0,0,0};

		for (const Pix& p : pa)
		{
			count[ p.pt ] ++;
		}

		bool found = false;
		int c, pt;
		for (c=0; c<4; ++c)
		{
			if (count[c] > (pa.size()/2))
			{
				found=true;
				pt=c;
			}
		}
		if (!found) return false;

		int h=0,s=0,v=0,a=0;
		for (const Pix& p : pa)
		{
			if (p.pt == pt)
			{
				h += p.hsv.h;
				s += p.hsv.s;
				v += p.hsv.v;
				a += p.hsv.a;
			}
		}
		mean.pt = (Anim::CIS::PixelType)pt;
		mean.hsv.h = h / count[pt];
		mean.hsv.s = s / count[pt];
		mean.hsv.v = v / count[pt];
		mean.hsv.a = a / count[pt];

		return true;
	}
}

void Anim::CIS::Scale150w()
{
	std::vector<PixelType> pt2;
	std::vector<HSVA> p2;

	int x, y;
	int i = 0;

	PixelType curr,next;
	HSVA hsv1, hsv2;

	int ww = 0;
	for (x=0; x<w; ++x)
	{
		++ww;
		if (((x%2)==0) && ((x+1)<w)) ++ww;
	}

	for (y=0; y<h; ++y)
	{
		for (x=0; x<w; ++x)
		{
			curr = pixeltypes[i];
			hsv1 = pixels[i];

			pt2.push_back(curr);
			p2.push_back(hsv1);

			if (((x%2)==0) && ((x+1)<w))
			{
				Pixels pp; Pix m;
				pp.push_back(Pix(curr,hsv1));
				next = pixeltypes[i+1];
				hsv2 = pixels[i+1];
				pp.push_back(Pix(next,hsv2));
				if (HasMajority(pp,m))
				{
					pt2.push_back(m.pt);
					p2.push_back(m.hsv);
				} else {
					if (y>0)
					{
						pp.push_back(Pix(pixeltypes[i-w],pixels[i-w]));
						pp.push_back(Pix(pixeltypes[i-w+1],pixels[i-w+1]));
					}
					if ((y+1)<h)
					{
						pp.push_back(Pix(pixeltypes[i+w],pixels[i+w]));
						pp.push_back(Pix(pixeltypes[i+w+1],pixels[i+w+1]));
					}
					if (HasMajority(pp,m))
					{
						pt2.push_back(m.pt);
						p2.push_back(m.hsv);
					} else {
						pt2.push_back(curr);
						p2.push_back(hsv1);
					}
				}
			}
			++i;
		}
	}

	w = ww;

	hx += hx/2;

	pixeltypes.clear();
	pixeltypes.assign(pt2.begin(), pt2.end());

	pixels.clear();
	pixels.assign(p2.begin(), p2.end());
}

void Anim::CIS::Skip3()
{
	std::vector<PixelType> pt2;
	std::vector<HSVA> p2;

	int x, y;
	int nw=0, nh=0;
	for (x=0; x<w; ++x) if((x%3) != 2) ++nw;
	for (y=0; y<h; ++y) if((y%3) != 2) ++nh;

	pt2.reserve(nw*nh);
	p2.reserve(nw*nh);

	for (y=0; y<h; ++y)
	{
		if ((y%3) == 2) continue;
		for (x=0; x<w; ++x)
		{
			if ((x%3) == 2) continue;
			int i = y*w+x;
			pt2.push_back(pixeltypes[i]);
			p2.push_back(pixels[i]);
		}
	}
	hx = (hx*2)/3;
	hy = (hy*2)/3;

	pixels = p2;
	pixeltypes = pt2;

	w = nw;
	h = nh;
}

void Anim::CIS::Scale150h()
{
	std::vector<PixelType> pt2;
	std::vector<HSVA> p2;

	int x,y;
	int i=0;

	PixelType curr,next;
	HSVA hsv1, hsv2;

	int hh = 0;
	for (y=0; y<h; ++y)
	{
		++hh;
		if (((y%2)==0) && ((y+1)<h))
			++hh;
	}

	for (y=0; y<h; ++y)
	{
		i = y*w;
		for (x=0; x<w; ++x)
		{
			curr = pixeltypes[i];
			hsv1 = pixels[i];

			pt2.push_back(curr);
			p2.push_back(hsv1);
			++i;
		}

		if (((y%2)==0) && ((y+1)<h))
		{
			i=y*w;
			for (x=0; x<w; ++x)
			{
				curr = pixeltypes[i];
				hsv1 = pixels[i];
				next = pixeltypes[i+w];
				hsv2 = pixels[i+w];

				Pixels pp; Pix m;
				pp.push_back( Pix(curr,hsv1) );
				pp.push_back( Pix(next,hsv2) );
				if(HasMajority(pp,m))
				{
					pt2.push_back(m.pt);
					p2.push_back(m.hsv);
				} else {
					if (x>0)
					{
						pp.push_back(Pix(pixeltypes[i-1], pixels[i-1]));
						pp.push_back(Pix(pixeltypes[i+w-1], pixels[i+w-1]));
					}
					if ((x+1) < w)
					{
						pp.push_back(Pix(pixeltypes[i+1], pixels[i+1]));
						pp.push_back(Pix(pixeltypes[i+w+1], pixels[i+w+1]));
					}
					if (HasMajority(pp,m))
					{
						pt2.push_back(m.pt);
						p2.push_back(m.hsv);
					} else {
						pt2.push_back(curr);
						p2.push_back(hsv1);
					}
				}

				++i;
			}
		}
	}

	h = hh;

	hy += hy/2;

	pixeltypes.clear();
	pixeltypes.assign(pt2.begin(), pt2.end());

	pixels.clear();
	pixels.assign(p2.begin(), p2.end());
}

void Anim::CIS::Scale50w()
{
	CIS small;

	small.w = w/2;
	small.h = h;
	int n = small.w*small.h;
	small.pixeltypes.clear();
	small.pixeltypes.reserve(n);
	small.pixels.clear();
	small.pixels.reserve(n);

	small.has_colimp = false;
	small.has_dither = false;
	small.has_trans  = false;

	int x,y;

	auto mkpix = [&](PixelType pt, const std::vector<HSVA>& pix) -> void
	{
		if (pt==alpha)  small.has_dither = true;
		if (pt==trans)  small.has_trans  = true;
		if (pt==colimp) small.has_colimp = true;

		int h=0,s=0,v=0;
		for( const HSVA& hsv : pix )
		{
			h += hsv.h;
			s += hsv.s;
			v += hsv.v;
		}
		int nn = pix.size();
		HSVA newpix = { UC(h/nn), UC(s/nn), UC(v/nn), 0 };
		small.pixeltypes.push_back(pt);
		small.pixels.push_back(newpix);
	};

	for (y=0; y<small.h; ++y) for (x=0; x<small.w; ++x)
	{
		std::vector<HSVA> nrm, trn, cli, dth;
		for (int xx=0; xx<2; ++xx)
		{
			int x2 = x*2+xx;
			int y2 = y;
			int idx = y2*w+x2;
			switch (pixeltypes[idx])
			{
				case normal:  nrm.push_back(pixels[idx]);
				case alpha:   dth.push_back(pixels[idx]);
				case trans:   trn.push_back(pixels[idx]);
				case colimp:  cli.push_back(pixels[idx]);
			}
		}

		for (int n = 2; n>0; --n)
		{
			if (ssize(nrm) == n) { mkpix(normal, nrm); break; }
			if (ssize(trn) == n) { mkpix(trans,  trn); break; }
			if (ssize(cli) == n) { mkpix(colimp, cli); break; }
			if (ssize(dth) == n) { mkpix(alpha,  dth); break; }
		}
	}

	small.hx = hx/2;
	small.hy = hy;

	(*this) = small;
}

void Anim::CIS::Scale50h()
{
	CIS small;

	small.w = w;
	small.h = h/2;
	int n = small.w*small.h;
	small.pixeltypes.clear();
	small.pixeltypes.reserve(n);
	small.pixels.clear();
	small.pixels.reserve(n);

	small.has_colimp =false;
	small.has_dither =false;
	small.has_trans  =false;

	int x,y;

	auto mkpix = [&](PixelType pt, const std::vector<HSVA>& pix) -> void
	{
		if (pt==alpha)  small.has_dither =true;
		if (pt==trans)  small.has_trans  =true;
		if (pt==colimp) small.has_colimp =true;

		int h=0,s=0,v=0;
		for (const HSVA& hsv : pix)
		{
			h += hsv.h;
			s += hsv.s;
			v += hsv.v;
		}
		int nn = pix.size();
		HSVA newpix = { UC(h/nn), UC(s/nn), UC(v/nn), 0 };
		small.pixeltypes.push_back(pt);
		small.pixels.push_back(newpix);
	};

	for (y=0; y<small.h; ++y) for(x=0; x<small.w; ++x)
	{
		std::vector<HSVA> nrm, trn, cli, dth;
		for (int yy=0; yy<2; ++yy)
		{
			int x2 = x;
			int y2 = y*2+yy;
			int idx = y2*w+x2;
			switch (pixeltypes[idx])
			{
				case normal: nrm.push_back(pixels[idx]);
				case alpha:  dth.push_back(pixels[idx]);
				case trans:  trn.push_back(pixels[idx]);
				case colimp: cli.push_back(pixels[idx]);
			}
		}

		for (int n = 2; n>0; --n)
		{
			if (ssize(nrm) == n) { mkpix(normal, nrm); break; }
			if (ssize(trn) == n) { mkpix(trans,  trn); break; }
			if (ssize(cli) == n) { mkpix(colimp, cli); break; }
			if (ssize(dth) == n) { mkpix(alpha,  dth); break; }
		}
	}

	small.hx = hx;
	small.hy = hy/2;

	(*this) = small;
}

Anim::CIS Anim::CIS::HalfSize()
{
	CIS small;

	small.w = w/2;
	small.h = h/2;
	int n = small.w*small.h;
	small.pixeltypes.clear();
	small.pixeltypes.reserve(n);
	small.pixels.clear();
	small.pixels.reserve(n);

	small.has_colimp = false;
	small.has_dither = false;
	small.has_trans  = false;

	int x,y;

	auto mkpix = [&](PixelType pt, const std::vector<HSVA>& pix) -> void
	{
		if (pt==alpha)  small.has_dither = true;
		if (pt==trans)  small.has_trans  = true;
		if (pt==colimp) small.has_colimp = true;

		int h=0,s=0,v=0;
		for (const HSVA& hsv : pix)
		{
			h += hsv.h;
			s += hsv.s;
			v += hsv.v;
		}
		int nn = pix.size();
		HSVA newpix = { UC(h/nn), UC(s/nn), UC(v/nn), 0 };
		small.pixeltypes.push_back(pt);
		small.pixels.push_back(newpix);
	};

	for (y=0; y<small.h; ++y) for (x=0; x<small.w; ++x)
	{
		std::vector<HSVA> nrm, trn, cli, dth;
		for (int yy=0; yy<2; ++yy) for (int xx=0; xx<2; ++xx)
		{
			int x2 = x*2+xx;
			int y2 = y*2+yy;
			int idx = y2*w+x2;
			switch(pixeltypes[idx])
			{
				case normal:  nrm.push_back(pixels[idx]);
				case alpha:   dth.push_back(pixels[idx]);
				case trans:   trn.push_back(pixels[idx]);
				case colimp:  cli.push_back(pixels[idx]);
			}
		}

		for (int n = 4; n>0; --n)
		{
			if (ssize(nrm) == n) { mkpix(normal, nrm); break; }
			if (ssize(trn) == n) { mkpix(trans,  trn); break; }
			if (ssize(cli) == n) { mkpix(colimp, cli); break; }
			if (ssize(dth) == n) { mkpix(alpha,  dth); break; }
		}
	}

	small.hx = hx/2;
	small.hy = hy/2;

	return small;
}

Anim::CIS Anim::CIS::CutOut(int x1,int y1,int ww,int hh)
{
	assert((x1>=0) && (y1>=0));
	assert((x1+ww) <= w);
	assert((y1+hh) <= h);
	CIS c2;
	c2.w = ww;
	c2.h = hh;
	c2.hx = c2.hy = 0;
	c2.has_colimp = c2.has_dither = c2.has_trans = false;
	int x,y, sz = ww*hh;
	c2.pixels.clear(); c2.pixels.reserve(sz);
	c2.pixeltypes.clear(); c2.pixeltypes.reserve(sz);
	for (y=0; y<hh; ++y) for (x=0; x<ww; ++x)
	{
		int idx1 = x1+x + (y1+y)*w;
		PixelType pt = pixeltypes[idx1];
		if (pt==alpha)  c2.has_dither = true;
		if (pt==colimp) c2.has_colimp = true;
		if (pt==trans)  c2.has_trans  = true;
		c2.pixeltypes.push_back(pt);
		c2.pixels.push_back(pixels[idx1]);
	}
	return c2;
}

void Anim::CIS::Crop()
{
	int W = w;
	auto xy = [&W](int x,int y) -> int { return y*W + x; } ;
	int topcrop = 0;
	while (topcrop<h)
	{
		bool empty=true;
		for (int x=0; empty&&(x<w); ++x)
			if (pixeltypes[xy(x,topcrop)] != trans)
				empty = false;
		if (empty) ++topcrop; else break;
	}
	if (topcrop==h) { w=h=0; pixeltypes.clear(); pixels.clear(); return; }
	int botcrop = 0;
	while (true)
	{
		bool empty=true;
		for (int x=0;empty&&(x<w);++x)
			if (pixeltypes[xy(x,h-botcrop-1)] != trans)
				empty=false;
		if(empty) ++botcrop; else break;
	}
	int lftcrop = 0;
	while (true)
	{
		bool empty=true;
		for (int y=0;empty&&(y<h);++y)
			if (pixeltypes[xy(lftcrop,y)] != trans)
				empty=false;
		if (empty) ++lftcrop; else break;
	}
	int rghcrop = 0;
	while (true)
	{
		bool empty=true;
		for (int y=0;empty&&(y<h);++y)
			if (pixeltypes[xy(w-rghcrop-1,y)] != trans)
				empty=false;
		if (empty) ++rghcrop; else break;
	}
	int new_w = w - lftcrop - rghcrop;
	int new_h = h - topcrop - botcrop;
	assert((new_w>0) && (new_w<=w) && (new_h>0) && (new_h<=h));
	std::vector<PixelType> new_pt; new_pt.reserve(new_w*new_h);
	std::vector<HSVA>      new_px; new_px.reserve(new_w*new_h);
	for (int y=0; y<new_h; ++y) for (int x=0; x<new_w; ++x)
	{
		int idx = xy(x+lftcrop, y+topcrop);
		new_pt.push_back(pixeltypes [idx]);
		new_px.push_back(pixels     [idx]);
	}
	pixeltypes. clear(); pixeltypes. assign(new_pt.begin(), new_pt.end());
	pixels.     clear(); pixels.     assign(new_px.begin(), new_px.end());
	w = new_w; h = new_h;
	hx -= lftcrop; hy -= topcrop;
}

//void Anim::CIS::FromImg( SDL_Surface* image, SDL_Surface* transmask, SDL_Surface* dithermask, SDL_Surface* colimpmask )
//{
//	assert(image);
//	w = image->w;
//	h = image->h;
//	hx=0; //w/2;
//	hy=0; //(2*h)/3;
//
//	has_dither = dithermask;
//	has_trans  = transmask;
//	has_colimp = colimpmask;
//
//	int n = w*h;
//	pixels.clear();
//	pixeltypes.clear();
//	pixels.reserve(n);
//	pixeltypes.reserve(n);
//
//	int x,y;
//	for(y=0;y<h;++y)
//	{
//		for(x=0;x<w;++x)
//		{
//			if(colimpmask && (grayscale( GetPixel(colimpmask,x,y) ) > 127) )
//			{
//				pixeltypes.push_back(colimp);
//				HSVA hsv = RGBA_2_HSVA( GetPixel(image,x,y) );
//				pixels.push_back(hsv);
//			}
//			else if(transmask && (grayscale( GetPixel(transmask,x,y) ) < 127) )
//			{
//				pixeltypes.push_back(trans);
//				HSVA hsv = { 0,0,0,255 };
//				pixels.push_back(hsv);
//			}
//			else if(dithermask && (grayscale( GetPixel(dithermask,x,y) ) > 127) )
//			{
//				pixeltypes.push_back(alpha);
//				HSVA hsv = RGBA_2_HSVA( GetPixel(image,x,y) );
//				hsv.a = 128;
//				pixels.push_back(hsv);
//			}
//			else
//			{
//				pixeltypes.push_back(normal);
//				HSVA hsv = RGBA_2_HSVA( GetPixel(image,x,y) );
//				pixels.push_back(hsv);
//			}
//		}
//	}
//}

sf::Texture Anim::CIS::MakeSurface()
{
	std::vector<UC> data;
	data.reserve(4*w*h);

	int i=0;
	for(int y=0; y<h; ++y)
	{
		for (int x=0; x<w; ++x)
		{
			RGBA pp = HSVA_2_RGBA(pixels[i]);

			switch (pixeltypes[i])
			{
			case alpha:
				if(((x+y-hx-hy)%2) == 0)
					pp = {255,255,255,0};
				break;
			case trans:
				pp = {255,255,255,0};
				break;
			default:
				break;
			}
			data.push_back(pp.r);
			data.push_back(pp.g);
			data.push_back(pp.b);
			data.push_back(pp.a);
			++i;
		}
	}

	sf::Image img;
	img.create(w, h, data.data());
	sf::Texture tx;
	tx.loadFromImage(img);
	return tx;
}


sf::Texture Anim::CIS::MakeSurface(UC hue)
{
	std::vector<UC> data;
	data.reserve(4*w*h);

	int i=0;
	for(int y=0; y<h; ++y)
	{
		for (int x=0; x<w; ++x)
		{
			RGBA pp;

			switch (pixeltypes[i])
			{
			case alpha:
				if(((x+y-hx-hy)%2) == 0)
					pp = {255,255,255,0};
				break;
			case trans:
				pp = {255,255,255,0};
				break;
			case colimp:
				pixels[i].h = hue;
				pp = HSVA_2_RGBA(pixels[i]);
				break;
			default:
				pp = HSVA_2_RGBA(pixels[i]);
				break;
			}
			data.push_back(pp.r);
			data.push_back(pp.g);
			data.push_back(pp.b);
			data.push_back(pp.a);
			++i;
		}
	}

	sf::Image img;
	img.create(w, h, data.data());
	sf::Texture tx;
	tx.loadFromImage(img);
	return tx;
}

sf::Texture Anim::CIS::MakeSurface(UC alp, UC hue) // blended
{
	std::vector<UC> data;
	data.reserve(4*w*h);

	int i=0;
	for(int y=0; y<h; ++y)
	{
		for (int x=0; x<w; ++x)
		{
			RGBA pp;

			switch (pixeltypes[i])
			{
			case alpha:
				pixels[i].a = alp;
				pp = HSVA_2_RGBA(pixels[i]);
				break;
			case trans:
				pp = {255,255,255,0};
				break;
			case colimp:
				pixels[i].h = hue;
				pp = HSVA_2_RGBA(pixels[i]);
				break;
			default:
				pp = HSVA_2_RGBA(pixels[i]);
				break;
			}
			data.push_back(pp.r);
			data.push_back(pp.g);
			data.push_back(pp.b);
			data.push_back(pp.a);
			++i;
		}
	}

	sf::Image img;
	img.create(w, h, data.data());
	sf::Texture tx;
	tx.loadFromImage(img);
	return tx;
}

void Anim::CIS::Unimport()
{
	int i,sz = w*h;
	for (i=0; i<sz; ++i)
	{
		if (pixeltypes[i] == colimp)
		{
			pixeltypes[i] = normal;
		}
	}
}

void Anim::CIS::MakeDark()
{
	int i,sz = w*h;
	for (i=0; i<sz; ++i)
	{
		if (pixeltypes[i] == colimp)
		{
			int diff = pixels[i].v;
			diff /= 2;
			pixels[i].v = diff;
			/*diff = 255 - pixels[i].s;
			diff /= 2;
			pixels[i].s = diff;*/
		}
	}
}

void Anim::CIS::MakeWhite()
{
	int i,sz = w*h;
	for (i=0; i<sz; ++i)
	{
		if (pixeltypes[i] == colimp)
		{
			int diff = 255 - pixels[i].v;
			diff /= 2;
			pixels[i].v = 255-diff;
			diff = pixels[i].s;
			diff /= 2;
			pixels[i].s = diff;
		}
	}
}

Anim::CIS Anim::CIS::Flip(bool fx, bool fy, bool rot)
{
	CIS cis;

	auto& pt = cis.pixeltypes;
	auto& px = cis.pixels;

	if (rot)
	{
		cis.w =h;   cis.h =w;
		cis.hx=hy;  cis.hy=hx;
	} else {
		cis.w =w;   cis.h =h;
		cis.hx = fx ? w-hx : hx;
		cis.hy = fy ? h-hy : hy;
	}
	cis.has_dither	= has_dither ;
	cis.has_trans	= has_trans	 ;
	cis.has_colimp	= has_colimp ;

	int x,y, i, n = w*h;

	pt.clear(); pt.reserve(n);
	px.clear(); px.reserve(n);

	for (i=0; i<n; ++i)
	{
		if (rot)
		{
			x = fx ? h-(i%h)-1 : (i%h);
			y = fy ? w-(i/h)-1 : (i/h);
		} else {
			x = fx ? w-(i%w)-1 : (i%w);
			y = fy ? h-(i/w)-1 : (i/w);
		}
		pt.push_back(pixeltypes [x + y*w]);
		px.push_back(pixels     [x + y*w]);
	}
	return cis;
}

Anim::Surface& Anim::CIS::Get(UC hue)
{
	std::map<UC,Surface>::iterator iter;
	if (!has_colimp)
		iter = instance.begin();
	else
		iter = instance.find(hue);
	assert(iter != instance.end());
	return iter->second;
}

void Anim::CIS::Instance(UC hue)
{
	if (!has_colimp)
	{
		if (instance.empty())
		{
			instance[hue].FromCIS(*this, 127, hue);
		}
	} else {
		if (instance.find(hue) == instance.end())
			instance[hue].FromCIS(*this, 127, hue);
	}
}

void Anim::CIS::FreeData()
{
	pixeltypes.clear();
	pixels.clear();
}

void Anim::CIS::UnInstance()
{
	for (auto itr : instance)
		itr.second.Free();
	instance.clear();
}

// **********************
// *** AnimReflection ***
// **********************

namespace Anim {
	sf::Clock clock;
}

void Anim::Init()
{
	clock.restart();
}

bool Anim::AnimReflection::Next()
{
	if (!ba) return false;
	time = 0;
	if (++current >= ba->Size())
	{
		current = 0;
		++loopcnt;
		if (!ba->repeating) return false;
	}
	return true;
}

void Anim::AnimReflection::Start()
{
	last = clock.getElapsedTime().asMilliseconds();
	time = 0;
}

bool Anim::AnimReflection::Update()
{
	auto ii = clock.getElapsedTime().asMilliseconds();
	bool ok = Update(ii - last);
	last = ii;
	return ok;
}

bool Anim::AnimReflection::Update(int ms)
{
	if ((!ba) || (!ba->delay)) return false;
	if (ms<0) return false;
	time += ms;
	if (time < ba->delay)
		return false;
	while (time >= ba->delay)
	{
		++current;
		time -= ba->delay;
	}
	if (current >= ba->Size())
	{
		loopcnt += (current / ba->Size());
		current = current % ba->Size();
		if (!ba->repeating) return false;
	}
	return true;
}

void Anim::AnimReflection::Overlay(sf::RenderTarget& rt, int x, int y)
{
	Surface* srf;
	/**/ if (cis)
		srf = &cis->Get(hue);
	else if (ba)
		srf = &ba->Get(current, hue);
	else
		return;

	sf::Texture& tx = srf->SFML();
	sf::Sprite sprite;
	sprite.setTexture(tx);
	sprite.setPosition((float)(x-srf->Hot().x), (float)(y-srf->Hot().y));
	rt.draw(sprite);
}

void Anim::AnimReflection::ContinueWith(const AnimReflection& ar)
{
	cis = ar.cis;
	ba = ar.ba;
	hue = ar.hue;
	if (ba)
	{
		auto n = ba->Size();
		if (n)
			current = current % n;
		else
			current = 0;
	} else {
		current = 0;
	}
}

void Anim::AnimReflection::Set(BasicAnim* b, UC h)
{
	clr(); ba=b; hue=h;
}

void Anim::AnimReflection::Set(CIS* c, UC h)
{
	clr(); cis=c; hue=h;
}

Anim::AnimReflection::AnimReflection() { clr(); }
Anim::AnimReflection::AnimReflection(BasicAnim* b, UC h) { Set(b, h); }
Anim::AnimReflection::AnimReflection(CIS* c, UC h) { Set(c, h); }

void Anim::AnimReflection::clr()
{
	cis = nullptr;
	ba = nullptr;
	last = clock.getElapsedTime().asMilliseconds();
	current = time = loopcnt = 0;
	hue = 0;
}

// *****************
// *** BasicAnim ***
// *****************

bool Anim::BasicAnim::LoadExt(std::string fn)
{
	std::string ext = ExtractFileExt(fn);
	if (ext=="ba")
	{
		std::ifstream ifs(fn,std::ios::in|std::ios::binary);
		Load(ifs);
		return true;
	} else {
		CIS cis;
		if (cis.LoadExt(fn))
		{
			anim.clear();
			anim.push_back(cis);
			return true;
		}
	}
	return false;
}

Anim::AnimReflection Anim::BasicAnim::Refl(UC hue)
{
	AnimReflection ar(this, hue);
	return ar;
}

Anim::Surface& Anim::BasicAnim::Get(int f, UC hue)
{
	assert (f>=0);
	assert (f<(int)anim.size());
	return anim[f].Get(hue);
}

void Anim::BasicAnim::Instance(UC hue)
{
	for (CIS& cis : anim)
		cis.Instance(hue);
}

void Anim::BasicAnim::UnInstance()
{
	for (CIS& cis : anim)
		cis.UnInstance();
}

void Anim::BasicAnim::FreeData()
{
	for (CIS& cis : anim)
		cis.FreeData();
}

void Anim::BasicAnim::SaveInternal(std::ostream& os)
{
	std::int16_t val = anim.size();
	WriteBinary(os, val);
	for (int i=0; i<val; ++i)
		anim[i].Save(os);
}

void Anim::BasicAnim::LoadInternal(std::istream& is, bool old)
{
	std::int16_t val;
	ReadBinary(is, val);
	anim.clear();
	for (int i=0; i<val; ++i)
	{
		anim.emplace_back();
		CIS& cis = anim.back();
		if (old)
			cis.LoadOld(is);
		else
			cis.Load(is);
	}
}

void Anim::BasicAnim::LoadOld(std::istream& is)
{
	ReadBinary(is, delay);
	std::int16_t val;
	ReadBinary(is, val);
	repeating = (bool) val;
	jbf = 0;
	LoadInternal(is,true);
}

void Anim::BasicAnim::SaveOld(std::ostream& os)
{
	WriteBinary(os, delay);
	std::int16_t val = (short)repeating;
	WriteBinary(os, val);
	SaveInternal(os);
}

void Anim::BasicAnim::Load(std::istream& is)
{
	char buff[5] = {};
	is.read(buff, 4);
	if (std::string(buff) != "BA_2")
	{
		is.seekg(-4, std::ios_base::cur);
		//is.putback(buff[3]).putback(buff[2]).putback(buff[1]).putback(buff[0]);
		LoadOld(is);
	} else {
		ReadBinary(is, delay);
		std::int16_t val;
		ReadBinary(is, val);
		repeating = (bool) (val>>15);
		jbf = val & 0x7FFF;
		LoadInternal(is);
	}
}

void Anim::BasicAnim::Save(std::ostream& os)
{
	os.write("BA_2",4);
	WriteBinary(os, delay);
	std::int16_t val = (std::int16_t)repeating;
	val = val << 15;
	val = val | jbf;
	WriteBinary(os, val);
	SaveInternal(os);
}

void Anim::BasicAnim::MakeMirror(BasicAnim& ba, bool mx, bool my, bool rot)
{
	delay = ba.delay;
	repeating = ba.repeating;
	anim.clear();
	for (CIS& cis : ba.anim)
		anim.push_back(cis.Flip(mx, my, rot));
}


// ***************
// *** AnimDir ***
// ***************

bool Anim::AnimDir::LoadExt(std::string fn)
{
	std::string ext = ExtractFileExt(fn);
	if (ext=="ad")
	{
		std::ifstream ifs(fn,std::ios::in|std::ios::binary);
		Load(ifs);
		return true;
	} else {
		BAD bd;
		if (bd.LoadExt(fn))
		{
			bad.clear();
			bd.dir=0; bd.mirror=false;
			bad.push_back(bd);
			return true;
		}
	}
	return false;
}


void Anim::AnimDir::Instance(UC hue)
{
	for (BAD& b : bad)
		b.Instance(hue);
}

void Anim::AnimDir::UnInstance()
{
	for (BAD& b : bad)
		b.UnInstance();
}

void Anim::AnimDir::FreeData()
{
	for (BAD& b : bad)
		b.FreeData();
}

void Anim::AnimDir::LoadInternal(std::istream& is,bool old)
{
	std::int16_t i,n;
	ReadBinary(is,n);
	bad.clear();
	bad.reserve(n);
	for (i=0; i<n; ++i)
	{
		bad.push_back(BAD());
		BAD& b = bad.back();
		ReadBinary(is, b.dir);
		UC uc;
		ReadBinary(is, uc);
		if (uc)
		{
			b.mirror = true;
			b.flipx = uc&2;
			b.flipy = uc&4;
			b.rot90 = uc&8;
			ReadBinary(is, b.mirrorof);
		} else {
			if (old)
				b.LoadOld(is);
			else
				b.Load(is);
			b.mirror = false;
		}
	}
	Mirror();
}

void Anim::AnimDir::LoadOld(std::istream& is) { LoadInternal(is, true); }

void Anim::AnimDir::Load(std::istream& is)
{
	char buff[5] = {};
	is.read(buff,4);
	if (std::string(buff) != "AD_2")
	{
		is.seekg(-4, std::ios_base::cur);
		//is.putback(buff[3]).putback(buff[2]).putback(buff[1]).putback(buff[0]);
		LoadOld(is);
	} else {
		LoadInternal(is);
	}
}

void Anim::AnimDir::SaveInternal(std::ostream& os, bool)
{
	short i,n = bad.size();
	WriteBinary(os, n);
	for (i=0; i<n; ++i)
	{
		BAD& b = bad[i];
		WriteBinary(os, b.dir);
		if (!b.mirror)
		{
			WriteBinary(os, (UC)0);
			b.Save(os);
		} else {
			UC uc = 1;
			if (b.flipx) uc|=2;
			if (b.flipy) uc|=4;
			if (b.rot90) uc|=8;
			WriteBinary(os, uc);
			WriteBinary(os, b.mirrorof);
		}
	}
}

void Anim::AnimDir::SaveOld(std::ostream& os) { SaveInternal(os); }

void Anim::AnimDir::Save(std::ostream& os)
{
	os.write("AD_2",4);
	SaveInternal(os);
}

Anim::AnimDir::BAD* Anim::AnimDir::findexact(short d)
{
	for (BAD& bd : bad)
	{
		if (bd.dir == d) return &bd;
	}
	return 0;
}

/*
int Anim::AnimDir::UseAsFont( SDL_Surface* d, Pos p, UC hue, string s)
{
	int xx = 0;
	for(char c : s)
	{
		UC cc = static_cast<UC>(c);
		BAD* b = findexact(cc);
		if(!b) continue;
		Surface& srf = b->Get(0,hue);
		if(d) srf.Overlay(d,p.x+xx,p.y);
		xx+=srf.Width();
	}
	return xx;
}
*/

Anim::AnimReflection Anim::AnimDir::Refl(short dir, UC hue)
{
	return Closest(dir).Refl(hue);
}

Anim::BasicAnim& Anim::AnimDir::Closest(short dir)
{
	return BestFit<BasicAnim>(dir,bad);
}

/*void Anim::AnimDir::Scale150()
{
	for(BAD& a:bad)
		if(!a.mirror)
			a.Scale150();
	Mirror();
}*/

void Anim::AnimDir::Mirror()
{
	for (BAD& a:bad)
		if (a.mirror)
			a.MakeMirror(Closest(a.mirrorof), a.flipx, a.flipy, a.rot90);
}

// ***********
// *** NAV ***
// ***********

std::string ExtractFileNameOnly(std::string fn)
{
	auto p1 = fn.find_last_of("/\\");
	if (p1==std::string::npos) p1=0;
	auto p2 = fn.find_last_of(".");
	if (p2<p1) p2 = std::string::npos;
	if (p2==std::string::npos)
		return fn.substr(p1+1);
	else
		return fn.substr(p1+1, p2-p1);
}

bool Anim::NAV::LoadExt(std::string fn)
{
	std::string ext = ExtractFileExt(fn);
	if (ext=="nav")
	{
		std::ifstream ifs(fn,std::ios::in|std::ios::binary);
		Load(ifs);
		return true;
	} else {
		AnimDir ad;
		if (ad.LoadExt(fn))
		{
			variants.clear();
			variants.push_back(ad);
			name = ExtractFileNameOnly(fn);
			return true;
		}
	}
	return false;
}

void Anim::NAV::Instance(UC hue)
{
	for (AnimDir& ad : variants)
		ad.Instance(hue);
}

void Anim::NAV::UnInstance()
{
	for (AnimDir& ad : variants)
		ad.UnInstance();
}

void Anim::NAV::FreeData()
{
	for (AnimDir& ad : variants)
		ad.FreeData();
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

static void SaveStr(std::ostream& os, std::string str)
{
	os.write(str.c_str(), str.size()+1);
}

void Anim::NAV::LoadInternal(std::istream& is, bool old)
{
	name = LoadStr(is);
	char vars;
	ReadBinary(is,vars);
	variants.clear();
	for (int i=0; i<vars; ++i)
	{
		variants.emplace_back();
		if (old)
			variants.back().LoadOld(is);
		else
			variants.back().Load(is);
	}
}

void Anim::NAV::SaveInternal(std::ostream& os)
{
	SaveStr(os, name);
	char n = variants.size();
	WriteBinary(os, n);
	for (AnimDir& ad : variants)
		ad.Save(os);
}

void Anim::NAV::LoadOld(std::istream& is) { LoadInternal(is, true); }
void Anim::NAV::SaveOld(std::ostream& os) { SaveInternal(os); }

void Anim::NAV::Save(std::ostream& os)
{
	os.write("NAV2",4);
	SaveInternal(os);
}

void Anim::NAV::Load(std::istream& is)
{
	char buff[5] = {};
	is.read(buff,4);
	if (std::string(buff) != "NAV2")
	{
		is.seekg(-4, std::ios_base::cur);
		//is.putback(buff[3]).putback(buff[2]).putback(buff[1]).putback(buff[0]);
		LoadOld(is);
	} else {
		LoadInternal(is);
	}
}

auto Anim::NAV::Refl(short dir, UC hue)
	-> AnimReflection
{
	int i = rand() % variants.size();
	AnimReflection ar( & variants[i].Closest(dir), hue );
	//ia.FromBasic( variants[i].Closest(dir), hue );
	//ar.FromIA(ia);
	return ar;
}

// **********************
// *** AnimCollection ***
// **********************

bool Anim::AnimCollection::LoadExt(std::string fn)
{
	std::string ext = ExtractFileExt(fn);
	if (ext == "ac")
	{
		std::cout << "loading file " << fn << " ext " << ext << std::endl;
		std::ifstream ifs(fn, std::ios::in|std::ios::binary);
		Load(ifs);
		return true;
	} else {
		NAV* nav = new NAV();
		if (nav->LoadExt(fn))
		{
			FreeData();
			core.push_back(nav);
			mappings[nav->name] = nav;
			return true;
		}
		delete nav;
	}
	return false;
}

void Anim::AnimCollection::Instance(UC hue)
{
	for (NAV* nav : core)
		nav->Instance(hue);
}

void Anim::AnimCollection::UnInstance()
{
	for (NAV* nav : core)
		nav->UnInstance();
}

void Anim::AnimCollection::FreeData()
{
	for (NAV* nav : core)
		nav->FreeData();
}

Anim::AnimCollection::~AnimCollection()
{
	UnInstance();
	for (NAV* nav : core)
		delete nav;
}

auto Anim::AnimCollection::Refl(std::string name, short dir, UC hue)
	-> AnimReflection
{
	auto itr = mappings.find(name);
	auto end = mappings.end();
	if ((itr==end) && default_anim.size())
		itr = mappings.find(default_anim);
	if (itr==end)
		itr = mappings.find("default");
	if (itr==end)
		itr = mappings.find("idle");
	if (itr==end)
		itr = mappings.begin();
	assert(itr!=end);
	return itr->second->Refl(dir, hue);
}

void Anim::AnimCollection::AddVariant(std::string name, AnimDir ad)
{
	auto itr = mappings.find(name);
	if (itr == mappings.end())
	{
		NAV* nav = new NAV();
		core.push_back(nav);
		nav->name = name;
		nav->variants.clear();
		nav->variants.push_back(ad);
		mappings[name] = nav;
	} else {
		itr->second->variants.push_back(ad);
	}
}

void Anim::AnimCollection::LoadInternal(std::istream& is, bool old)
{
	core.clear();
	mappings.clear();
	char n;
	ReadBinary(is,n);
	core.reserve(n);
	for (int i=0; i<n; ++i)
	{
		NAV* nav = new NAV();
		core.push_back(nav);
		if (old)
			nav->LoadOld(is);
		else
			nav->Load(is);
		mappings[nav->name] = nav;
	}
	ReadBinary(is,n);
	for (int i=0;i<n;++i)
	{
		std::string name  = LoadStr(is);
		std::string mapof = LoadStr(is);
		auto itr = mappings.find(mapof);
		assert(itr != mappings.end());
		mappings[name] = itr->second;
	}
}

void Anim::AnimCollection::SaveInternal(std::ostream& os)
{
	char n = core.size();
	WriteBinary(os, n);
	for (NAV* nav : core)
	{
		nav->Save(os);
	}
	char n2 = mappings.size() - n;
	WriteBinary(os,n2);
	if (!n2) return;
	for (auto itr : mappings)
	{
		if (itr.first == itr.second->name) continue;
		SaveStr(os, itr.first);
		SaveStr(os, itr.second->name);
		--n2;
	}
	assert(!n2);
}


void Anim::AnimCollection::LoadOld(std::istream& is) { LoadInternal(is, true); }
void Anim::AnimCollection::SaveOld(std::ostream& os) { SaveInternal(os); }

void Anim::AnimCollection::Save(std::ostream& os)
{
	os.write("AC_2",4);
	SaveInternal(os);
	SaveStr(os, default_anim);
}

void Anim::AnimCollection::Load(std::istream& is)
{
	char buff[5] = {};
	std::cout << "Loading AC, at pos " << is.tellg() << std::endl;
	is.read(buff,4);
	if (std::string(buff) != "AC_2")
	{
		is.seekg(-4, std::ios_base::cur);
		//is.putback(buff[3]).putback(buff[2]).putback(buff[1]).putback(buff[0]);
		LoadOld(is);
		default_anim.clear();
	} else {
		LoadInternal(is);
		default_anim = LoadStr(is);
	}
}

std::vector<std::string> Anim::AnimCollection::CoreNames()
{
	std::vector<std::string> sl;
	for (NAV* nav : core)
	{
		sl.push_back(nav->name);
	}
	return sl;
}

std::vector<std::string> Anim::AnimCollection::AllNames()
{
	std::vector<std::string> sl;
	for (auto itr : mappings)
	{
		sl.push_back(itr.first);
	}
	return sl;

	DoAll<&NAV::DoAll<&AnimDir::DoAll<&BasicAnim::DoAll<&CIS::Crop>>>>();

}

