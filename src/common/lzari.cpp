// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "lzari.h"
#include <algorithm>
#include <array>

const int ARITH_BITS = 15;
const int QUADRANT1 = 1 << ARITH_BITS;
const int QUADRANT2 = QUADRANT1 * 2;
const int QUADRANT3 = QUADRANT1 * 3;
const int QUADRANT4 = QUADRANT1 * 4;
const int MAX_CUM = QUADRANT1 - 1;
const int MAX_CHAR = (256 + MAX_MATCH_LEN - MIN_MATCH_LEN + 1);

const int N = HIST_LEN; // ring buffer size
const int F = MAX_MATCH_LEN; // lookahead buffer size
const int THRESHOLD = MIN_MATCH_LEN - 1; // encode matches when len > THRESHOLD

class LzariCodec::Impl
{
public:
	std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
	std::vector<uint8_t> decompress(const std::vector<uint8_t>& input, size_t outputSize);

private:
	// Model state
	std::array<int, MAX_CHAR + 1> sym_freq{}; // frequency for symbols
	std::array<int, MAX_CHAR + 1> sym_cum{}; // cumulative freq (sym_cum[0] = total)
	std::array<int, MAX_CHAR + 1> sym_to_char{};
	std::array<int, MAX_CHAR + 1> char_to_sym{};
	std::array<int, N + 1> position_cum{};

	// Arithmetic coder state
	int low = 0;
	int high = QUADRANT4;
	int code = 0;
	int shifts = 0;

	// Bit IO
	std::vector<uint8_t> bitout;
	int bitbuf = 0;
	int bitcnt = 0;
	const uint8_t* bitin = nullptr;
	size_t bitin_size = 0;
	size_t bitin_pos = 0;

	void init_models(bool decode);
	void output_bit(int bit);
	void flush_bits();
	int input_bit();

	void encode_symbol(int symbol);
	int decode_symbol();
	void encode_position(int position);
	int decode_position();
	void update_model_encode(int symbol);
	void update_model_decode(int symbol);

	void find_longest_match(const std::array<uint8_t, N>& buf, int r, int lookahead, int& match_pos, int& match_len);
};

LzariCodec::LzariCodec()
	: pImpl(std::make_unique<Impl>())
{
}
LzariCodec::~LzariCodec() = default;

std::vector<uint8_t> LzariCodec::compress(const std::vector<uint8_t>& input)
{
	return pImpl->compress(input);
}

std::vector<uint8_t> LzariCodec::decompress(const std::vector<uint8_t>& input, size_t outputSize)
{
	return pImpl->decompress(input, outputSize);
}

void LzariCodec::Impl::output_bit(int bit)
{
	bitbuf = (bitbuf << 1) | (bit & 1);
	bitcnt++;
	if (bitcnt == 8)
	{
		bitout.push_back(static_cast<uint8_t>(bitbuf));
		bitcnt = 0;
		bitbuf = 0;
	}
}

void LzariCodec::Impl::flush_bits()
{
	if (bitcnt)
	{
		bitbuf <<= (8 - bitcnt);
		bitout.push_back(static_cast<uint8_t>(bitbuf));
		bitcnt = 0;
		bitbuf = 0;
	}
}

int LzariCodec::Impl::input_bit()
{
	if (bitin_pos >= bitin_size)
		return 0; // padding with zeros
	if (bitcnt == 0)
	{
		bitbuf = bitin[bitin_pos++];
		bitcnt = 8;
	}
	int b = (bitbuf >> 7) & 1;
	bitbuf <<= 1;
	--bitcnt;
	return b;
}

void LzariCodec::Impl::init_models(bool decode)
{
	for (int i = 1; i <= MAX_CHAR; ++i)
	{
		sym_freq[i] = 1;
		sym_to_char[i] = i - 1;
		char_to_sym[i - 1] = i;
	}
	sym_freq[0] = 0;
	if (decode)
	{
		for (int i = 0; i <= MAX_CHAR; ++i)
		{
			sym_cum[i] = i; // ascending cumulative for decoder
		}
	}
	else
	{
		for (int i = 0; i <= MAX_CHAR; ++i)
		{
			sym_cum[i] = MAX_CHAR - i; // descending for encoder
		}
	}

	position_cum.fill(0);
	int a = 0;
	for (int i = N; i >= 1; --i)
	{
		a += 10000 / (200 + i);
		position_cum[i - 1] = a;
	}

	low = 0;
	high = QUADRANT4;
	code = 0;
	shifts = 0;
}

void LzariCodec::Impl::update_model_encode(int symbol)
{
	if (sym_cum[0] >= MAX_CUM)
	{
		int c = 0;
		for (int i = MAX_CHAR; i >= 1; --i)
		{
			sym_cum[i] = c;
			int a = (sym_freq[i] + 1) / 2;
			sym_freq[i] = a;
			c += a;
		}
		sym_cum[0] = c;
	}
	int freq = sym_freq[symbol];
	int new_symbol = symbol;
	while (sym_freq[new_symbol - 1] == freq)
	{
		--new_symbol;
	}
	if (new_symbol != symbol)
	{
		int swap_char = sym_to_char[new_symbol];
		int ch = sym_to_char[symbol];
		sym_to_char[new_symbol] = ch;
		sym_to_char[symbol] = swap_char;
		char_to_sym[ch] = new_symbol;
		char_to_sym[swap_char] = symbol;
	}
	sym_freq[new_symbol] += 1;
	for (int i = new_symbol - 1; i >= 0; --i)
	{
		sym_cum[i] += 1;
	}
}

void LzariCodec::Impl::update_model_decode(int symbol)
{
	if (sym_cum[MAX_CHAR] >= MAX_CUM)
	{
		int c = 0;
		for (int i = MAX_CHAR; i >= 1; --i)
		{
			sym_cum[MAX_CHAR - i] = c;
			int a = (sym_freq[i] + 1) / 2;
			sym_freq[i] = a;
			c += a;
		}
		sym_cum[MAX_CHAR] = c;
	}
	int freq = sym_freq[symbol];
	int new_symbol = symbol;
	while (sym_freq[new_symbol - 1] == freq)
	{
		--new_symbol;
	}
	if (new_symbol != symbol)
	{
		int swap_char = sym_to_char[new_symbol];
		int ch = sym_to_char[symbol];
		sym_to_char[new_symbol] = ch;
		sym_to_char[symbol] = swap_char;
	}
	sym_freq[new_symbol] = freq + 1;
	for (int i = MAX_CHAR - new_symbol + 1; i <= MAX_CHAR; ++i)
	{
		sym_cum[i] += 1;
	}
}

void LzariCodec::Impl::encode_symbol(int symbol)
{
	int range = high - low;
	int total = sym_cum[0];
	high = low + (range * sym_cum[symbol - 1]) / total;
	low = low + (range * sym_cum[symbol]) / total;

	for (;;)
	{
		if (high <= QUADRANT2)
		{
			output_bit(0);
			for (; shifts > 0; --shifts)
				output_bit(1);
		}
		else if (low >= QUADRANT2)
		{
			output_bit(1);
			for (; shifts > 0; --shifts)
				output_bit(0);
			low -= QUADRANT2;
			high -= QUADRANT2;
		}
		else if (low >= QUADRANT1 && high <= QUADRANT3)
		{
			++shifts;
			low -= QUADRANT1;
			high -= QUADRANT1;
		}
		else
		{
			break;
		}
		low <<= 1;
		high <<= 1;
	}
	update_model_encode(symbol);
}

int LzariCodec::Impl::decode_symbol()
{
	int range = high - low;
	int total = sym_cum[MAX_CHAR];
	int n = ((code - low + 1) * total - 1) / range;

	int i = 1;
	while (sym_cum[i] <= n)
		++i; // sym_cum is ascending in decode mode
	int symbol = MAX_CHAR + 1 - i;
	high = low + (range * sym_cum[i]) / total;
	low = low + (range * sym_cum[i - 1]) / total;

	for (;;)
	{
		if (low >= QUADRANT2)
		{
			low -= QUADRANT2;
			high -= QUADRANT2;
			code -= QUADRANT2;
		}
		else if (low >= QUADRANT1 && high <= QUADRANT3)
		{
			low -= QUADRANT1;
			high -= QUADRANT1;
			code -= QUADRANT1;
		}
		else if (high <= QUADRANT2)
		{
			// do nothing
		}
		else
		{
			break;
		}
		low <<= 1;
		high <<= 1;
		code = (code << 1) + input_bit();
	}
	update_model_decode(symbol);
	return sym_to_char[symbol];
}

void LzariCodec::Impl::encode_position(int position)
{
	// position is 0..N-1 representing distance-1
	int range = high - low;
	int total = position_cum[0];
	high = low + (range * position_cum[position]) / total;
	low = low + (range * position_cum[position + 1]) / total;

	for (;;)
	{
		if (high <= QUADRANT2)
		{
			output_bit(0);
			for (; shifts > 0; --shifts)
				output_bit(1);
		}
		else if (low >= QUADRANT2)
		{
			output_bit(1);
			for (; shifts > 0; --shifts)
				output_bit(0);
			low -= QUADRANT2;
			high -= QUADRANT2;
		}
		else if (low >= QUADRANT1 && high <= QUADRANT3)
		{
			++shifts;
			low -= QUADRANT1;
			high -= QUADRANT1;
		}
		else
		{
			break;
		}
		low <<= 1;
		high <<= 1;
	}
}

int LzariCodec::Impl::decode_position()
{
	int range = high - low;
	int total = position_cum[0];
	int n = ((code - low + 1) * total - 1) / range;

	// binary search position_cum.
	int c = 1, s = N;
	while (true)
	{
		int a = (c + s) >> 1;
		if (position_cum[a] > n)
		{
			c = a + 1;
		}
		else
		{
			s = a;
		}
		if (c >= s)
			break;
	}
	int position = s - 1;
	high = low + (range * position_cum[position]) / total;
	low = low + (range * position_cum[position + 1]) / total;

	for (;;)
	{
		if (high <= QUADRANT2)
		{
			// no bit
		}
		else if (low >= QUADRANT2)
		{
			low -= QUADRANT2;
			high -= QUADRANT2;
			code -= QUADRANT2;
		}
		else if (low >= QUADRANT1 && high <= QUADRANT3)
		{
			low -= QUADRANT1;
			high -= QUADRANT1;
			code -= QUADRANT1;
		}
		else
		{
			break;
		}
		low <<= 1;
		high <<= 1;
		code = (code << 1) + input_bit();
	}
	return position;
}

// Longest match
void LzariCodec::Impl::find_longest_match(const std::array<uint8_t, N>& buf, int r, int lookahead, int& match_pos, int& match_len)
{
	match_len = 0;
	match_pos = 0;
	// search back up to N bytes
	for (int dist = 1; dist <= N; ++dist)
	{
		int len = 0;
		while (len < lookahead && len < F && buf[(r - dist + N + len) % N] == buf[(r + len) % N])
		{
			++len;
		}
		if (len > match_len)
		{
			match_len = len;
			match_pos = dist;
			if (match_len == std::min(lookahead, F))
				break;
		}
	}
}

// Compression
std::vector<uint8_t> LzariCodec::Impl::compress(const std::vector<uint8_t>& input)
{
	init_models(false);
	bitout.clear();
	bitbuf = bitcnt = 0;
	shifts = 0;
	low = 0;
	high = QUADRANT4;

	// Ring buffer
	std::array<uint8_t, N> text{};
	text.fill(' ');

	int r = N - F;
	int lookahead = 0;
	size_t in_pos = 0;

	// preload lookahead
	while (lookahead < F && in_pos < input.size())
	{
		text[(r + lookahead) % N] = input[in_pos++];
		++lookahead;
	}

	while (lookahead > 0)
	{
		int match_pos = 0, match_len = 0;
		find_longest_match(text, r, lookahead, match_pos, match_len);
		if (match_len > lookahead)
			match_len = lookahead;

		if (match_len <= THRESHOLD)
		{
			match_len = 1;
			int c = text[r];
			encode_symbol(char_to_sym[c]);
		}
		else
		{
			encode_symbol(char_to_sym[256 + match_len - MIN_MATCH_LEN]);
			encode_position(match_pos - 1);
		}

		// slide window by match_len and refill lookahead
		r = (r + match_len) % N;
		lookahead -= match_len;
		while (lookahead < F && in_pos < input.size())
		{
			text[(r + lookahead) % N] = input[in_pos++];
			++lookahead;
		}
	}

	++shifts;
	if (low < QUADRANT1)
	{
		output_bit(0);
		for (; shifts > 0; --shifts)
			output_bit(1);
	}
	else
	{
		output_bit(1);
		for (; shifts > 0; --shifts)
			output_bit(0);
	}
	flush_bits();
	return bitout;
}

// Decompression
std::vector<uint8_t> LzariCodec::Impl::decompress(const std::vector<uint8_t>& input, size_t outputSize)
{
	init_models(true);
	bitin = input.data();
	bitin_size = input.size();
	bitin_pos = 0;
	bitcnt = 0;
	bitbuf = 0;
	low = 0;
	high = QUADRANT4;
	code = 0;
	for (int i = 0; i < ARITH_BITS; ++i)
	{
		code = (code << 1) + input_bit();
	}

	std::array<uint8_t, N> text{};
	text.fill(' ');
	int r = N - F;

	std::vector<uint8_t> output;
	output.reserve(outputSize ? outputSize : input.size() * 4);

	while (outputSize == 0 || output.size() < outputSize)
	{
		int c = decode_symbol();
		if (c < 256)
		{
			output.push_back(static_cast<uint8_t>(c));
			text[r] = static_cast<uint8_t>(c);
			r = (r + 1) % N;
		}
		else
		{
			int match_len = c - 256 + MIN_MATCH_LEN;
			int match_pos = decode_position();
			int p = (r - match_pos - 1 + N) % N;
			for (int i = 0; i < match_len; ++i)
			{
				uint8_t ch = text[(p + i) % N];
				output.push_back(ch);
				text[r] = ch;
				r = (r + 1) % N;
				if (outputSize && output.size() >= outputSize)
					break;
			}
		}

		if (outputSize == 0 && bitin_pos >= bitin_size && bitcnt == 0)
		{
			// if there's no more bits, break.
			break;
		}
	}
	return output;
}

std::vector<uint8_t> lzariCompress(const std::vector<uint8_t>& data)
{
	LzariCodec codec;
	return codec.compress(data);
}

std::vector<uint8_t> lzariDecompress(const std::vector<uint8_t>& data, size_t outputSize)
{
	LzariCodec codec;
	return codec.decompress(data, outputSize);
}
