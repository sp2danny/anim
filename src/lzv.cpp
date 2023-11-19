
#include "lzv.hpp"

#include <cassert>
#include <iostream>
#include <algorithm>

static_assert(std::numeric_limits<UC>::digits == 8  , "UC not 8bit");
static_assert(std::numeric_limits<UL>::digits == 32 , "UL not 32bit");

void lzv_core::init(UL max)
{
	m6.init(max);
	for (UC c = 0; c < 64; ++c)
	{
		BVec bv = {c};
		m6.add(bv);
	}
	current_bd = BIT+1;
	next_token = 64;
	max_token = max;
	have_prev = false;
	prev.clear();
}

UC pre1[] = {0x0, 0x0, 0b111111, 0b111111, 0b111111};
const UL NN = 1;

void lzv_encoder::encode(token_channel& src, bittarget& dst)
{
	UL curr=0, nn = (UL)src.tokens.size();

	auto& nbl = src.tokens;

	#ifndef NDEBUG
	[[maybe_unused]] UL stats_pre_n = 0;
	[[maybe_unused]] UL stats_cod_n = 0;
	[[maybe_unused]] UL stats_pre_l = 0;
	[[maybe_unused]] UL stats_cod_l = 0;
	#endif

	auto max_rle = [&]() -> UL
	{
		auto max_t = 1 << current_bd;
		return (max_t - next_token) / NN;
	};

	//auto rst = [&]() -> UL { return nn - curr; };

	auto mat = [&](UL p, UC* t) -> bool
	{
		for (UL i = 0ul; i < 5; ++i)
			if (nbl[p + i] != t[i])
				return false;
		return true;
	};
	auto tok_rep_l = [&](UC* t, UL maxrep) -> UL
	{
		UL p = curr;
		UL rep = 0;
		while (true)
		{
			if ((p + 5) >= nn) break;
			if (!mat(p, t))
				break;
			++rep;
			if (rep >= maxrep) return rep;
			p += 5;
		}
		return rep;
	};

	while (true)
	{
		if (curr>=nn) break;

		UC token = nbl[curr];

		// 1 - preset sequences
		UL max_n = std::min( UL{nn-curr} / 5, max_rle());
		if (max_n)
		{
			UL trl = tok_rep_l(pre1, max_n);
			if (trl)
			{
				//trl = std::min(trl, max_rle());
				dst.put(next_token + NN * (trl - 1) + 0, current_bd);
				curr += trl * 5;

				#ifndef NDEBUG
				stats_pre_n += 1;
				stats_pre_l += 5 * trl;
				#endif

				prev.clear();
				continue;
			}
		}

		// 2 - find longest sequence in tab
		BVec bv = {token};
		while (true)
		{
			++curr;
			if (curr >= nn)
				break;
			token = nbl[curr];
			bv.push_back(token);
			auto r = m6.find(bv);
			if (r.first) continue;
			bv.pop_back();
			break;
		}
		auto r = m6.find(bv);
		assert(r.first);
		dst.put(r.second, current_bd);

		#ifndef NDEBUG
		stats_cod_n += 1;
		stats_cod_l += (UL)bv.size();
		#endif

		if (curr >= nn)
			break;

		if (next_token >= max_token)
			continue;

		if (prev.empty())
		{
			prev = std::move(bv);
			continue;
		}
		
		BVec ny = std::move(prev);
		ny.push_back(bv[0]);
		bool ok = m6.add(ny);
		if (ok)
		{
			++next_token;
			if (next_token >= (1ul << current_bd))
				++current_bd;
			prev = std::move(bv);
		}
		else {
			prev = std::move(bv);
		}
	}

}

lzv_decoder::lzv_decoder(UL max, bitsource& src)
	: source(src)
{
	init(max);
	have_prev = false;
}

bool lzv_decoder::have(UC bitcount)
{
	assert((bitcount%6)==0);
	UL nc = bitcount/6;
	make(nc);
	return tokens.size() >= nc;
}

UL lzv_decoder::get(UC bitcount)
{
	(void)bitcount;
	assert(bitcount == 6);
	make(1);
	UC ret = (UC)tokens.get(6);
	return ret;
}

void lzv_decoder::make(UL nc)
{
	while (true)
	{
		if (tokens.size() >= nc) return;

		if (!source.have(current_bd)) return;

		UL code = source.get(current_bd);
		bool ok;
		BVec bv;

		if (code < next_token)
		{
			ok = m6.lookup(code, bv);
			assert(ok);
			for (auto c : bv)
				tokens.put(c, 5);
		}
		else {
			UL x = code - next_token;
			UL nn = (x / NN) + 1;
			UC* pp = 0;
			switch (x % NN)
			{
				case 0: pp = pre1; break;
				default: assert(false);
			}
			while (nn--)
				for (int i = 0; i < 5; ++i)
					tokens.put(pp[i], 5);
			// no table growth after preset entry, just continue
			have_prev = false;
			continue;
		}

		// prepare expand table
		// the code size might have expanded here
		if (next_token >= max_token)
		{
			have_prev = false;
			continue;
		}

		if (have_prev)
		{
			ok = m6.addh(bv.front(), prev_code);
			if (ok)
			{
				++next_token;
				if (next_token >= (1ul << current_bd))
					++current_bd;
			}
		}

		have_prev = true;
		prev_code = code;
	}

}

// ----------------------------------------------------------------------------

// ******************************
// *** compress_bypass_target ***
// ******************************

void compress_bypass_target::reset()
{
	internal_done();

	current_bd = tokensz + 1;
	next_token = 1 << tokensz;

	BVec bv;
	bv.resize(1);
	mapx.init(1'500'000, false);
	for (UC i = 0; i < next_token; ++i)
	{
		bv[0] = (UC)i;
		[[maybe_unused]] bool ok = mapx.add(bv);
		assert(ok);
	}
	prev.clear();
}

compress_bypass_target::compress_bypass_target(UC tsz, bittarget& to)
	: mytarget(to), mapx(/*tsz*/), tokensz(tsz)
{
	assert (tsz <= 8);
	reset();
}

void compress_bypass_target::bypass(UL bits, UC bitcount)
{
	prev.clear();
	if (!tvec.empty())
		push_tokens();
	add_bits(bits, bitcount);
}

void compress_bypass_target::compress(const BVec& tokens)
{
	if (!bvec.empty())
		push_bits();
	for (auto&& t : tokens)
		add_token(t);
}

void compress_bypass_target::put(UL bits, UC bitcount)
{
	std::cerr << " ========================== WARNING ==========================\n";
	std::cerr << "             CBT not suitable for put interface\n";
	std::cerr << " ========================== WARNING ==========================\n";
	if ((bitcount % tokensz) == 0)
	{
		// lz tokens
		BVec tokens;
		size_t i, n = bitcount / tokensz;
		UC mask = (1ul<<tokensz) - 1;
		for (i = 0; i < n; ++i)
		{
			tokens.push_back(bits & mask);
			bits >>= tokensz;
		}
		std::reverse(tokens.begin(), tokens.end());
		compress(tokens);
	}
	else
	{
		bypass(bits, bitcount);
	}
}

void compress_bypass_target::internal_done()
{
	bool have_bit = !bvec.empty();
	bool have_tok = !tvec.empty();
	if (have_bit && have_tok)
		assert(false && "internal error");
	if (have_bit)
		push_bits();
	if (have_tok)
		push_tokens();
}

void compress_bypass_target::done()
{
	internal_done();
	mytarget.done();
}

void compress_bypass_target::push_tokens()
{
	UL curr = 0, nn = (UL)tvec.size();

	//std::cerr << "pushing " << nn << " tokens\n";
	[[maybe_unused]] UL dbg = 0;

	while (true)
	{
		if (curr >= nn) break;

		UC token = tvec[curr];

		// 2 - find longest sequence in tab
		BVec bv = {token};
		while (true)
		{
			++curr;
			if (curr >= nn)
				break;
			token = tvec[curr];
			bv.push_back(token);
			auto r = mapx.find(bv);
			if (r.first) continue;
			bv.pop_back();
			break;
		}
		auto r = mapx.find(bv);
		assert(r.first);
		mytarget.put(r.second, current_bd); ++dbg;

		if (prev.empty())
		{
			prev = std::move(bv);
			continue;
		}

		BVec ny = std::move(prev);
		ny.push_back(bv[0]);
		bool ok = mapx.add(ny);
		if (ok)
		{
			++next_token;
			if (next_token >= (1ul << current_bd))
			{
				++current_bd;
				//std::cerr << "bumped bitdepth to " << (int)current_bd << "\n";
			}
		}
		prev = std::move(bv);
	}

	tvec.clear();
	//std::cerr << "compressed to " << dbg << " codepoints, next token " << next_token << "\n";

}

void compress_bypass_target::push_bits()
{
	//std::cerr << "pushing " << bvec.bitcount() << " bits";
	[[maybe_unused]] UL dbg = 0;

	UL maxcode = (1ul << current_bd)-1;
	UL max = 1 + maxcode - next_token;
	while (bvec.bitcount() > max)
	{
		mytarget.put(maxcode, current_bd); ++dbg;
		UL rem = max;
		while (rem > 24)
		{
			UL b = bvec.get(24);
			mytarget.put(b, 24);
			rem -= 24;
		}
		UL b = bvec.get(rem);
		mytarget.put(b, rem);
	}
	UL rem = bvec.bitcount();
	UL code = rem + next_token - 1;
	mytarget.put(code, current_bd); ++dbg;
	while (rem > 24)
	{
		UL b = bvec.get(24);
		mytarget.put(b, 24);
		rem -= 24;
	}
	UL b = bvec.get(rem);
	mytarget.put(b, rem);
	assert (bvec.empty());
	prev.clear();
	//std::cerr << " width " << dbg << " leads\n";
}

void compress_bypass_target::add_bits(UL bits, UC bitcount)
{
	assert(tvec.empty());
	bvec.put(bits, bitcount);
}

void compress_bypass_target::add_token(UC token)
{
	assert(bvec.empty());
	tvec.push_back(token);
}

// ----------------------------------------------------------------------------

// ********************************
// *** decompress_bypass_source ***
// ********************************

void decompress_bypass_source::reset()
{
	current_bd = tsz + 1;
	next_token = 1 << tsz;
	bv.resize(1);
	mapx.init(1'500'000, false);
	for (UC i = 0; i < next_token; ++i)
	{
		bv[0] = (UC)i;
		[[maybe_unused]] bool ok = mapx.add(bv);
	}
	bv.clear();
	prev.clear();
}

decompress_bypass_source::decompress_bypass_source(UC tsz, bitsource& from)
	: mysource(from)
	, tsz(tsz)
	, mapx(/*tsz*/)
{
	assert (tsz == 6);
	//assert (tsz <= 8);
	reset();
}

bool decompress_bypass_source::have(UC bitcount)
{
	while (true)
	{
		auto sz = chan.bitcount();
		if (sz >= bitcount) return true;
		if (!getsome()) return false;
	}
}

UL decompress_bypass_source::get(UC bitcount)
{
	assert (chan.bitcount() >= bitcount);
	return chan.get(bitcount);
}

bool decompress_bypass_source::getsome()
{
	static int phase = 0;
	static std::size_t codepoints, tokens, bits, leads;

	[[maybe_unused]] auto phaseto = [&](int ph)
	{
		if (phase == ph) return;
		if (phase == 1)
		{
			std::cerr << "pulled " << tokens << " tokens from " << codepoints << " codepoints, next token " << next_token << "\n";
		}
		else if (phase == 2)
		{
			std::cerr << "pulled " << bits << " bits with " << leads << " leads\n";
		}
		codepoints = tokens = bits = leads = 0;
		phase = ph;
	};

	bool ok;
	ok = mysource.have(current_bd);
	if (!ok)
	{
		//phaseto(0);
		return false;
	}

	UL tok = mysource.get(current_bd);
	if (tok < next_token)
	{
		//phaseto(1);
		ok = mapx.lookup(tok, bv);
		assert(ok); ++codepoints;
		for (auto x : bv)
			chan.put(x, tsz);
		tokens += bv.size();

		if (!prev.empty())
		{
			prev.push_back(bv[0]);
			ok = mapx.add(prev);
			if (ok)
			{
				++next_token;
				if (next_token >= (1ul << current_bd))
				{
					++current_bd;
					//std::cerr << "bumped bitdepth to " << (int)current_bd << "\n";
				}
			}
		}
		prev = std::move(bv);
	}
	else
	{
		//phaseto(2);
		UL sz = 1 + tok - next_token;
		bits += sz; ++leads;
		while (sz > 24)
		{
			ok = mysource.have(24);
			assert(ok);
			UL b = mysource.get(24);
			chan.put(b, 24);
			sz -= 24;
		}
		ok = mysource.have(sz);
		assert(ok);
		UL b = mysource.get(sz);
		chan.put(b, sz);
		prev.clear();
	}

	return true;
}



void bypass_test()
{
	BVec part_1 = {2,3,4,7};
	UL   part_2 = 1234567;
	BVec part_3 = {2,3,7,7};

	bitchannel chan;
	compress_bypass_target cbt(6, chan);
	cbt.compress(part_1);
	cbt.bypass(part_2, 24);
	cbt.compress(part_3);
	cbt.done();

	decompress_bypass_source dbs(6, chan);
	for (int i=0; i<4; ++i)
	{
		if (!dbs.have(6))
			std::cerr << "compare error\n";
		if (dbs.get(6) != part_1[i])
			std::cerr << "compare error\n";
	}
	if (!dbs.have(24))
		std::cerr << "compare error\n";
	if (dbs.get(24) != part_2)
		std::cerr << "compare error\n";
	for (int i = 0; i < 4; ++i)
	{
		if (!dbs.have(6))
			std::cerr << "compare error\n";
		if (dbs.get(6) != part_3[i])
			std::cerr << "compare error\n";
	}
	assert(chan.empty());
}

