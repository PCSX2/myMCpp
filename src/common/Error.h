// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <string>

class Error
{
public:
	Error() = default;
	Error(const Error& other) = default;
	Error(Error&& other) = default;
	~Error() = default;

	Error& operator=(const Error& other) = default;
	Error& operator=(Error&& other) = default;

	bool IsValid() const { return !m_description.empty(); }
	const std::string& GetDescription() const { return m_description; }

	void Clear();

	bool Fail(std::string message);
	bool Assign(const Error& other);
	bool Assign(std::string message);

	static Error CreateString(std::string description);
	static bool Fail(Error* errptr, std::string message);

private:
	void setDescription(std::string description);

	std::string m_description;
};
