
#pragma once

#include <iosfwd>
#include <deque>
#include <array>

#include "core.hpp"
#include "tokenmap.hpp"

struct bitsource
{
	virtual ~bitsource() = default;
	virtual bool have(UC bitcount) =0;
	virtual UL get(UC bitcount) =0;
	signed long getS(UC bitcount);
};

struct bittarget
{
	virtual ~bittarget() = default;
	virtual void put(UL bits, UC bitcount) =0;
	virtual void done() =0;
};

struct bitvector : bittarget
{
private:
	virtual void put(UL bits, UC bitcount) override;
	virtual void done() override;
	bool empty() const { return vec.empty() && !cnt; }
	UL bitcount() const { return (UL)vec.size()*8 + cnt; }
	BVec vec;
	UL bsf = 0;
	UC cnt = 0;
	void write(std::ostream&);
};

struct bitchannel : bittarget, bitsource
{
	virtual void put(UL bits, UC bc) override;
	virtual void done() override;
	virtual bool have(UC bc) override { return bitcount() >= bc; }
	virtual UL get(UC bc) override;
	bool empty() const { return byte_channel.empty() && !in_cnt && !ut_cnt; }
	UL bitcount() const { return (UL)byte_channel.size()*8 + in_cnt + ut_cnt; }
private:
	std::deque<UC> byte_channel;
	UL bits_in=0, bits_ut=0;
	UC in_cnt=0, ut_cnt=0;
};

struct compress_bypass_target : bittarget
{
	compress_bypass_target(UC tsz, bittarget& to);
	virtual void put(UL bits, UC bitcount) override;
	virtual void done() override;
	virtual void bypass(UL bits, UC bitcount);
	virtual void compress(const BVec& tokens);
	virtual void reset();

protected:
	bittarget& mytarget;

	sbct mapx;
	UC current_bd;
	UC tokensz;
	UL next_token;

	BVec tvec, prev;
	bitchannel bvec;

	void push_tokens();
	void push_bits();
	void add_bits(UL bits, UC bitcount);
	void add_token(UC token);
	void internal_done();
};

struct fake_bypass_target : compress_bypass_target
{
	fake_bypass_target(UC tsz, bittarget& to)
		: compress_bypass_target(tsz, to)
	{}
	virtual void put(UL bits, UC bitcount) override { mytarget.put(bits, bitcount); }
	virtual void done() override { mytarget.done(); }
	virtual void bypass(UL bits, UC bitcount) override { mytarget.put(bits,bitcount); }
	virtual void reset() override {}
	virtual void compress(const BVec& tokens) override
	{
		for (auto c : tokens)
			mytarget.put(c, tokensz);
	}

private:
};

struct decompress_bypass_source : bitsource
{
	decompress_bypass_source(UC tsz, bitsource& from);
	virtual bool have(UC bitcount) override;
	virtual UL get(UC bitcount) override;
	virtual void reset();
//protected:
	bitsource& mysource;
	bool getsome();
	bitchannel chan;

	UC tsz;
	sbct mapx;
	UC current_bd;
	UL next_token;
	BVec bv, prev;
};

struct fake_bypass_source : decompress_bypass_source
{
	fake_bypass_source(UC tsz, bitsource& from)
		: decompress_bypass_source(tsz, from) {}
	virtual bool have(UC bitcount) override { return mysource.have(bitcount); }
	virtual UL get(UC bitcount) override { return mysource.get(bitcount); }
	virtual void reset() override {}
};

struct streamsource : bitsource
{
	streamsource(std::istream& in) : in(in) {}
	virtual bool have(UC bitcount) override;
	virtual UL get(UC bitcount) override;
	void make(UC bitcount);
	std::istream& in;
	UL bsf = 0;
	UC cnt = 0;
};

struct streamtarget : bittarget
{
	streamtarget(std::ostream& out) : out(out) {}
	virtual void put(UL bits, UC bitcount) override;
	virtual void done() override;
	std::ostream& out;
	UL bsf = 0;
	UC cnt = 0;
};

struct token_channel : bitsource, bittarget
{
//private:
	virtual bool have(UC bitcount) override;
	virtual UL get(UC bitcount) override;
	virtual void put(UL bits, UC bitcount) override;
	virtual void done() override;

	std::deque<UC> tokens;

friend
	struct lzv_encoder;
};

struct cyclic_token_channel //: bitsource, bittarget
{
//private:
	cyclic_token_channel();
	static const UL N = 16*1024;
	static const UL mask = N-1;
	bool have(UC bitcount);// override;
	UL get(UC bitcount);// override;
	void put(UL bits, UC bitcount);// override;
	void done();// override;

	std::array<UC, N> tokens;
	UL inp=0, utp=0, sz=0;

	UL size() const;
	UC& operator[](UL);
	UC operator[](UL) const;
friend
	struct lzv_decoder;
};



