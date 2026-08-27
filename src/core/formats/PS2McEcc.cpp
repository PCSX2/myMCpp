// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+
// ECC routines are a C++ version of the mymc++ / mymc ECC code and related public PS2 memory card ECC references.

#include "PS2McEcc.h"
#include "round.h"
#include <algorithm>

namespace
{
	int popCount(uint32_t a)
	{
		int count = 0;
		while (a != 0)
		{
			a &= a - 1;
			count++;
		}
		return count;
	}

	int parityB(uint8_t a)
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
			table[i] = static_cast<uint8_t>(parityB(static_cast<uint8_t>(i)));
		}
		return table;
	}

	std::array<uint8_t, 256> makeColumnParityMasks()
	{
		std::array<uint8_t, 256> masks;
		const uint8_t cpmasks[] = {0x55, 0x33, 0x0F, 0x00, 0xAA, 0xCC, 0xF0};
		auto parityTableLocal = makeParityTable();

		for (int b = 0; b < 256; b++)
		{
			uint8_t mask = 0;
			for (int i = 0; i < 7; i++)
			{
				size_t idx = static_cast<size_t>(b & cpmasks[i]);
				mask = static_cast<uint8_t>(mask | (static_cast<uint8_t>(parityTableLocal[idx]) << i));
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
	uint8_t columnParity = 0x77;
	uint8_t lineParity0 = 0x7F;
	uint8_t lineParity1 = 0x7F;

	size_t len = std::min(data.size(), static_cast<size_t>(128));
	for (size_t i = 0; i < len; i++)
	{
		uint8_t b = data[i];
		columnParity ^= columnParityMasks[b];
		if (parityTable[b])
		{
			lineParity0 ^= ~i;
			lineParity1 ^= i;
		}
	}

	return {columnParity, static_cast<uint8_t>(lineParity0 & 0x7F), lineParity1};
}

int eccCheck(std::vector<uint8_t>& data, std::array<uint8_t, 3>& ecc)
{
	auto computed = eccCalculate(data);

	if (computed == ecc)
	{
		return ECC_CHECK_OK;
	}

	uint8_t cpDiff = (computed[0] ^ ecc[0]) & 0x77;
	uint8_t lp0Diff = (computed[1] ^ ecc[1]) & 0x7F;
	uint8_t lp1Diff = (computed[2] ^ ecc[2]) & 0x7F;

	uint8_t lpComp = lp0Diff ^ lp1Diff;
	uint8_t cpComp = static_cast<uint8_t>((cpDiff >> 4) ^ (cpDiff & 0x07));

	if (lpComp == 0x7F && cpComp == 0x07)
	{
		if (lp1Diff < data.size())
		{
			data[lp1Diff] = static_cast<uint8_t>(data[lp1Diff] ^ (1u << (cpDiff >> 4)));
			ecc = eccCalculate(data);
			return ECC_CHECK_CORRECTED;
		}
	}

	if ((cpDiff == 0 && lp0Diff == 0 && lp1Diff == 0) || (popCount(lpComp) + popCount(cpComp) == 1))
	{
		ecc = computed;
		return ECC_CHECK_CORRECTED;
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
		size_t len = std::min(static_cast<size_t>(128), page.size() - offset);
		std::vector<uint8_t> chunk(page.begin() + offset, page.begin() + offset + len);
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
		size_t len = std::min(static_cast<size_t>(128), page.size() - offset);
		std::vector<uint8_t> chunk(page.begin() + offset, page.begin() + offset + len);
		if (chunk.size() < 128)
		{
			chunk.resize(128, 0);
		}

		std::array<uint8_t, 3> ecc = {0, 0, 0};
		if (spare.size() >= static_cast<size_t>(i * 3 + 3))
		{
			std::copy(spare.begin() + i * 3, spare.begin() + i * 3 + 3, ecc.begin());
		}

		int checkResult = eccCheck(chunk, ecc);
		if (checkResult != ECC_CHECK_OK)
		{
			std::copy(chunk.begin(), chunk.begin() + len, page.begin() + offset);
			std::copy(ecc.begin(), ecc.end(), spare.begin() + i * 3);

			if (checkResult == ECC_CHECK_FAILED)
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
