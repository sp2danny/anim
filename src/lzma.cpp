
//#include "stdafx.h"

#include <vector>

#pragma hdrstop

#define LZMA_API_STATIC

#include "lzma/include/lzma.h"

#include "core.hpp"

static bool init_encoder(lzma_stream *strm, uint32_t preset)
{
	lzma_ret ret = lzma_easy_encoder(strm, preset, LZMA_CHECK_CRC64);
	return (ret == LZMA_OK);
}

static bool compress(lzma_stream *strm, const BVec& inBuf, BVec& outBuf)
{
	UL inSz = (UL)inBuf.size();
	outBuf.resize(inSz+128);

	lzma_action action = LZMA_FINISH;

	strm->next_in = inBuf.data();
	strm->avail_in = inSz;
	strm->next_out = outBuf.data();
	strm->avail_out = outBuf.size();

	lzma_ret ret = lzma_code(strm, action);

	if (strm->avail_out == 0)
	{
		return false;
	}

	if (ret == LZMA_STREAM_END)
	{
		UL outSz = (UL)outBuf.size() - (UL)strm->avail_out;
		outBuf.resize(outSz);
		return true;
	}

	return false;
}

bool LZMACompress(const BVec& inBuf, BVec& outBuf, UL preset)
{
	lzma_stream strm = LZMA_STREAM_INIT;

	bool success = init_encoder(&strm, preset);
	if (success)
		success = compress(&strm, inBuf, outBuf);

	lzma_end(&strm);

	return success;
}

// ----------------------------------------------------------------------------

static bool init_decoder(lzma_stream *strm)
{
	lzma_ret ret = lzma_stream_decoder(strm, UINT64_MAX, 0);

	return (ret == LZMA_OK);
}

static bool decompress(lzma_stream *strm, const BVec& inBuf, BVec& outBuf)
{
	lzma_action action = LZMA_FINISH;

	constexpr int SZ = 1024;

	uint8_t outbuf[SZ];
	outBuf.clear();
	outBuf.reserve(inBuf.size() * 5);

	strm->next_in = inBuf.data();
	strm->avail_in = inBuf.size();
	strm->next_out = outbuf;
	strm->avail_out = SZ;

	while (true) {

		lzma_ret ret = lzma_code(strm, action);

		size_t write_size = SZ - strm->avail_out;
		outBuf.insert(outBuf.end(), outbuf, outbuf + write_size);

		if (strm->avail_out == 0)
		{
			strm->next_out = outbuf;
			strm->avail_out = SZ;
			continue;
		}

		if (ret == LZMA_STREAM_END)
			return true;

		return false;
	}
}

bool LZMADecompress(const BVec& inBuf, BVec& outBuf)
{
	lzma_stream strm = LZMA_STREAM_INIT;

	bool ok = init_decoder(&strm);

	ok = ok && decompress(&strm, inBuf, outBuf);

	return ok;
}

