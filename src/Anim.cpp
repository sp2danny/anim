
// --------------------------------------------

#include "Anim.h"

using namespace std;

template<typename T,typename U>
T& BestFit( std::int16_t dir, U& vec )
{
	auto dirdiff = [](std::int16_t d1,std::int16_t d2) -> short
	{
		std::int16_t df = abs(d1-d2);
		if (df>180) df=360-180;
		return df;
	};

	int idx=0;
	int n = vec.size();
	assert(n);
	int srt = dirdiff( dir, vec[0].dir );
	for (int i=1;i<n;++i)
	{
		int s = dirdiff( dir, vec[i].dir );
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

C35::RGBA::RGBA()
	: r(0),g(0),b(0),a(0)
{
}

C35::RGBA::RGBA(UC _r,UC _g,UC _b)
	: r(_r), g(_g), b(_b), a(0)
{
}
	
C35::RGBA::RGBA(UC _r,UC _g,UC _b,UC _a)
	: r(_r), g(_g), b(_b), a(_a)
{
}

// ***************
// *** Surface ***
// ***************

C35::Pos C35::Surface::Hot() const
{
	return Pos(hx,hy);
}
void C35::Surface::Hot(const Pos& p) { hx=p.x; hy=p.y; }

void C35::Surface::Overlay( SDL_Surface* dst, int x,int y) const
{
	if( surface && dst )
	{
		SDL_Rect dr = { (short)(x-hx), (short)(y-hy), (unsigned short)(surface->w), (unsigned short)(surface->h) };
		SDL_BlitSurface( surface, 0, dst, &dr );
	}
}

void C35::Surface::Overlay( Surface& dst, int x,int y ) const
{
	if( surface && dst.surface )
	{
		SDL_Rect dr = { (short)(x-hx), (short)(y-hy), (unsigned short)(surface->w), (unsigned short)(surface->h) };
		SDL_BlitSurface( surface, 0, dst.surface, &dr );
	}

}

void C35::Surface::Create(int w,int h)
{
	surface = MakeSurface(w,h,SDL_GetVideoSurface()->format );
	hx=hy=0;
}

void C35::Surface::FromCIS(CIS& cis)
{
	surface = cis.MakeSurface();
	hx = cis.hx; hy=cis.hy;
}

void C35::Surface::FromCIS(CIS& cis,UC hue)
{
	surface = cis.MakeSurface(hue);
	hx = cis.hx; hy=cis.hy;
}

void C35::Surface::FromBMP(char* bmp)
{
	surface = SDL_LoadBMP(bmp);
	hx=hy=0;
}

void C35::Surface::FromBMP(char* bmp,RGBA ck)
{
	surface = SDL_LoadBMP(bmp);
	hx=hy=0;
	if(surface)
	{
		Uint32 colk = SDL_MapRGB(surface->format, ck.r, ck.g, ck.a);
		SDL_SetColorKey(surface,SDL_SRCCOLORKEY,colk);
	}
}

void  C35::Surface::Free()
{
	if(surface)
		SDL_FreeSurface(surface);
	surface=0;
}

C35::Surface C35::Surface::Screen()
{
	Surface s;
	s.surface = SDL_GetVideoSurface();
	s.hx = s.hy = 0;
	return s;
}

int C35::Surface::Width()  const { return surface->w; }
int C35::Surface::Height() const { return surface->h; }

// ***********
// *** CIS ***
// ***********

C35::AnimReflection C35::CIS::Refl(UC hue)
{
	AnimReflection ar(this,hue);
	return ar;
}

string ExtractFileExt(std::string fn)
{
	auto p = fn.find_last_of('.');
	if(p==string::npos) return "";
	string ret = fn.substr(p+1);
	for( char& c : ret ) c=tolower(c);
	return ret;
}

string ExtractFileBase(std::string fn)
{
	auto p = fn.find_last_of('.');
	if(p==string::npos) return fn;
	string ret = fn.substr(0,p);
	//for( char& c : ret ) c=tolower(c);
	return ret;
}

extern C35::CIS LoadPCX(const char* fn);

bool C35::CIS::LoadExt(string fn)
{
	string ext = ExtractFileExt(fn);
	if(ext=="cis")
	{
		ifstream ifs(fn,ios::in|ios::binary);
		if(ifs.bad()) return false;
		Load(ifs);
		return true;
	}
	else if(ext=="bmp")
	{
		SDL_Surface* srf = SDL_LoadBMP(fn.c_str());
		if(!srf) return false;
		SDL_Surface* tm = MakeTransMaskFromImage(srf);
		FromImg(srf,tm,0,0);
		return true;
	}
	else if(ext=="pcx")
	{
		LoadPCX(fn.c_str());
		return true;
	}
	return false;
}
/*
void C35::CIS::DummySave()
{
	static int ii = 1;
	char buff[256];
	sprintf(buff,"tst/img_%04d.bmp",ii++);
	Surface srf;
	srf.FromCIS(*this,0);
	SDL_SaveBMP( srf.surface, buff );
	srf.Free();
}*/

void C35::CIS::LoadInternal( istream& is )
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

	for(i=0;i<sz;++i)
	{
		if(!leftbit)
		{
			ReadBinary(is,c);
			leftbit=8;
		}

		PixelType pt = (PixelType) (c&3);
		leftbit -= 2;
		c = c >> 2;

		pixeltypes.push_back(pt);

		switch(pt)
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
		if(!odd)
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
		/**/ if( this->depth==4 ) return GetC_4(is);
		else if( this->depth==6 ) return GetC_6(is);
		else if( this->depth==8 ) return GetC_8(is);
		else { assert(false); return 0; }
	};

	pixels.clear();
	pixels.reserve(sz);
	for(i=0;i<sz;++i)
	{
		HSVA p = { 0,0,0,255 };
		switch(pixeltypes[i])
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

void C35::CIS::SaveInternal( ostream& os )
{
	unsigned int i,sz = w*h;

	assert(sz<=pixeltypes.size());
	unsigned char c=0;
	int leftbits = 0;
	for(i=0;i<sz;++i)
	{
		if(leftbits==8)
		{
			WriteBinary( os, c );
			leftbits = 0;
			c=0;
		}
		PixelType& pt = pixeltypes[i];
		c = c | ( (pt&3) << leftbits );
		leftbits += 2;

		assert( (pt&3) == pt );
	}
	if(leftbits!=0) WriteBinary( os, c );

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
			WriteBinary(os,cc);
		}
	};
	auto Rem = [&odd,&cc](std::ostream& os) -> void
	{
		if(odd) WriteBinary(os,cc);
	};
	auto PutC_8 = [](std::ostream& os,UC uc) -> void
	{
		WriteBinary(os,uc);
	};
	auto PutC_6 = [&](std::ostream& ,UC ) -> void
	{
		// later
	};

	auto PutC = [&](std::ostream& os,UC uc) -> void
	{
		switch(this->depth)
		{
			case 4: PutC_4(os,uc); break;
			case 6: PutC_6(os,uc); break;
			case 8: PutC_8(os,uc); break;
			default: assert(false);
		}
	};

	assert(sz<=pixels.size());
	for(i=0;i<sz;++i)
	{
		PixelType& pt = pixeltypes[i];
		HSVA& p = pixels[i];
		switch(pt)
		{
			case trans: break;
			case normal: PutC(os, p.h ); PutC(os, p.s ); PutC(os, p.v ); break;
			case alpha:  PutC(os, p.h ); PutC(os, p.s ); PutC(os, p.v ); break;
			case colimp:                 PutC(os, p.s ); PutC(os, p.v ); break;
			default:
				assert(false);
		}
	}
	Rem(os);
}

void C35::CIS::LoadOld( istream& is )
{
	ReadBinary( is, w );
	ReadBinary( is, h );
	ReadBinary( is, hx );
	ReadBinary( is, hy );
	depth=4;
	LoadInternal(is);
}

void C35::CIS::SaveOld( ostream& os )
{
	WriteBinary( os, w );
	WriteBinary( os, h );
	WriteBinary( os, hx );
	WriteBinary( os, hy );
	depth=4;
	SaveInternal(os);
}

void C35::CIS::Load( istream& is )
{
	char buff[5] = {};

	is.read(buff,4);
	if( string(buff) != "CIS2" )
	{
		is.seekg(-4, ios_base::cur);
		//is.putback(buff[3]).putback(buff[2]).putback(buff[1]).putback(buff[0]);
		LoadOld(is);
	} else {
		ReadBinary( is, w );
		ReadBinary( is, h );
		ReadBinary( is, hx );
		ReadBinary( is, hy );
		ReadBinary( is, depth );
		LoadInternal(is);
	}
}

void C35::CIS::Save( ostream& os, int dep )
{
	os.write( "CIS2" , 4 );
	WriteBinary( os, w );
	WriteBinary( os, h );
	WriteBinary( os, hx );
	WriteBinary( os, hy );
	if(dep) depth=dep;
	if(!depth) depth=4;
	WriteBinary( os, depth );
	SaveInternal(os);
}

C35::RGBA C35::HSVA_2_RGBA( const HSVA& hsv )
{
	RGBA rgb;
	rgb.a = hsv.a;

	unsigned char reg, rem, p,q,t;

	if(hsv.s == 0)
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

	switch(reg)
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

C35::HSVA C35::RGBA_2_HSVA( const RGBA& rgb )
{
	HSVA hsv = { 0,0,0,rgb.a };

	unsigned char rgb_min = min( rgb.r, min( rgb.g, rgb.b) );
	unsigned char rgb_max = max( rgb.r, max( rgb.g, rgb.b) );
	long delta = rgb_max - rgb_min;

	hsv.v = rgb_max;

	if(!hsv.v) return hsv;

	hsv.s = 255 * delta / hsv.v;
	if(!hsv.s) return hsv;

	/**/ if( rgb_max == rgb.r ) { hsv.h =   0 + 43 * (rgb.g-rgb.b) / delta; }
	else if( rgb_max == rgb.g ) { hsv.h =  85 + 43 * (rgb.b-rgb.r) / delta; }
	else if( rgb_max == rgb.b ) { hsv.h = 171 + 43 * (rgb.r-rgb.g) / delta; }
	else assert(false);

	return hsv;
}

C35::RGBA GetPixel( SDL_Surface* surf, int x, int y)
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

	return C35::RGBA(red,green,blue,alpha);
}

SDL_Surface* C35::MakeSurface(int w,int h,SDL_PixelFormat* pf,int mask)
{
	return
		SDL_CreateRGBSurface(mask,w,h,pf->BitsPerPixel,pf->Rmask,pf->Gmask,pf->Bmask,pf->Amask);
}

SDL_Surface* C35::MakeSurface(int w,int h,SDL_PixelFormat* pf)
{
	return MakeSurface(w,h,pf,SDL_HWSURFACE);
}

SDL_Surface* C35::MakeTransMaskFromImage(SDL_Surface* img)
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
			C35::RGBA pp = GetPixel(img,x,y);
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

static int grayscale(C35::RGBA p)
{
	return (p.r+p.g+p.b) / 3;
}

namespace
{
	struct Pix
	{
		Pix() {}
		Pix(C35::CIS::PixelType _pt, C35::HSVA _hsv) : pt(_pt), hsv(_hsv) {}
		C35::CIS::PixelType pt;
		C35::HSVA hsv;
	};
	typedef std::vector<Pix> Pixels;

	bool HasMajority(const Pixels& pa, Pix& mean)
	{
		unsigned int count[4] = {0,0,0,0};

		for(const Pix& p : pa)
		{
			count[ p.pt ] ++;
		}

		bool found=false;
		int c,pt;
		for( c=0; c<4; ++c )
		{
			if( count[c] > (pa.size()/2) )
			{
				found=true;
				pt=c;
			}
		}
		if(!found) return false;

		int h=0,s=0,v=0,a=0;
		for(const Pix& p : pa)
		{
			if( p.pt == pt )
			{
				h += p.hsv.h;
				s += p.hsv.s;
				v += p.hsv.v;
				a += p.hsv.a;
			}
		}
		mean.pt = (C35::CIS::PixelType)pt;
		mean.hsv.h = h / count[pt];
		mean.hsv.s = s / count[pt];
		mean.hsv.v = v / count[pt];
		mean.hsv.a = a / count[pt];

		return true;
	}
}

void C35::CIS::Scale150w()
{
	vector<PixelType> pt2;
	vector<HSVA> p2;

	int x,y;
	int i=0;

	PixelType curr,next;
	HSVA hsv1, hsv2;

	int ww = 0;
	for(x=0;x<w;++x)
	{
		++ww;
		if( ((x%2)==0) && ((x+1)<w) ) ++ww;
	}

	for(y=0;y<h;++y)
	{
		for(x=0;x<w;++x)
		{
			curr = pixeltypes[i];
			hsv1 = pixels[i];

			pt2.push_back(curr);
			p2.push_back(hsv1);

			if( ((x%2)==0) && ((x+1)<w) )
			{
				Pixels pp; Pix m;
				pp.push_back( Pix(curr,hsv1) );
				next = pixeltypes[i+1];
				hsv2 = pixels[i+1];
				pp.push_back( Pix(next,hsv2) );
				if(HasMajority(pp,m))
				{
					pt2.push_back(m.pt);
					p2.push_back(m.hsv);
				} else {
					if( y>0 )
					{
						pp.push_back( Pix(pixeltypes[i-w],pixels[i-w]) );
						pp.push_back( Pix(pixeltypes[i-w+1],pixels[i-w+1]) );
					}
					if( (y+1)<h )
					{
						pp.push_back( Pix(pixeltypes[i+w],pixels[i+w]) );
						pp.push_back( Pix(pixeltypes[i+w+1],pixels[i+w+1]) );
					}
					if(HasMajority(pp,m))
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
	pixeltypes.assign( pt2.begin(), pt2.end() );

	pixels.clear();
	pixels.assign( p2.begin(), p2.end() );
}

void C35::CIS::Skip3()
{
	vector<PixelType> pt2;
	vector<HSVA> p2;

	int x,y;
	int nw=0,nh=0;
	for(x=0;x<w;++x) if( (x%3)!=2 ) ++nw;
	for(y=0;y<h;++y) if( (y%3)!=2 ) ++nh;

	pt2.reserve(nw*nh);
	p2.reserve(nw*nh);

	for(y=0;y<h;++y)
	{
		if( (y%3)==2 ) continue;
		for(x=0;x<w;++x)
		{
			if( (x%3)==2 ) continue;
			int i=y*w+x;
			pt2.push_back( pixeltypes[i]);
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

void C35::CIS::Scale150h()
{
	vector<PixelType> pt2;
	vector<HSVA> p2;

	int x,y;
	int i=0;

	PixelType curr,next;
	HSVA hsv1, hsv2;

	int hh = 0;
	for(y=0;y<h;++y)
	{
		++hh;
		if( ((y%2)==0) && ((y+1)<h) )
			++hh;
	}

	for(y=0;y<h;++y)
	{
		i=y*w;
		for(x=0;x<w;++x)
		{
			curr = pixeltypes[i];
			hsv1 = pixels[i];

			pt2.push_back(curr);
			p2.push_back(hsv1);
			++i;
		}

		if( ((y%2)==0) && ((y+1)<h) )
		{
			i=y*w;
			for(x=0;x<w;++x)
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
					if( x>0 )
					{
						pp.push_back( Pix(pixeltypes[i-1],pixels[i-1]) );
						pp.push_back( Pix(pixeltypes[i+w-1],pixels[i+w-1]) );
					}
					if( (x+1)<w )
					{
						pp.push_back( Pix(pixeltypes[i+1],pixels[i+1]) );
						pp.push_back( Pix(pixeltypes[i+w+1],pixels[i+w+1]) );
					}
					if(HasMajority(pp,m))
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
	pixeltypes.assign( pt2.begin(), pt2.end() );

	pixels.clear();
	pixels.assign( p2.begin(), p2.end() );
}

void C35::CIS::Scale50w()
{
	CIS small;

	small.w = w/2;
	small.h = h;
	int n = small.w*small.h;
	small.pixeltypes.clear();
	small.pixeltypes.reserve(n);
	small.pixels.clear();
	small.pixels.reserve(n);

	small.has_colimp =false;
	small.has_dither =false;
	small.has_trans  =false;

	int x,y;

	auto mkpix = [&]( PixelType pt, const vector<HSVA>& pix ) -> void
	{
		if(pt==alpha)  small.has_dither =true;
		if(pt==trans)  small.has_trans  =true;
		if(pt==colimp) small.has_colimp =true;

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

	for(y=0; y<small.h; ++y) for(x=0; x<small.w; ++x)
	{
		vector<HSVA> nrm, trn, cli, dth;
		for(int xx=0; xx<2; ++xx)
		{
			int x2 = x*2+xx;
			int y2 = y;
			int idx = y2*w+x2;
			switch(pixeltypes[idx])
			{
				case normal:  nrm.push_back(pixels[idx]);
				case alpha:   dth.push_back(pixels[idx]);
				case trans:   trn.push_back(pixels[idx]);
				case colimp:  cli.push_back(pixels[idx]);
			}
		}


		for( int n = 2; n>0; --n )
		{
			if( (int)nrm.size() == n ) { mkpix( normal, nrm ); break; }
			if( (int)trn.size() == n ) { mkpix( trans,  trn ); break; }
			if( (int)cli.size() == n ) { mkpix( colimp, cli ); break; }
			if( (int)dth.size() == n ) { mkpix( alpha,  dth ); break; }
		}
	}

	small.hx = hx/2;
	small.hy = hy;

	(*this)=small;

}

void C35::CIS::Scale50h()
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

	auto mkpix = [&]( PixelType pt, const vector<HSVA>& pix ) -> void
	{
		if(pt==alpha)  small.has_dither =true;
		if(pt==trans)  small.has_trans  =true;
		if(pt==colimp) small.has_colimp =true;

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

	for(y=0; y<small.h; ++y) for(x=0; x<small.w; ++x)
	{
		vector<HSVA> nrm, trn, cli, dth;
		for( int yy=0; yy<2; ++yy )
		{
			int x2 = x;
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

		for( int n = 2; n>0; --n )
		{
			if( (int)nrm.size() == n ) { mkpix( normal, nrm ); break; }
			if( (int)trn.size() == n ) { mkpix( trans,  trn ); break; }
			if( (int)cli.size() == n ) { mkpix( colimp, cli ); break; }
			if( (int)dth.size() == n ) { mkpix( alpha,  dth ); break; }
		}
	}

	small.hx = hx;
	small.hy = hy/2;

	(*this) = small;

}

C35::CIS C35::CIS::HalfSize()
{
	CIS small;

	small.w = w/2;
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

	auto mkpix = [&]( PixelType pt, const vector<HSVA>& pix ) -> void
	{
		if(pt==alpha)  small.has_dither =true;
		if(pt==trans)  small.has_trans  =true;
		if(pt==colimp) small.has_colimp =true;

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

	for(y=0; y<small.h; ++y) for(x=0; x<small.w; ++x)
	{
		vector<HSVA> nrm, trn, cli, dth;
		for( int yy=0; yy<2; ++yy ) for(int xx=0; xx<2; ++xx)
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


		for( int n = 4; n>0; --n )
		{
			if( (int)nrm.size() == n ) { mkpix( normal, nrm ); break; }
			if( (int)trn.size() == n ) { mkpix( trans,  trn ); break; }
			if( (int)cli.size() == n ) { mkpix( colimp, cli ); break; }
			if( (int)dth.size() == n ) { mkpix( alpha,  dth ); break; }
		}
	}

	small.hx = hx/2;
	small.hy = hy/2;

	return small;
}

C35::CIS C35::CIS::CutOut( int x1,int y1,int ww,int hh)
{
	assert( (x1>=0) && (y1>=0) );
	assert( (x1+ww) <= w );
	assert( (y1+hh) <= h );
	CIS c2;
	c2.w = ww;
	c2.h = hh;
	c2.hx=c2.hy=0;
	c2.has_colimp=c2.has_dither=c2.has_trans=false;
	int x,y, sz = ww*hh;
	c2.pixels.clear(); c2.pixels.reserve(sz);
	c2.pixeltypes.clear(); c2.pixeltypes.reserve(sz);
	for(y=0;y<hh;++y) for(x=0;x<ww;++x)
	{
		int idx1 = x1+x + (y1+y)*w;
		PixelType pt = pixeltypes[idx1];
		if(pt==alpha)  c2.has_dither =true;
		if(pt==colimp) c2.has_colimp =true;
		if(pt==trans)  c2.has_trans  =true;
		c2.pixeltypes.push_back(pt);
		c2.pixels.push_back(pixels[idx1]);
	}
	return c2;
}


void C35::CIS::Crop()
{
	int W = w;
	auto xy = [&W](int x,int y) -> int { return y*W + x; } ;
	int topcrop = 0;
	while(topcrop<h)
	{
		bool empty=true;
		for(int x=0;empty&&(x<w);++x)
			if( pixeltypes[xy(x,topcrop)] != trans )
				empty=false;
		if(empty) ++topcrop; else break;
	}
	if(topcrop==h) { w=h=0; pixeltypes.clear(); pixels.clear(); return; }
	int botcrop = 0;
	while(true)
	{
		bool empty=true;
		for(int x=0;empty&&(x<w);++x)
			if( pixeltypes[xy(x,h-botcrop-1)] != trans )
				empty=false;
		if(empty) ++botcrop; else break;
	}
	int lftcrop = 0;
	while(true)
	{
		bool empty=true;
		for(int y=0;empty&&(y<h);++y)
			if( pixeltypes[xy(lftcrop,y)] != trans )
				empty=false;
		if(empty) ++lftcrop; else break;
	}
	int rghcrop = 0;
	while(true)
	{
		bool empty=true;
		for(int y=0;empty&&(y<h);++y)
			if( pixeltypes[xy(w-rghcrop-1,y)] != trans )
				empty=false;
		if(empty) ++rghcrop; else break;
	}
	int new_w = w - lftcrop - rghcrop;
	int new_h = h - topcrop - botcrop;
	assert( (new_w>0) && (new_w<=w) && (new_h>0) && (new_h<=h) );
	vector<PixelType> new_pt; new_pt.reserve(new_w*new_h);
	vector<HSVA>      new_px; new_px.reserve(new_w*new_h);
	for(int y=0;y<new_h;++y) for(int x=0;x<new_w;++x)
	{
		int idx = xy( x+lftcrop, y+topcrop );
		new_pt.push_back( pixeltypes [idx] );
		new_px.push_back( pixels     [idx] );
	}
	pixeltypes. clear(); pixeltypes. assign( new_pt.begin(), new_pt.end() );
	pixels.     clear(); pixels.     assign( new_px.begin(), new_px.end() );
	w=new_w; h=new_h;
	hx-=lftcrop; hy-=topcrop;
}

void C35::CIS::FromImg( SDL_Surface* image, SDL_Surface* transmask, SDL_Surface* dithermask, SDL_Surface* colimpmask )
{
	assert(image);
	w = image->w;
	h = image->h;
	hx=0; //w/2;
	hy=0; //(2*h)/3;

	has_dither = dithermask;
	has_trans  = transmask;
	has_colimp = colimpmask;

	int n = w*h;
	pixels.clear();
	pixeltypes.clear();
	pixels.reserve(n);
	pixeltypes.reserve(n);

	int x,y;
	for(y=0;y<h;++y) 
	{
		for(x=0;x<w;++x)
		{
			if(colimpmask && (grayscale( GetPixel(colimpmask,x,y) ) > 127) )
			{
				pixeltypes.push_back(colimp);
				HSVA hsv = RGBA_2_HSVA( GetPixel(image,x,y) );
				pixels.push_back(hsv);
			}
			else if(transmask && (grayscale( GetPixel(transmask,x,y) ) < 127) )
			{
				pixeltypes.push_back(trans);
				HSVA hsv = { 0,0,0,255 };
				pixels.push_back(hsv);
			}
			else if(dithermask && (grayscale( GetPixel(dithermask,x,y) ) > 127) )
			{
				pixeltypes.push_back(alpha);
				HSVA hsv = RGBA_2_HSVA( GetPixel(image,x,y) );
				hsv.a = 128;
				pixels.push_back(hsv);
			}
			else
			{
				pixeltypes.push_back(normal);
				HSVA hsv = RGBA_2_HSVA( GetPixel(image,x,y) );
				pixels.push_back(hsv);
			}
		}
	}
}

SDL_Surface* C35::CIS::MakeSurface()
{
	SDL_Surface* srf;
	SDL_PixelFormat* pf = SDL_GetVideoSurface()->format;
	int mask = SDL_HWSURFACE;
	if( has_trans || has_dither )
		mask |= SDL_SRCCOLORKEY;
	srf = C35::MakeSurface(w,h,pf,mask);
	//srf = SDL_CreateRGBSurface(mask,w,h,pf->BitsPerPixel,pf->Rmask,pf->Gmask,pf->Bmask,pf->Amask);
	assert(srf);
	SDL_LockSurface(srf);
	int x,y;
	pf = srf->format;
	int i=0;

	for(y=0;y<h;++y)
	{
		unsigned char* ptr = (unsigned char*) srf->pixels;
		ptr += (srf->pitch) * y;

		for(x=0;x<w;++x)
		{
			RGBA pp = HSVA_2_RGBA(pixels[i]);
			if(has_trans || has_dither)
			{
				if( pp.r==255 && pp.g==0 && pp.b==255 )
					pp.g = 1;
			}
			unsigned long pix;
			if(pixeltypes[i]==alpha)
				if( /*(((y-hy)%2)==1) &&*/ (((x+y-hx-hy)%2)!=0) )
					pix = SDL_MapRGB(pf,pp.r,pp.g,pp.b);
				else
					pix = SDL_MapRGB(pf,255,0,255);
			else if(pixeltypes[i]==trans)
				pix = SDL_MapRGB(pf,255,0,255);
			else
				pix = SDL_MapRGB(pf,pp.r,pp.g,pp.b);
			for(int j=0;j<pf->BytesPerPixel;++j)
			{
				(*ptr) = (unsigned char)(pix&255);
				pix = pix >> 8;
				++ptr;
			}
			++i;
		}
	}
	SDL_UnlockSurface(srf);

	if( has_trans || has_dither )
	{
		SDL_SetColorKey( srf,SDL_SRCCOLORKEY, SDL_MapRGB(pf,255,0,255) );
	}
	return srf;
}

SDL_Surface* C35::CIS::MakeSurface(unsigned char alpha,unsigned char hue)
{
	SDL_Surface* srf;
	SDL_PixelFormat* pf = SDL_GetVideoSurface()->format;
	int mask = SDL_HWSURFACE | SDL_SRCALPHA;
	srf = SDL_CreateRGBSurface(mask,w,h,32,0xFF0000ul,0xFF00ul,0xFFul,0xFF000000ul);
	assert(srf);
	SDL_LockSurface(srf);
	int x,y;
	pf = srf->format;
	int i=0;

	for(y=0;y<h;++y)
	{
		unsigned char* ptr = (unsigned char*) srf->pixels;
		ptr += (srf->pitch) * y;

		for(x=0;x<w;++x)
		{
			if(pixeltypes[i]==colimp)
				pixels[i].h = hue;
			RGBA pp = HSVA_2_RGBA(pixels[i]);
			unsigned long pix;
			if(pixeltypes[i]==alpha)
				pix = SDL_MapRGBA(pf,pp.r,pp.g,pp.b,alpha);
			else if(pixeltypes[i]==trans)
				pix = SDL_MapRGBA(pf,0,0,0,0);
			else
				pix = SDL_MapRGBA(pf,pp.r,pp.g,pp.b,255);
			for(int j=0;j<pf->BytesPerPixel;++j)
			{
				(*ptr) = (unsigned char)(pix&255);
				pix = pix >> 8;
				++ptr;
			}
			++i;
		}
	}
	SDL_UnlockSurface(srf);

	return srf;
}

SDL_Surface* C35::CIS::MakeSurface(unsigned char hue)
{
	int i,sz = w*h;
	for(i=0;i<sz;++i)
	{
		if( pixeltypes[i] == colimp )
		{
			pixels[i].h = hue;
		}
	}
	return MakeSurface();
}

void C35::CIS::Unimport()
{
	int i,sz = w*h;
	for(i=0;i<sz;++i)
	{
		if( pixeltypes[i] == colimp )
		{
			pixeltypes[i] = normal;
		}
	}
}

void C35::CIS::MakeDark()
{
	int i,sz = w*h;
	for(i=0;i<sz;++i)
	{
		if( pixeltypes[i] == colimp )
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

void C35::CIS::MakeWhite()
{
	int i,sz = w*h;
	for(i=0;i<sz;++i)
	{
		if( pixeltypes[i] == colimp )
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

C35::CIS C35::CIS::Flip(bool fx,bool fy,bool rot)
{
	CIS cis;

	auto& pt = cis.pixeltypes;
	auto& px = cis.pixels;

	if(rot)
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

	for(i=0;i<n;++i)
	{
		if(rot)
		{
			x = fx ? h-(i%h)-1 : (i%h);
			y = fy ? w-(i/h)-1 : (i/h);
		} else {
			x = fx ? w-(i%w)-1 : (i%w);
			y = fy ? h-(i/w)-1 : (i/w);
		}
		pt.push_back( pixeltypes [ x + y*w ] );
		px.push_back( pixels     [ x + y*w ] );
	}
	return cis;
}

C35::Surface& C35::CIS::Get(UC hue)
{
	map<UC,Surface>::iterator iter;
	if(!has_colimp)
		iter = instance.begin();
	else
		iter = instance.find(hue);
	assert( iter != instance.end() );
	return iter->second ;
}

void C35::CIS::Instance(UC hue)
{
	if(!has_colimp)
	{
		if(instance.empty())
		{
			instance[hue].FromCIS(*this);
		}
	} else {
		if( instance.find(hue) == instance.end() )
			instance[hue].FromCIS(*this,hue);
	}
}

void C35::CIS::FreeData()
{
	pixeltypes.clear();
	pixels.clear();
}

void C35::CIS::UnInstance()
{
	for( auto itr : instance )
		itr.second.Free();
	instance.clear();
}

// **********************
// *** AnimReflection ***
// **********************

bool C35::AnimReflection::Next()
{
	if(!ba) return false;
	time = 0;
	if( ++current > ba->Size() )
	{
		current = 0;
		if(!ba->repeating) return false;
	}
	return true;
}

void C35::AnimReflection::Start()
{
	last=SDL_GetTicks();
}

bool C35::AnimReflection::Update()
{
	int ii = SDL_GetTicks();
	bool ok = Update( last - ii );
	last = ii;
	return ok;
}

bool C35::AnimReflection::Update(int ms)
{
	if( (!ba) || (!ba->delay) ) return false;
	time += ms;
	while( time >= ba->delay )
	{
		++current;
		time -= ba->delay;
	}
	if( current >= ba->Size() )
	{
		loopcnt += (current / ba->Size());
		current = current % ba->Size();
		if(!ba->repeating) return false;
	}
	return true;
}

void C35::AnimReflection::Overlay( SDL_Surface* dst, int x,int y )
{
	if(ba)
		ba->Get(current,hue).Overlay(dst,x,y);
	else if(cis)
		cis->Get(hue).Overlay(dst,x,y);
}

void C35::AnimReflection::Set(BasicAnim* b,UC h)
{
	clr(); ba=b; hue=h;
}
void C35::AnimReflection::Set(CIS* c,UC h)
{
	clr(); cis=c; hue=h;
}
C35::AnimReflection::AnimReflection() { clr(); }
C35::AnimReflection::AnimReflection(BasicAnim* b,UC h) { clr(); Set(b,h); }
C35::AnimReflection::AnimReflection(CIS* c,UC h) { clr(); Set(c,h); }

void C35::AnimReflection::clr()
{
	cis=0; ba=0;
	current=time=last=loopcnt=0;
	hue=0;
}

// *****************
// *** BasicAnim ***
// *****************

bool C35::BasicAnim::LoadExt(string fn)
{
	string ext = ExtractFileExt(fn);
	if(ext=="ba")
	{
		ifstream ifs(fn,ios::in|ios::binary);
		Load(ifs);
		return true;
	} else {
		CIS cis;
		if( cis.LoadExt(fn) )
		{
			anim.clear();
			anim.push_back(cis);
			return true;
		}
	}
	return false;
}

C35::AnimReflection C35::BasicAnim::Refl(UC hue)
{
	AnimReflection ar(this,hue);
	return ar;
}

C35::Surface& C35::BasicAnim::Get( int f, UC hue )
{
	return anim[f].Get(hue);
}

void C35::BasicAnim::Instance(UC hue)
{
	for(CIS& cis : anim)
		cis.Instance(hue);
}

void C35::BasicAnim::UnInstance()
{
	for(CIS& cis : anim)
		cis.UnInstance();
}

void C35::BasicAnim::FreeData()
{
	for(CIS& cis : anim)
		cis.FreeData();
}

void C35::BasicAnim::SaveInternal(ostream& os)
{
	std::int16_t val = anim.size();
	WriteBinary( os, val );
	for(int i=0;i<val;++i)
		anim[i].Save(os);
}
void C35::BasicAnim::LoadInternal(istream& is,bool old)
{
	std::int16_t val;
	ReadBinary( is, val );
	anim.clear();
	for(int i=0;i<val;++i)
	{
		anim.emplace_back();
		CIS& cis = anim.back();
		if(old)
			cis.LoadOld(is);
		else
			cis.Load(is);
	}
}

void C35::BasicAnim::LoadOld( istream& is )
{
	ReadBinary( is, delay );
	std::int16_t val;
	ReadBinary( is, val );
	repeating = (bool) val;
	jbf=0;
	LoadInternal(is,true);
}

void C35::BasicAnim::SaveOld( ostream& os )
{
	WriteBinary( os, delay );
	std::int16_t val = (short)repeating;
	WriteBinary( os, val );
	SaveInternal(os);
}

void C35::BasicAnim::Load( istream& is )
{
	char buff[5] = {};
	is.read(buff,4);
	if( string(buff) != "BA_2" )
	{
		is.seekg(-4, ios_base::cur);
		//is.putback(buff[3]).putback(buff[2]).putback(buff[1]).putback(buff[0]);
		LoadOld(is);
	} else {
		ReadBinary( is, delay );
		std::int16_t val;
		ReadBinary( is, val );
		repeating = (bool) (val>>15);
		jbf = val & 0x7FFF;
		LoadInternal(is);
	}
}

void C35::BasicAnim::Save( ostream& os )
{
	os.write("BA_2",4);
	WriteBinary( os, delay );
	std::int16_t val = (std::int16_t)repeating;
	val = val << 15;
	val = val | jbf;
	WriteBinary( os, val );
	SaveInternal(os);
}

void C35::BasicAnim::MakeMirror( BasicAnim& ba, bool mx, bool my, bool rot)
{
	delay = ba.delay;
	repeating = ba.repeating;
	anim.clear();
	for( CIS& cis : ba.anim )
		anim.push_back(cis.Flip(mx,my,rot));
}


// ***************
// *** AnimDir ***
// ***************

bool C35::AnimDir::LoadExt(string fn)
{
	string ext = ExtractFileExt(fn);
	if(ext=="ad")
	{
		ifstream ifs(fn,ios::in|ios::binary);
		Load(ifs);
		return true;
	} else {
		BAD bd;
		if( bd.LoadExt(fn) )
		{
			bad.clear();
			bd.dir=0; bd.mirror=false;
			bad.push_back(bd);
			return true;
		}
	}
	return false;
}


void C35::AnimDir::Instance(UC hue)
{
	for(BAD& b : bad)
		b.Instance(hue);
}

void C35::AnimDir::UnInstance()
{
	for(BAD& b : bad)
		b.UnInstance();
}

void C35::AnimDir::FreeData()
{
	for(BAD& b : bad)
		b.FreeData();
}

void C35::AnimDir::LoadInternal(istream& is,bool old)
{
	std::int16_t i,n;
	ReadBinary(is,n);
	bad.clear();
	bad.reserve(n);
	for(i=0;i<n;++i)
	{
		bad.push_back(BAD());
		BAD& b = bad.back();
		ReadBinary(is,b.dir);
		UC uc;
		ReadBinary(is,uc);
		if(uc)
		{
			b.mirror = true;
			b.flipx = uc&2;
			b.flipy = uc&4;
			b.rot90 = uc&8;
			ReadBinary(is,b.mirrorof);
		} else {
			if(old)
				b.LoadOld(is);
			else
				b.Load(is);
			b.mirror=false;
		}
	}
	Mirror();
}

void C35::AnimDir::LoadOld(istream& is) { LoadInternal(is,true); }

void C35::AnimDir::Load(istream& is)
{
	char buff[5] = {};
	is.read(buff,4);
	if( string(buff) != "AD_2" )
	{
		is.seekg(-4, ios_base::cur);
		//is.putback(buff[3]).putback(buff[2]).putback(buff[1]).putback(buff[0]);
		LoadOld(is);
	} else {
		LoadInternal(is);
	}
}

void C35::AnimDir::SaveInternal(ostream& os)
{
	short i,n = bad.size();
	WriteBinary(os,n);
	for(i=0;i<n;++i)
	{
		BAD& b = bad[i];
		WriteBinary(os,b.dir);
		if(!b.mirror)
		{
			WriteBinary(os,(UC)0);
			b.Save(os);
		} else {
			UC uc = 1;
			if(b.flipx) uc|=2;
			if(b.flipy) uc|=4;
			if(b.rot90) uc|=8;
			WriteBinary(os,uc);
			WriteBinary(os,b.mirrorof);
		}
	}
}

void C35::AnimDir::SaveOld(ostream& os) { SaveInternal(os); }

void C35::AnimDir::Save(ostream& os)
{
	os.write("AD_2",4);
	SaveInternal(os);
}

C35::AnimDir::BAD* C35::AnimDir::findexact(short d)
{
	for( BAD& bd : bad )
	{
		if( bd.dir == d ) return &bd;
	}
	return 0;
}


int C35::AnimDir::UseAsFont( SDL_Surface* d, Pos p, UC hue, string s)
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


C35::AnimReflection C35::AnimDir::Refl(short dir,UC hue)
{
	return Closest(dir).Refl(hue);
}

C35::BasicAnim& C35::AnimDir::Closest(short dir)
{
	return BestFit<BasicAnim>(dir,bad);
}

/*void C35::AnimDir::Scale150()
{
	for(BAD& a:bad)
		if(!a.mirror)
			a.Scale150();
	Mirror();
}*/

void C35::AnimDir::Mirror()
{
	for(BAD& a:bad)
		if(a.mirror)
			a.MakeMirror( Closest(a.mirrorof), a.flipx,a.flipy,a.rot90 );
}

// ***********
// *** NAV ***
// ***********

string ExtractFileNameOnly(string fn)
{
	auto p1 = fn.find_last_of("/\\");
	if(p1==string::npos) p1=0;
	auto p2 = fn.find_last_of(".");
	if( p2<p1 ) p2 = string::npos;
	if(p2==string::npos)
		return fn.substr(p1+1);
	else
		return fn.substr(p1+1,p2-p1);

}

bool C35::NAV::LoadExt(string fn)
{
	string ext = ExtractFileExt(fn);
	if(ext=="nav")
	{
		ifstream ifs(fn,ios::in|ios::binary);
		Load(ifs);
		return true;
	} else {
		AnimDir ad;
		if( ad.LoadExt(fn) )
		{
			variants.clear();
			variants.push_back(ad);
			name = ExtractFileNameOnly(fn);
			return true;
		}
	}
	return false;
}

void C35::NAV::Instance(UC hue)
{
	for( AnimDir& ad : variants )
		ad.Instance(hue);
}

void C35::NAV::UnInstance()
{
	for( AnimDir& ad : variants )
		ad.UnInstance();
}

void C35::NAV::FreeData()
{
	for( AnimDir& ad : variants )
		ad.FreeData();
}

static std::string LoadStr( std::istream& is )
{
	std::string s;
	while(true)
	{
		char c;
		ReadBinary(is,c);
		if (!c) break;
		s += c;
	}
	return s;
}

static void SaveStr( std::ostream& os, std::string str )
{
	os.write( str.c_str(), str.size()+1 );
}

void C35::NAV::LoadInternal( istream& is, bool old )
{
	name = LoadStr(is);
	char vars;
	ReadBinary(is,vars);
	variants.clear();
	for(int i=0; i<vars; ++i)
	{
		variants.emplace_back();
		if(old)
			variants.back().LoadOld(is);
		else
			variants.back().Load(is);
	}
}

void C35::NAV::SaveInternal( ostream& os )
{
	SaveStr(os,name);
	char n = variants.size();
	WriteBinary(os,n);
	for( AnimDir& ad : variants )
		ad.Save(os);
}

void C35::NAV::LoadOld(istream& is) { LoadInternal(is,true); }
void C35::NAV::SaveOld(ostream& os) { SaveInternal(os); }

void C35::NAV::Save(ostream& os)
{
	os.write("NAV2",4);
	SaveInternal(os);
}

void C35::NAV::Load(istream& is)
{
	char buff[5] = {};
	is.read(buff,4);
	if( string(buff) != "NAV2" )
	{
		is.seekg(-4, ios_base::cur);
		//is.putback(buff[3]).putback(buff[2]).putback(buff[1]).putback(buff[0]);
		LoadOld(is);
	} else {
		LoadInternal(is);
	}
}

C35::AnimReflection C35::NAV::Refl(short dir,UC hue)
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

bool C35::AnimCollection::LoadExt(string fn)
{
	string ext = ExtractFileExt(fn);
	if(ext=="ac")
	{
		std::cout << "loading file " << fn << " ext " << ext << std::endl;
		ifstream ifs(fn,ios::in|ios::binary);
		Load(ifs);
		return true;
	} else {
		NAV* nav = new NAV();
		if( nav->LoadExt(fn) )
		{
			FreeData();
			core.push_back(nav);
			mappings[nav->name]=nav;
			return true;
		}
		delete nav;
	}
	return false;
}

void C35::AnimCollection::Instance(UC hue)
{
	for(NAV* nav : core)
		nav->Instance(hue);
}

void C35::AnimCollection::UnInstance()
{
	for(NAV* nav : core)
		nav->UnInstance();
}

void C35::AnimCollection::FreeData()
{
	for(NAV* nav : core)
		nav->FreeData();
}

C35::AnimCollection::~AnimCollection()
{
	UnInstance();
	for(NAV* nav : core)
		delete nav;
}

C35::AnimReflection C35::AnimCollection::Refl(string name,short dir,UC hue)
{
	auto itr = mappings.find(name);
	auto end = mappings.end();
	if( (itr==end) && default_anim.size() )
		itr = mappings.find(default_anim);
	if(itr==end)
		itr = mappings.find("default");
	if(itr==end)
		itr = mappings.find("idle");
	if(itr==end)
		itr = mappings.begin();
	return itr->second->Refl(dir,hue);
}

void C35::AnimCollection::AddVariant(string name,AnimDir ad)
{
	auto itr = mappings.find(name);
	if(itr==mappings.end())
	{
		NAV* nav = new NAV();
		core.push_back(nav);
		nav->name=name;
		nav->variants.clear();
		nav->variants.push_back(ad);
		mappings[name]=nav;
	} else {
		itr->second->variants.push_back(ad);
	}
}

void C35::AnimCollection::LoadInternal( istream& is, bool old )
{
	core.clear();
	mappings.clear();
	char n;
	ReadBinary(is,n);
	core.reserve(n);
	for(int i=0;i<n;++i)
	{
		NAV* nav = new NAV();
		core.push_back(nav);
		if(old)
			nav->LoadOld(is);
		else
			nav->Load(is);
		mappings[ nav->name ] = nav;
	}
	ReadBinary(is,n);
	for(int i=0;i<n;++i)
	{
		string name  = LoadStr(is);
		string mapof = LoadStr(is);
		auto itr = mappings.find(mapof);
		assert(itr!=mappings.end());
		mappings[name] = itr->second;
	}
}

void C35::AnimCollection::SaveInternal( ostream& os )
{
	char n = core.size();
	WriteBinary(os,n);
	for( NAV* nav : core )
	{
		nav->Save(os);
	}
	char n2 = mappings.size() - n;
	WriteBinary(os,n2);
	if(!n2) return;
	for( auto itr : mappings )
	{
		if( itr.first == itr.second->name ) continue;
		SaveStr( os, itr.first );
		SaveStr( os, itr.second->name );
		--n2;
	}
	assert(!n2);
}


void C35::AnimCollection::LoadOld(istream& is) { LoadInternal(is,true); }
void C35::AnimCollection::SaveOld(ostream& os) { SaveInternal(os); }

void C35::AnimCollection::Save(ostream& os)
{
	os.write("AC_2",4);
	SaveInternal(os);
	SaveStr(os,default_anim);
}

void C35::AnimCollection::Load(istream& is)
{
	char buff[5] = {};
	std::cout << "Loading AC, at pos " << is.tellg() << std::endl;
	is.read(buff,4);
	if( string(buff) != "AC_2" )
	{
		is.seekg(-4, ios_base::cur);
		//is.putback(buff[3]).putback(buff[2]).putback(buff[1]).putback(buff[0]);
		LoadOld(is);
		default_anim.clear();
	} else {
		LoadInternal(is);
		default_anim=LoadStr(is);
	}
}

std::vector<std::string> C35::AnimCollection::CoreNames()
{
	std::vector<std::string> sl;
	for( NAV* nav : core )
	{
		sl.push_back( nav->name );
	}
	return sl;
}

std::vector<std::string> C35::AnimCollection::AllNames()
{
	std::vector<std::string> sl;
	for( auto itr : mappings )
	{
		sl.push_back( itr.first );
	}
	return sl;

	DoAll< &NAV::DoAll< &AnimDir::DoAll< &BasicAnim::DoAll< &CIS::Crop > > > > ();

}

