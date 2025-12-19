// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <cstdint>
#include <vector>
#include <array>

const int ECC_CHECK_OK = 0;
const int ECC_CHECK_CORRECTED = 1;
const int ECC_CHECK_FAILED = 2;

// Calculate ECC for 128 bytes of data
std::array<uint8_t, 3> eccCalculate(const std::vector<uint8_t>& data);

// Check and correct ECC
int eccCheck(std::vector<uint8_t>& data, std::array<uint8_t, 3>& ecc);

// Calculate ECC for a full page
std::vector<uint8_t> eccCalculatePage(const std::vector<uint8_t>& page, int pageSize);

// Check and correct ECC for a full page
int eccCheckPage(std::vector<uint8_t>& page, std::vector<uint8_t>& spare, int pageSize);
