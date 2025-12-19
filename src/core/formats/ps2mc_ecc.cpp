// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "ps2mc_ecc.h"
#include "round.h"
#include <algorithm>

namespace
{
	int popcount(uint32_t a)
	{
		int count = 0;
		while (a != 0)
		{
			a &= a - 1;
			count++;
		}
		return count;
	}

	int parityb(uint8_t a)
	{
		a = (a ^ (a >> 1));
		a = (a ^ (a >> 2));
		a = (a ^ (a >> 4));
		return a & 1;
	}

	std::array<uint8_t, 256> makeParityTable()
	{
		std::array<uint8_t, 256> table;
		for (int i = 0; i < 256; i++)
		{
			table[i] = static_cast<uint8_t>(parityb(static_cast<uint8_t>(i)));
		}
		return table;
	}

	std::array<uint8_t, 256> makeColumnParityMasks()
	{
		std::array<uint8_t, 256> masks;
		const uint8_t cpmasks[] = {0x55, 0x33, 0x0F, 0x00, 0xAA, 0xCC, 0xF0};
		auto parity_table = makeParityTable();

		for (int b = 0; b < 256; b++)
		{
			uint8_t mask = 0;
			for (int i = 0; i < 7; i++)
			{
				size_t idx = static_cast<size_t>(b & cpmasks[i]);
				mask = static_cast<uint8_t>(mask | (static_cast<uint8_t>(parity_table[idx]) << i));
			}
			masks[b] = mask;
		}
		return masks;
	}

	const std::array<uint8_t, 256> parityTable = makeParityTable();
	const std::array<uint8_t, 256> columnParityMasks = makeColumnParityMasks();
} // namespace

std::array<uint8_t, 3> eccCalculate(const std::vector<uint8_t>& data)
{
	uint8_t column_parity = 0x77;
	uint8_t line_parity_0 = 0x7F;
	uint8_t line_parity_1 = 0x7F;

	size_t len = std::min(data.size(), size_t(128));
	for (size_t i = 0; i < len; i++)
	{
		uint8_t b = data[i];
		column_parity ^= columnParityMasks[b];
		if (parityTable[b])
		{
			line_parity_0 ^= ~i;
			line_parity_1 ^= i;
		}
	}

	return {column_parity, static_cast<uint8_t>(line_parity_0 & 0x7F), line_parity_1};
}

int eccCheck(std::vector<uint8_t>& data, std::array<uint8_t, 3>& ecc)
{
	auto computed = eccCalculate(data);

	if (computed == ecc)
	{
		return ECC_CHECK_OK;
	}

	uint8_t cp_diff = (computed[0] ^ ecc[0]) & 0x77;
	uint8_t lp0_diff = (computed[1] ^ ecc[1]) & 0x7F;
	uint8_t lp1_diff = (computed[2] ^ ecc[2]) & 0x7F;

	if (cp_diff == 0 && lp0_diff == 0 && lp1_diff == 0)
	{
		return ECC_CHECK_OK;
	}

	if (popcount(cp_diff) == 1 && popcount(lp0_diff) == 1 && popcount(lp1_diff) == 1)
	{
		uint8_t byte_pos = lp0_diff ^ lp1_diff;
		if (byte_pos < data.size())
		{
			uint8_t bit_pos = 0;
			for (int i = 0; i < 7; i++)
			{
				if (cp_diff & (1 << i))
				{
					bit_pos |= (1 << (i < 3 ? i : i - 4));
				}
			}
			data[byte_pos] ^= (1 << bit_pos);

			ecc = eccCalculate(data);
			return ECC_CHECK_CORRECTED;
		}
	}

	return ECC_CHECK_FAILED;
}

std::vector<uint8_t> eccCalculatePage(const std::vector<uint8_t>& page, int pageSize)
{
	int numChunks = divRoundUp(pageSize, 128);
	std::vector<uint8_t> spare(numChunks * 3);

	for (int i = 0; i < numChunks; i++)
	{
		size_t offset = i * 128;
		size_t len = std::min(size_t(128), page.size() - offset);
		std::vector<uint8_t> chunk(page.begin() + offset,
			page.begin() + offset + len);
		if (chunk.size() < 128)
		{
			chunk.resize(128, 0);
		}

		auto ecc = eccCalculate(chunk);
		std::copy(ecc.begin(), ecc.end(), spare.begin() + i * 3);
	}

	return spare;
}

int eccCheckPage(std::vector<uint8_t>& page, std::vector<uint8_t>& spare, int pageSize)
{
	int numChunks = divRoundUp(pageSize, 128);
	int result = ECC_CHECK_OK;

	for (int i = 0; i < numChunks; i++)
	{
		size_t offset = i * 128;
		size_t len = std::min(size_t(128), page.size() - offset);
		std::vector<uint8_t> chunk(page.begin() + offset,
			page.begin() + offset + len);
		if (chunk.size() < 128)
		{
			chunk.resize(128, 0);
		}

		std::array<uint8_t, 3> ecc;
		std::copy(spare.begin() + i * 3, spare.begin() + i * 3 + 3, ecc.begin());

		int check_result = eccCheck(chunk, ecc);
		if (check_result != ECC_CHECK_OK)
		{
			std::copy(chunk.begin(), chunk.begin() + len, page.begin() + offset);
			std::copy(ecc.begin(), ecc.end(), spare.begin() + i * 3);

			if (check_result == ECC_CHECK_FAILED)
			{
				result = ECC_CHECK_FAILED;
			}
			else if (result == ECC_CHECK_OK)
			{
				result = ECC_CHECK_CORRECTED;
			}
		}
	}

	return result;
}
