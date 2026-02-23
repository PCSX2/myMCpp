// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

inline int divRoundUp(int a, int b)
{
	return (a + b - 1) / b;
}

inline int roundUp(int a, int b)
{
	return (a + b - 1) / b * b;
}

inline int roundDown(int a, int b)
{
	return a / b * b;
}
