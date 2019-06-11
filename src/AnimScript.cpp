
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <string_view>
#include <utility>
#include <experimental/filesystem>
#include <sstream>


#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/format.hpp>
//#include <fmt/format.h>

#define NO_SFML

#include "alib.hpp"
#include "bitstream.hpp"
#include "inireader.hpp"
#include "bitmap.hpp"

using namespace std;
using namespace alib;
using namespace boost;

namespace fs = std::experimental::filesystem;

extern bool LZMACompress(const BVec& inBuf, BVec& outBuf, UL preset);
extern bool LZMADecompress(const BVec& inBuf, BVec& outBuf);

struct ACN {
	string name;
	AC ac;
};

struct ADN {
	string name;
	AD ad;
};

struct BAN {
	string name;
	BA ba;
};

vector<ACN> acns;
vector<ADN> adns;
vector<BAN> bans;

std::string operator+(std::string_view sv)
{
	std::string s{sv};
	return s;
}

std::string_view filenameonly(std::string_view sv)
{
	std::string_view::iterator in, ut, p, end;
	in = p = sv.begin();
	ut = end = sv.end();
	for (; p != end; ++p)
	{
		switch (*p)
		{
		case ':':
		case '/':
		case '\\':
			in = p + 1;
			break;
		case '.':
			ut = p;
			break;
		default:
			;
		}
	}
	if (ut < in) ut = end;
	std::string_view ret(&*in, ut - in);
	return ret;
}

void exec_fastunzip(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: fastunzip [AC|AD|BA] <path>" << endl;
	};
	auto fail = []() -> void
	{
		cerr << "fastunzip: operation failed" << endl;
	};

	if (toks.size() != 3) return usage();

	to_upper(toks[1]);
	/**/ if (toks[1] == "AC")
	{
		fs::path fn = toks[2];
		if (!fs::exists(fn))
			return usage();
		auto t1 = chrono::high_resolution_clock::now();
		ifstream ifs{fn, fstream::binary};
		UL sz = (UL)fs::file_size(fn);
		BVec v1,v2; v1.resize(sz);
		ifs.read((char*)v1.data(), sz);
		bool ok = LZMADecompress(v1,v2);
		if (!ok) return fail();
		ACN acn;
		acn.name = fn.filename().replace_extension("").string();
		ok = acn.ac.LoadFast(v2);
		if (!ok) return fail();
		auto t2 = chrono::high_resolution_clock::now();
		auto dur = chrono::duration_cast<chrono::milliseconds>(t2 - t1);
		UL i = (UL)acns.size();
		cout << "\t[" << i << "] " << acn.name << " (" << dur.count() << " ms)\n";
		acns.push_back( std::move(acn) );
	}
	else
		return usage();
}

void exec_fastzip(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: fastzip [AC|AD|BA] # <path>" << endl;
	};
	auto count = []() -> void
	{
		cerr << "fastzip: id not found" << endl;
	};
	auto fail = []() -> void
	{
		cerr << "fastzip: operation failed" << endl;
	};

	if (toks.size() != 4) return usage();

	to_upper(toks[1]);
	/**/ if (toks[1] == "AC")
	{
		if (toks[2]=="*")
		{
		}
		else
		{
			size_t i = stoi(toks[2]);
			size_t n = acns.size();
			if (i >= n) return count();
			ACN& acn = acns[i];
			BVec v1,v2;
			auto t1 = chrono::high_resolution_clock::now();
			acn.ac.SaveFast(v1);
			bool ok = LZMACompress(v1, v2, 5);
			auto t2 = chrono::high_resolution_clock::now();
			auto dur = chrono::duration_cast<chrono::milliseconds>(t2-t1);
			if (!ok) return fail();
			fs::path name = toks[3];
			if (fs::is_directory(name))
			{
				name /= acn.name;
				name = name.replace_extension(".fzac");
			}
			ofstream ofs{name, fstream::binary};
			if (!ofs) return fail();
			ofs.write( (char*)v2.data(), v2.size() );
			cout << "wrote " << name << " in " << dur.count() << " ms\n";
		}
	}
	else
		return usage();
}

void exec_makefast(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: makefast <src> <dst>" << endl;
	};

	if (toks.size() != 3) return usage();

	fs::path src = toks[1];
	fs::path dst = toks[2];

	if (!fs::is_directory(src)) return usage();
	if (!fs::is_directory(dst)) return usage();

	fs::directory_iterator di{src};
	for (auto&& d : di)
	{
		if (fs::is_regular_file(d))
		{
			auto ext = d.path().extension().string();
			to_lower(ext);
			if (ext == ".ac")
			{
				auto fn = d.path().filename();
				auto sfn = dst / fn.replace_extension(".fac");
				if (!fs::exists(sfn))
				{
					AC ac;
					ac.Load(d.path().string());
					ac.SaveFast(sfn.string());
					cout << "processed " << fn.replace_extension("") << endl;
				} else {
					cout << "skipped " << fn.replace_extension("") << endl;
				}
			}
		}
	}

}

void exec_loadfast(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: loadfast [AC|AD|BA] <path>" << endl;
	};

	if (toks.size() != 3) return usage();

	boost::to_upper(toks[1]);

	/**/ if (toks[1] == "AC")
	{
		ACN acn;
		fs::path pth = toks[2];
		acn.ac.LoadFast(pth.string());
		acn.name = pth.filename().string();
		UL i = (UL)acns.size();
		acns.push_back(std::move(acn));
		cout << "\t[" << i << "] " << acn.name << endl;
	}
	else if (toks[1] == "BA")
	{
		BAN ban;
		fs::path pth = toks[2];
		UL sz = (UL)fs::file_size(pth);
		if (!sz) return usage();
		ifstream ifs{pth, fstream::binary};
		BVec bv; bv.resize(sz);
		UC* data = bv.data();
		ifs.read((char*)data, sz);
		ban.ba.LoadFast(data, data+sz);
		ban.name = pth.filename().string();
		UL i = (UL)bans.size();
		bans.push_back(std::move(ban));
		cout << "\t[" << i << "] " << ban.name << endl;
	}
	else if (toks[1] == "AD")
	{
		ADN adn;
		fs::path pth = toks[2];
		UL sz = (UL)fs::file_size(pth);
		if (!sz) return usage();
		ifstream ifs{pth, fstream::binary};
		BVec bv; bv.resize(sz);
		UC* data = bv.data();
		ifs.read((char*)data, sz);
		adn.ad.LoadFast(data, data+sz);
		adn.name = pth.filename().replace_extension().string();
		UL i = (UL)acns.size();
		cout << "\t[" << i << "] " << adn.name << endl;
		adns.push_back(std::move(adn));
	}
	else
		usage();

}

void exec_savefast(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: savefast [AC|AD|BA] # <dest>" << endl;
	};
	auto count = []() -> void
	{
		cerr << "savefast: id not found" << endl;
	};

	if (toks.size() != 4) return usage();

	boost::to_upper(toks[1]);

	/**/ if (toks[1] == "AC")
	{
		if (toks[2] == "*")
		{
			fs::path pth = toks[3];
			if (!fs::is_directory(pth))
				return usage();
			UL i, n = (UL)acns.size();
			for (i = 0; i < n; ++i)
			{
				fs::path ofn = pth; ofn /= acns[i].name; ofn += ".fac";
				acns[i].ac.SaveFast(ofn.string());
				cout << "wrote " << ofn << endl;
			}
		}
		else
		{
			UL i = (UL)stoi(toks[2]);
			UL n = (UL)acns.size();
			if ((i < 0) || (i >= n))
				return count();
			AC& ac = acns[i].ac;
			fs::path pth = toks[3];
			if (fs::is_directory(pth))
			{
				pth /= acns[i].name;
				pth += ".fac";
			}
			ac.SaveFast(pth.string());
			cout << "wrote " << pth << endl;
		}
	}
	else if (toks[1] == "BA")
	{
		if (toks[2] == "*")
		{
			fs::path pth = toks[3];
			if (!fs::is_directory(pth))
				return usage();
			UL i, n = (UL)bans.size();
			for (i = 0; i < n; ++i)
			{
				fs::path ofn = pth; ofn /= bans[i].name; ofn += ".fba";
				ofstream ofs{ofn, fstream::binary};
				bans[i].ba.SaveFast(ofs);
				cout << "wrote " << ofn << endl;
			}
		}
		else
		{
			UL i = (UL)stoi(toks[2]);
			UL n = (UL)bans.size();
			if ((i < 0) || (i >= n))
				return count();
			BA& ba = bans[i].ba;
			fs::path pth = toks[3];
			if (fs::is_directory(pth))
			{
				pth /= adns[i].name;
				pth += ".fba";
			}
			ofstream ofs{pth, fstream::binary};
			ba.SaveFast(ofs);
			cout << "wrote " << pth << endl;
		}
	}
	else if (toks[1] == "AD")
	{
		if (toks[2] == "*")
		{
			fs::path pth = toks[3];
			if (!fs::is_directory(pth))
				return usage();
			UL i, n = (UL)adns.size();
			for (i=0; i<n; ++i)
			{
				fs::path ofn = pth; ofn /= adns[i].name; ofn += ".fad";
				ofstream ofs{ofn, fstream::binary};
				adns[i].ad.SaveFast(ofs);
				cout << "wrote " << ofn << endl;
			}
		}
		else
		{
			UL i = (UL)stoi(toks[2]);
			UL n = (UL)adns.size();
			if ((i<0) || (i>=n))
				return count();
			AD& ad = adns[i].ad;
			fs::path pth = toks[3];
			if (fs::is_directory(pth))
			{
				pth /= adns[i].name;
				pth += ".fad";
			}
			ofstream ofs{pth, fstream::binary};
			ad.SaveFast(ofs);
			cout << "wrote " << pth << endl;
		}
	}
	else
		usage();

}

void exec_list(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: list [AC|AD|BA]" << endl;
	};

	if (toks.size() != 2) return usage();
	to_lower(toks[1]);
	if (toks[1] == "*")
	{
		exec_list({"list", "AC"});
		exec_list({"list", "AD"});
		exec_list({"list", "BA"});
	}
	else if (toks[1] == "ac")
	{
		size_t i, n = acns.size();
		cout << n << " ACs" << endl;
		for (i = 0; i < n; ++i)
		{
			cout << "\t[" << i << "] ";
			cout << setw(25) << acns[i].name << " ";
			cout << "(";
			auto anims = acns[i].ac.CoreNames();
			bool frst = true;
			for (auto&& n : anims)
			{
				if (!frst) cout << ",";
				cout << n;
				frst = false;
			}
			cout << ")" << endl;
		}
	}
	else if (toks[1] == "ad")
	{
		size_t i, n = adns.size();
		cout << n << " ADs" << endl;
		for (i=0; i<n; ++i)
		{
			cout << "\t[" << i << "] ";
			cout << setw(25) << adns[i].name << " ";
			cout << "(";
			auto dirs = adns[i].ad.AllDirs();
			sort(dirs.begin(), dirs.end());
			bool frst = true;
			for (short d : dirs)
			{
				if (!frst) cout << ",";
				cout << d;
				frst=false;
			}
			cout << ")" << endl;
		}
	}
	else if (toks[1] == "ba")
	{
		size_t i, n = bans.size();
		cout << n << " BAs" << endl;
		for (i = 0; i < n; ++i)
		{
			cout << "\t[" << i << "] ";
			cout << setw(25) << bans[i].name << " ";
			cout << "(";
			auto sz = bans[i].ba.Size();
			cout << sz << " frames)" << endl;
		}
	}
	else
		usage();
}

void exec_load(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: load [AC|AD|BA] filename" << endl;
	};
	auto file = []() -> void
	{
		cerr << "unable to load file" << endl;
	};

	if (toks.size() != 3) return usage();
	ifstream ifs{toks[2], fstream::binary};
	if (!ifs) return file();
	to_lower(toks[1]);
	if (toks[1] == "ac")
	{
		AC ac;
		ifs.seekg(4);
		streamsource bs(ifs);
		bool ok = ac.LBS(6, bs);
		if (!ok) return file();
		auto fno = filenameonly(toks[2]);
		acns.push_back({+fno, std::move(ac)});
		cout << "[" << acns.size() - 1 << "] " << fno << " " << endl;
	}
	else if (toks[1] == "ad")
	{
		AD ad;
		ifs.seekg(4);
		streamsource bs(ifs);
		bool ok = ad.LBS(6,bs);
		if (!ok) return file();
		auto fno = filenameonly(toks[2]);
		adns.push_back({+fno, std::move(ad)});
		cout << "[" << adns.size()-1 << "] "  << fno << " " << endl;
	}
	else if (toks[1] == "ba")
	{
		BA ba;
		ifs.seekg(4);
		streamsource bs(ifs);
		bool ok = ba.LBS(6, bs);
		if (!ok) return file();
		auto fno = filenameonly(toks[2]);
		bans.push_back({+fno, std::move(ba)});
		cout << "[" << bans.size() - 1 << "] " << fno << " " << endl;
	}
	else
		usage();
}

using DBS = decompress_bypass_source;
using CBT = compress_bypass_target;

//typedef fake_bypass_source DBS;
//typedef fake_bypass_target CBT;

void exec_load_pack(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: unpack [AC|AD|BA] filename" << endl;
	};
	auto file = []() -> void
	{
		cerr << "unable to load file" << endl;
	};

	if (toks.size() != 3) return usage();
	ifstream ifs{toks[2], fstream::binary};
	if (!ifs) return file();
	to_lower(toks[1]);
	if (toks[1] == "ac")
	{
		AC ac;
		ifs.seekg(4);
		streamsource bs(ifs);
		DBS dbs(6, bs);
		bool ok = ac.LoadPacked(6, dbs);
		if (!ok) return file();
		auto fno = filenameonly(toks[2]);
		acns.push_back({+fno, std::move(ac)});
		cout << "[" << acns.size() - 1 << "] " << fno << " " << endl;
	}
	else if (toks[1] == "ad")
	{
		AD ad;
		ifs.seekg(4);
		streamsource bs(ifs);
		DBS dbs(6, bs);
		bool ok = ad.LoadPacked(6, dbs);
		if (!ok) return file();
		auto fno = filenameonly(toks[2]);
		adns.push_back({+fno, std::move(ad)});
		cout << "[" << adns.size() - 1 << "] " << fno << " " << endl;
	}
	else if (toks[1] == "ba")
	{
		BA ba;
		ifs.seekg(4);
		streamsource bs(ifs);
		DBS dbs(6, bs);
		bool ok = ba.LoadPacked(6, dbs);
		if (!ok) return file();
		auto fno = filenameonly(toks[2]);
		bans.push_back({+fno, std::move(ba)});
		cout << "[" << bans.size() - 1 << "] " << fno << " " << endl;
	}
	else
		usage();
}


void exec_extr(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: extract [AC|AD|BA] #" << endl;
	};
	auto count = []() -> void
	{
		cerr << "extract: id not found" << endl;
	};

	if (toks.size() < 3) return usage();
	to_lower(toks[1]);
	if (toks[1] == "ac")
	{
		if (toks.size() != 3) return usage();
		size_t i = stoi(toks[2]);
		size_t n = acns.size();
		if (i >= n) return count();
		ACN& acn = acns[i];
		cout << "extracting " << acn.name << endl;
		auto anims = acn.ac.CoreNames();
		for (auto n : anims)
		{
			NAV& nav = acn.ac.Get(n);
			int i2, n2=nav.Count();
			for (i2 = 0; i2 < n2; ++i2)
			{
				AD& ad = nav.Get(i2);
				string name = acn.name + "-" + n;
				if (n2>1) name += "-" + to_string(i2);
				adns.push_back( {name, ad} );
				cout << "\t[" << adns.size() - 1 << "] " << name << " " << endl;
			}
		}
	}
	else if (toks[1] == "ad")
	{
		if (toks.size() != 3) return usage();
		size_t i = stoi(toks[2]);
		size_t n = adns.size();
		if (i>=n) return count();
		ADN& adn = adns[i];
		cout << "extracting " << adn.name << endl;
		auto dirs = adn.ad.AllDirs();
		sort(dirs.begin(), dirs.end());
		for (auto d : dirs)
		{
			BA& ba = adn.ad.Get(d);
			string name = adn.name;
			if (dirs.size() > 1) { name += "-" + to_string(d); }
			bans.push_back({name, ba});
			cout << "\t[" << bans.size() - 1 << "] " << name << " " << endl;
		}
	}
	else if (toks[1] == "ba")
	{
		auto usage = []() -> void
		{
			cerr << "usage: extract BA # <dir> [BMP|CIS] #digits" << endl;
		};
		if (toks.size() != 6) return usage();
		size_t i,n;
		i = stoi(toks[2]);
		n = bans.size();
		if (i>=n) return count();
		fs::path fn = toks[3];
		if (!fs::exists(fn)) return usage();
		bool docis;
		to_lower(toks[4]);
		/**/ if (toks[4] == "bmp") docis = false;
		else if (toks[4] == "cis") docis = true;
		else return usage();
		int dig = stoi(toks[5]);
		BAN& ban = bans[i];
		n = ban.ba.Size();
		for (i = 0; i < n; ++i)
		{
			CIS& cis = ban.ba.Get((int)i);
			stringstream ss;
			ss << (fn / ban.name) << "-" << setw(dig) << setfill('0') << i;
			if (docis)
			{
				string ofn = ss.str() + ".cis";
				ofstream ofs{ofn, fstream::binary};
				ofs.write("cis3", 4);
				streamtarget st(ofs);
				cis.SBT(6, st);
				st.done();
			} else {
				string ofn = ss.str() + ".bmp";
				cis.SaveBMP( ofn.c_str() , 0, {255,0,255} );
			}
		}
		cout << "extracted " << n << " items" << endl;
	}
	else
		usage();
}

void exec_save(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: save [AC|AD|BA] # filename" << endl;
	};
	auto file = []() -> void
	{
		cerr << "unable to save file" << endl;
	};
	auto count = []() -> void
	{
		cerr << "save: id not found" << endl;
	};

	if (toks.size() != 4) return usage();
	to_lower(toks[1]);
	if (toks[1] == "ac")
	{
		size_t i = stoi(toks[2]);
		size_t n = acns.size();
		if (i>=n) return count();
		ACN& acn = acns[i];

		fs::path name = toks[3];
		if (fs::is_directory(name)) name /= acn.name + ".ac";
		ofstream ofs{name, fstream::binary};
		if (!ofs) return file();

		ofs.write("AC_3", 4);
		streamtarget st(ofs);
		acn.ac.SBT(6, st);
		st.done();
		cout << "wrote " << name << endl;
	}
	else if (toks[1] == "ad")
	{
		size_t i = stoi(toks[2]);
		size_t n = adns.size();
		if (i >= n) return count();
		ADN& adn = adns[i];

		fs::path name = toks[3];
		if (fs::is_directory(name)) name /= adn.name + ".ad";
		ofstream ofs{name, fstream::binary};
		if (!ofs) return file();

		ofs.write("AD_3", 4);
		streamtarget st(ofs);
		adn.ad.SBT(6, st);
		st.done();
		cout << "wrote " << name << endl;
	}
	else if (toks[1] == "ba")
	{
		size_t i = stoi(toks[2]);
		size_t n = bans.size();
		if (i >= n) return count();
		BAN& ban = bans[i];

		fs::path name = toks[3];
		if (fs::is_directory(name)) name /= ban.name + ".ba";
		ofstream ofs{name, fstream::binary};
		if (!ofs) return file();

		ofs.write("BA_3", 4);
		streamtarget st(ofs);
		ban.ba.SBT(6, st);
		st.done();
		cout << "wrote " << name << endl;
	}
	else
		usage();
}

void exec_save_pack(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: pack [AC|AD|BA] # filename" << endl;
	};
	auto file = []() -> void
	{
		cerr << "unable to save file" << endl;
	};
	auto count = []() -> void
	{
		cerr << "save: id not found" << endl;
	};

	if (toks.size() != 4) return usage();
	to_lower(toks[1]);
	if (toks[1] == "ac")
	{
		size_t i = stoi(toks[2]);
		size_t n = acns.size();
		if (i >= n) return count();
		ACN& acn = acns[i];
		
		fs::path name = toks[3];
		if (fs::is_directory(name)) name /= acn.name + ".pac";
		ofstream ofs{name, fstream::binary};
		if (!ofs) return file();
		
		ofs.write("ACP1", 4);
		streamtarget st(ofs);
		CBT cbt(6, st);
		acn.ac.SavePacked(6, cbt);
		cbt.done();
		cout << "wrote " << name << endl;
	}
	else if (toks[1] == "ad")
	{
		size_t i = stoi(toks[2]);
		size_t n = adns.size();
		if (i >= n) return count();
		ADN& adn = adns[i];
		
		fs::path name = toks[3];
		if (fs::is_directory(name)) name /= adn.name + ".pad";
		ofstream ofs{name, fstream::binary};
		if (!ofs) return file();
		
		ofs.write("ADP1", 4);

		streamtarget st(ofs);
		CBT cbt(6, st);
		adn.ad.SavePacked(6, cbt);
		cbt.done();
		cout << "wrote " << name << endl;
	}
	else if (toks[1] == "ba")
	{
		size_t i = stoi(toks[2]);
		size_t n = bans.size();
		if (i >= n) return count();
		BAN& ban = bans[i];

		fs::path name = toks[3];
		if (fs::is_directory(name)) name /= ban.name + ".pba";
		ofstream ofs{name, fstream::binary};
		if (!ofs) return file();

		ofs.write("BAP1", 4);
		streamtarget st(ofs);
		CBT cbt(6, st);
		ban.ba.SavePacked(6, cbt);
		cbt.done();
		cout << "wrote " << name << endl;
	}
	else
		usage();
}


void exec_saveall(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: save* [AC|AD|BA] <path>" << endl;
	};
	auto file = []() -> void
	{
		cerr << "unable to save file" << endl;
	};

	if (toks.size() != 3) return usage();
	to_lower(toks[1]);
	fs::path dir = toks[2];
	if (!fs::exists(dir)) return file();
	if (!fs::is_directory(dir)) return file();
	if (toks[1] == "ac")
	{
		size_t i,n = acns.size();
		for (i = 0; i < n; ++i)
		{
			auto name = dir / acns[i].name;
			name += ".ac"s;
			ofstream ofs{name, fstream::binary};
			if (!ofs) return file();
			ofs.write("AC_3", 4);
			streamtarget st(ofs);
			acns[i].ac.SBT(6, st);
			st.done();
		}
		cout << "Wrote " << n << " ACs" << endl;
	}
	else if (toks[1] == "ad")
	{
		size_t i, n = adns.size();
		for (i = 0; i < n; ++i)
		{
			auto name = dir / adns[i].name;
			name += ".ad"s;
			ofstream ofs{name, fstream::binary};
			if (!ofs) return file();
			ofs.write("AD_3", 4);
			streamtarget st(ofs);
			adns[i].ad.SBT(6, st);
			st.done();
		}
		cout << "Wrote " << n << " ADs" << endl;
	}
	else if (toks[1] == "ba")
	{
		size_t i, n = bans.size();
		for (i = 0; i < n; ++i)
		{
			auto name = dir / bans[i].name;
			name += ".ba"s;
			ofstream ofs{name, fstream::binary};
			if (!ofs) return file();
			ofs.write("BA_3", 4);
			streamtarget st(ofs);
			bans[i].ba.SBT(6, st);
			st.done();
		}
		cout << "Wrote " << n << " BAs" << endl;
	}
	else
		usage();
}

void exec_new(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: new [AC|AD|BA] name" << endl;
	};

	if (toks.size() != 3) return usage();
	string name = toks[2];
	to_lower(toks[1]);
	if (toks[1] == "ac")
	{
		acns.push_back({name, {}});
		size_t n = acns.size();
		cout << "[" << n-1 << "] " << name << " " << endl;
	}
	else if (toks[1] == "ad")
	{
		adns.push_back({name, {}});
		size_t n = adns.size();
		cout << "[" << n - 1 << "] " << name << " " << endl;
	}
	else if (toks[1] == "ba")
	{
		bans.push_back({name, {}});
		size_t n = bans.size();
		cout << "[" << n - 1 << "] " << name << " " << endl;
	}
	else
		usage();
}

void exec_add (vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: add [AC|AD] # <where> [AD|BA] #" << endl;
	};
	auto count = []() -> void
	{
		cerr << "save: id not found" << endl;
	};
	auto direction = []() -> void
	{
		cerr << "direction: 0-360" << endl;
	};

	if (toks.size() != 6) return usage();
	string name = toks[2];
	to_lower(toks[1]);
	if (toks[1] == "ac")
	{
		auto usage = []() -> void
		{
			cerr << "usage: add AC # <anim-name> AD #" << endl;
		};
		size_t i = stoi(toks[2]);
		size_t n = acns.size();
		if (i>=n) return count();
		AC& ac = acns[i].ac;
		string name = toks[3];
		to_lower(toks[4]);
		if (toks[4] != "ad") return usage();
		size_t i2 = stoi(toks[5]);
		size_t n2 = adns.size();
		if (i2 >= n2) return count();
		AD& ad = adns[i2].ad;
		ac.AddVariant(name, ad);
	}
	else if (toks[1] == "ad")
	{
		auto usage = []() -> void
		{
			cerr << "usage: add AD # <direction> BA #" << endl;
		};
		size_t i = stoi(toks[2]);
		size_t n = adns.size();
		if (i >= n) return count();
		AD& ad = adns[i].ad;
		short dir = stoi(toks[3]);
		if (dir<0 || dir>360) return direction();
		to_lower(toks[4]);
		if (toks[4] != "ba") return usage();
		size_t i2 = stoi(toks[5]);
		size_t n2 = bans.size();
		if (i2 >= n2) return count();
		BA& ba = bans[i2].ba;
		ad.AddDir(dir, ba);
	}
	else
		usage();

}

auto regular_anims = {
	"BLANK", "DEFAULT", "WALK", "RUN", "DEFEND", "DEATH", "DEAD", "FORTIFY", "FORTIFYHOLD", "FIDGET",
	"VICTORY", "TURNLEFT", "TURNRIGHT", "BUILD", "ROAD", "MINE", "IRRIGATE", "FORTRESS", "CAPTURE",
	"JUNGLE", "FOREST", "PLANT"
};

extern bool LoadFLC(const char* fn, alib::AD&);

bool LoadFLC(const char* fn, alib::AD&) { return false; }

bool flc(AC& ac, fs::path fn)
{

	Ini ini;
	{
		if (!fs::is_directory(fn)) return false;
		auto inifn = fn / fn.filename().concat(".INI");
		std::ifstream fs{inifn};
		ini.read(fs);
	}

	auto doit = [&](fs::path fn, string name, string key) -> void
	{
		auto nam = ini.getValue("animations", key);
		if (nam.empty()) return;
		if (!fs::exists(fn / nam)) return;
		std::string nn = name;
		boost::to_lower(nn);
		string ss = (fn / nam).generic_string();
		AD ad;
		bool ok = LoadFLC(ss.c_str(), ad);
		if (ok)
			ac.AddVariant(nn, ad);
	};

	doit(fn, "attack", "attack1");
	doit(fn, "attack", "attack2");
	doit(fn, "attack", "attack3");

	for (auto& ra : regular_anims)
		doit(fn, ra, ra);

	return !ac.CoreNames().empty();
}

void exec_ifp(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: ifp <in-path> [ac|pac|fac|fzac] <out-path>" << endl;
	};
	if (toks.size() != 4) return usage();

	fs::path dir = toks[1];
	fs::directory_iterator di{dir};
	for (const auto& itm : di)
	{
		auto name = itm.path();

		fs::path out_fn = toks[3];
		out_fn /= name.filename();
		string ext;
		to_upper(toks[2]); int kind;
		/**/ if (toks[2] == "AC")   { kind=1; ext = ".ac";   }
		else if (toks[2] == "PAC")  { kind=2; ext = ".pac";  }
		else if (toks[2] == "FAC")  { kind=3; ext = ".fac";  }
		else if (toks[2] == "FZAC") { kind=4; ext = ".fzac"; }
		else
			return usage();
		out_fn += ext;

		if (fs::is_directory(name) && !fs::exists(out_fn))
		{
			AC ac;
			bool ok = flc(ac, name);
			if (ok)
			{
				ofstream ofs{out_fn, fstream::binary};
				switch (kind)
				{
					case 1: {
						ofs.write("AC_3", 4);
						streamtarget st{ofs};
						ac.SBT(6, st);
						st.done();
					} break;
					case 2: {
						ofs.write("acp1", 4);
						streamtarget st{ofs};
						compress_bypass_target cbt(6, st);
						ac.SavePacked(6,cbt);
						cbt.done();
					} break;
					case 3: {
						ac.SaveFast(ofs);
					} break;
					case 4: {
						BVec v1,v2;
						ac.SaveFast(v1);
						LZMACompress(v1,v2, 6);
						ofs.write((char*)v2.data(), v2.size());
					} break;
				}
				std::cout << "wrote " << out_fn << endl;
			}
			else {
				cout << name.filename() << "failed.\n";
			}
		}
	}
}


void exec_imp(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: import [FLC|PCX|DAT|SEQ] <path>" << endl;
	};
	auto format = []() -> void
	{
		cerr << "import: flc parse error" << endl;
	};
	if (toks.size() < 2) return usage();
	to_lower(toks[1]);
	if (toks[1] == "flc")
	{
		if (toks.size() != 3) return usage();
		auto p = toks[2].find('*');
		if (p != string::npos)
		{
			p = toks[2].find_last_of("/\\");
			fs::path dir;
			string pat;
			if (p==string::npos)
			{
				dir = ".";
				pat = toks[2];
			} else {
				dir = toks[2].substr(0, p);
				pat = toks[2].substr(p+1);
				cout << " dir : '" << dir << "'\n";
				cout << " pat : '" << pat << "'\n";
				fs::directory_iterator di{dir};
				for (const auto& itm : di)
				{
					auto name = itm.path();
					if (fs::is_directory(name))
					{
						AC ac;
						bool ok = flc(ac, name);
						if (ok)
						{
							acns.push_back({name.filename().generic_string(),ac});
							size_t n = acns.size();
							cout << "[" << n - 1 << "] " << acns.back().name << " " << endl;
						}
						else {
							cout << name.filename() << "failed.\n";
						}
					}
				}
			}
		}
		else
		{
			AC ac;
			fs::path name = toks[2];
			bool ok = flc(ac, name);
			if (ok)
			{
				acns.push_back({name.filename().generic_string(),ac});
				size_t n = acns.size();
				cout << "[" << n - 1 << "] " << acns.back().name << " " << endl;
			} else {
				format();
			}
		}
	}
	else if (toks[1] == "dat")
	{
		cerr << "not yet implemented" << endl;
	}
	else if (toks[1] == "seq")
	{
		auto usage = []() -> void
		{
			cerr << "usage: import SEQ <path-lead> #digits #delay" << endl;
		};
		if (toks.size() != 5) return usage();
		//fs::path name = toks[2];
		int dig = stoi(toks[3]);
		BA ba;
		for (int i=0; ;++i)
		{
			stringstream ss;
			ss << toks[2] << setw(dig) << setfill('0') << i;
			CIS cis;
			if (fs::exists(ss.str() + ".cis"))
			{
				cis.Load(ss.str() + ".cis");
			}
			else if (fs::exists(ss.str() + ".pcx"))
			{
				cis.LoadPCX((ss.str()+".pcx").c_str());
			}
			else if (fs::exists(ss.str() + ".bmp"))
			{
				cis.LoadBMP((ss.str() + ".bmp").c_str(), {255,0,255}, 0, 0);
			}
			if (!cis.Loaded())
				break;
			ba.Append(std::move(cis));
		}
		if (ba.Size())
		{
			ba.SetJBF(0);
			ba.SetRep(true);
			ba.SetDelay(stoi(toks[4]));
			fs::path p = toks[2];
			string name = p.filename().generic_string();
			bans.push_back({name, std::move(ba)});
			size_t n = bans.size();
			cout << "[" << n - 1 << "] " << name << " " << endl;
		} else {
			cerr << "import failed" << endl;
		}
	}
	else
		usage();
}

void exec_del(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: del [AC|AD|BA] #" << endl;
	};
	auto count = []() -> void
	{
		cerr << "del: id not found" << endl;
	};

	if (toks.size() != 3) return usage();
	to_lower(toks[1]);
	size_t i = stoi(toks[2]);
	if (toks[1] == "ac")
	{
		size_t n = acns.size();
		if (i>=n) return count();
		string name = acns[i].name;
		acns.erase(acns.begin() + i);
		cout << "removed " << name << endl;
	}
	else if (toks[1] == "ad")
	{
		size_t n = adns.size();
		if (i >= n) return count();
		string name = adns[i].name;
		adns.erase(adns.begin() + i);
		cout << "removed " << name << endl;
	}
	else if (toks[1] == "ba")
	{
		size_t n = bans.size();
		if (i >= n) return count();
		string name = bans[i].name;
		bans.erase(bans.begin() + i);
		cout << "removed " << name << endl;
	}
	else
		usage();
}

void exec_ren(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: rename [AC|AD|BA] # <newname>" << endl;
	};
	auto count = []() -> void
	{
		cerr << "rename: id not found" << endl;
	};

	if (toks.size() != 4) return usage();
	to_lower(toks[1]);
	size_t i = stoi(toks[2]);
	std::string newname = toks[3];
	if (toks[1] == "ac")
	{
		size_t n = acns.size();
		if (i >= n) return count();
		acns[i].name = newname;
	}
	else if (toks[1] == "ad")
	{
		size_t n = adns.size();
		if (i >= n) return count();
		adns[i].name = newname;
	}
	else if (toks[1] == "ba")
	{
		size_t n = bans.size();
		if (i >= n) return count();
		bans[i].name = newname;
	}
	else
		usage();
}

void exec_delall(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: del* [AC|AD|BA]" << endl;
	};

	if (toks.size() != 2) return usage();
	to_lower(toks[1]);
	if (toks[1] == "ac")
	{
		size_t n = acns.size();
		acns.clear();
		cout << "Removed " << n << " ACs" << endl;
	}
	else if (toks[1] == "ad")
	{
		size_t n = adns.size();
		adns.clear();
		cout << "Removed " << n << " ADs" << endl;
	}
	else if (toks[1] == "ba")
	{
		size_t n = bans.size();
		bans.clear();
		cout << "Removed " << n << " BAs" << endl;
	}
	else
		usage();
}

void exec_pwd(vector<string>)
{
	auto x = fs::current_path();
	cout << x.generic_string() << endl;
}

void exec_cd(vector<string> toks)
{
	auto usage = []() -> void
	{
		cerr << "usage: cd <path>" << endl;
	};
	if (toks.size()!=2) return usage();
	fs::path dir = toks[1];
	fs::current_path(dir);
	exec_pwd({});
}

#include "StringMap.hpp"

using exec = auto (*)(vector<string>) -> void;
StringMap<exec> commands;

void init()
{
	commands["exit"]      = nullptr;
	commands["quit"]      = nullptr;
	commands["list"]      = &exec_list;
	commands["add"]       = &exec_add;
	commands["extract"]   = &exec_extr;
	commands["load"]      = &exec_load;
	commands["save"]      = &exec_save;
	commands["save*"]     = &exec_saveall;
	commands["new"]       = &exec_new;
	commands["import"]    = &exec_imp;
	commands["del"]       = &exec_del;
	commands["del*"]      = &exec_delall;
	commands["cd"]        = &exec_cd;
	commands["pwd"]       = &exec_pwd;
	commands["rename"]    = &exec_ren;
	commands["pack"]      = &exec_save_pack;
	commands["unpack"]    = &exec_load_pack;
	commands["ifp"]       = &exec_ifp;
	commands["savefast"]  = &exec_savefast;
	commands["loadfast"]  = &exec_loadfast;
	commands["makefast"]  = &exec_makefast;
	commands["fastzip"]   = &exec_fastzip;
	commands["fastunzip"] = &exec_fastunzip;
}

void doit(std::istream* src)
{
	string line;
	while (getline(*src, line))
	{
		trim(line);
		escaped_list_separator<char> els("\\", " ", "\"'");
		tokenizer<decltype(els)> tizer(line, els);
		vector<string> toks;
		copy(tizer.begin(), tizer.end(), back_inserter(toks));
		if (toks.empty()) continue;
		if (toks[0].empty()) continue;
		if (toks[0][0] == '#') continue;
		to_lower(toks[0]);
		auto res = commands.find(toks[0]);
		if (res.exact_match || res.count == 1)
		{
			auto ptr = *res.begin;
			if (ptr)
				ptr(std::move(toks));
			else
				break;
		}
		#ifdef NDEBUG
		else if (line == "-")
		{
			cout << "now reading from stdin\n";
			src = &cin;
		}
		#endif
		else if (res.count > 1)
		{
			auto itr = res.begin;
			cout << "could be";
			while (itr != res.end)
			{
				cout << " '" << itr.key() << "'";
				++itr;
			}
			cout << endl;
		}
		else
		{
			cout << "commands:\n\t"; 
			size_t i = 8;
			auto itr = commands.begin();
			while (itr != commands.end())
			{
				auto str = itr.key();
				auto n = str.size();
				if ((i+n) > 80)
				{
					cout << "\n\t";
					i = 8;
				}
				else if (i != 8)
				{
					cout << ", ";
					i += 2;
				}
				cout << str; i += n;
				++itr;
			}
			cout << endl;
		}
	}
}

int main(int argc, char** argv)
{

	BVec in;
	for (int i=0; i<25; ++i)
		in.insert(in.end(),{1,2,3,4,5,6,7,8,9});
	BVec ut;

	bool ok = LZMACompress(in, ut, 5);
	BVec utut;
	if (ok)
	{
		ok = LZMADecompress(ut, utut);
		assert (in == utut);
	}

	init();
	if (argc == 2)
	{
		std::string s = argv[1];
		s.erase(remove(s.begin(), s.end(), '\"'), s.end());
		ifstream in = std::ifstream{s};
		doit(&in);
		#ifndef NDEBUG
		cout << "now reading from stdin\n";
		doit(&cin);
		#endif
	} else {
		doit(&cin);
	}
}


