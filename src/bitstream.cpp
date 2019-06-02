
//#include "stdafx.h"

#include "bitstream.hpp"

#include <cassert>
#include <iostream>

// ----------------------------------------------------------------------------

// *****************
// *** bitsource ***
// *****************

signed long bitsource::getS(UC bitcount)
{
	auto u = get(bitcount);
	auto shift = 32 - bitcount;
	signed long sl = u << shift; sl >>= shift;
	return sl;
}

// ----------------------------------------------------------------------------

// *****************
// *** bitvector ***
// *****************

void bitvector::put(UL bits, UC bitcount)
{
	bsf <<= bitcount;
	UL mask = (1 << bitcount) - 1;
	//assert( (bits & mask) == bits );
	bsf |= (bits & mask);
	cnt += bitcount;
	while (cnt >= 8)
	{
		UC c = (bsf >> (cnt-8)) & 0xff;
		vec.push_back(c);
		cnt -= 8;
		mask = (1<<cnt) - 1;
		bsf &= mask;
	}
}

void bitvector::done()
{
	if (cnt)
	{
		UC bc = 8-cnt;
		put(0, bc);
	}
	assert(!cnt);
}

void bitvector::write(std::ostream& out)
{
	out.write((char*)vec.data(), vec.size());
}

// ----------------------------------------------------------------------------

// ******************
// *** bitchannel ***
// ******************

void bitchannel::put(UL bits, UC bc)
{
	bits_in <<= bc;
	UL mask = (1ul << bc) - 1;
	bits_in |= (bits & mask);
	in_cnt += bc;
	while (in_cnt >= 8)
	{
		UC c = (bits_in >> (in_cnt - 8)) & 0xff;
		byte_channel.push_back(c);
		in_cnt -= 8;
		mask = (1ul << in_cnt) - 1;
		bits_in &= mask;
	}
}

UL bitchannel::get(UC bc)
{
	if (!bc) return 0;

	// 1 - fill from deque
	while (true)
	{
		if (ut_cnt >= bc) break;
		if (byte_channel.empty()) break;
		ut_cnt += 8;
		bits_ut <<= 8;
		bits_ut |= byte_channel.front();
		byte_channel.pop_front();
	}
	// 2 - fill from in
	if (ut_cnt < bc)
	{
		ut_cnt += in_cnt;
		bits_ut <<= in_cnt;
		UL mask = (1ul << in_cnt) - 1;
		bits_ut |= bits_in&mask;
		in_cnt = 0;
		bits_in = 0;
	}
	assert(ut_cnt <= 32);
	assert(ut_cnt >= bc);
	UL ret = bits_ut >> (ut_cnt-bc);
	UL mask = (1ul << bc) - 1;
	ret &= mask;
	ut_cnt -= bc;
	mask = (1ul << ut_cnt) - 1;
	bits_ut &= mask;
	return ret;
}

void bitchannel::done()
{
	// noop
	// assert ( false && "source-target does not support done" );
}

// ----------------------------------------------------------------------------

// ********************
// *** streamsource ***
// ********************

void streamsource::make(UC bitcount)
{
	while (cnt < bitcount)
	{
		char c;
		if (!in.get(c)) return;
		UC uc = (UC)c;
		bsf = (bsf<<8) | uc;
		cnt += 8;
	}
	assert(cnt <= 32);
}

bool streamsource::have(UC bitcount)
{
	make(bitcount);
	return cnt >= bitcount;
}

UL streamsource::get(UC bitcount)
{
	make(bitcount);
	assert(cnt >= bitcount);
	UL ret = bsf >> (cnt-bitcount);
	cnt -= bitcount;
	bsf &= ((1<<cnt)-1);
	return ret;
}

// ----------------------------------------------------------------------------

// ********************
// *** streamtarget ***
// ********************

void streamtarget::put(UL bits, UC bitcount)
{
	bsf <<= bitcount;
	UL mask = (1 << bitcount) - 1;
	bsf |= (bits & mask);
	cnt += bitcount;
	assert (cnt <= 32);
	while (cnt >= 8)
	{
		UC c = UC(bsf >> (cnt - 8));
		out.write((char*)&c, 1);
		cnt -= 8;
		bsf &= ((1 << cnt) - 1);
	}
}

void streamtarget::done()
{
	assert(cnt < 8);
	if (cnt)
		put(0, 8 - cnt);
}

// ----------------------------------------------------------------------------

// *********************
// *** token_channel ***
// *********************

bool token_channel::have(UC bitcount)
{
	assert((bitcount%5)==0);
	return tokens.size() > (bitcount/5u);
}

UL token_channel::get(UC bitcount)
{
	(void)bitcount;
	assert(bitcount == 5);
	assert(!tokens.empty());
	UC c = tokens.front();
	tokens.pop_front();
	return c;
}

void token_channel::put(UL bits, UC bitcount)
{
	(void)bitcount;
	assert(bitcount==5);
	tokens.push_back((UC)bits);
}

void token_channel::done()
{
}

// ----------------------------------------------------------------------------

// ****************************
// *** cyclic_token_channel ***
// ****************************

cyclic_token_channel::cyclic_token_channel()
{
	//N = 1 << 14;
	//mask = N-1;
	//nibbles.resize(N);
}

bool cyclic_token_channel::have(UC bitcount)
{
	assert((bitcount % 5) == 0);
	return sz > (bitcount / 5u);
}

UL maximum_token_channel_load = 0;

UL cyclic_token_channel::get(UC bitcount)
{
	(void)bitcount;
	assert(bitcount == 5);
	assert(sz > 0);
	UC c = tokens[utp++];
	utp &= mask;
	--sz;
	return c;
}

void cyclic_token_channel::put(UL bits, UC bitcount)
{
	(void)bitcount;
	assert(bitcount == 5);
	assert(sz < N);
	tokens[inp++] = bits & 0b11111;
	inp &= mask;
	++sz;
	if (sz>maximum_token_channel_load)
		maximum_token_channel_load = sz;
}

void cyclic_token_channel::done()
{
}

UL cyclic_token_channel::size() const
{
	return sz;
}

UC& cyclic_token_channel::operator[](UL idx)
{
	assert (idx < size());
	idx = (idx + utp) % N;
	return tokens[idx];
}

UC cyclic_token_channel::operator[](UL idx) const
{
	assert (idx < size());
	idx = (idx + utp) % N;
	return tokens[idx];
}




