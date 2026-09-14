// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "svnrev.h"

namespace BuildVersion
{
	extern const char* GitTag;
	extern bool GitTaggedCommit;
	extern int GitTagHi;
	extern int GitTagMid;
	extern int GitTagLo;
	extern const char* GitRev;
	extern const char* GitHash;
	extern const char* GitDate;

	inline const char* GetChannelName() { return (GitTagMid % 2 != 0) ? "Dev" : "Stable"; }
	inline bool IsDevelopment() { return (GitTagMid % 2 != 0); }
} // namespace BuildVersion
