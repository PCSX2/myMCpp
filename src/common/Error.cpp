// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "Error.h"
#include "Logger.h"

void Error::Clear()
{
	m_description.clear();
}

void Error::setDescription(std::string description)
{
	m_description = std::move(description);
}

bool Error::Fail(std::string message)
{
	setDescription(std::move(message));
	Logger::error("{}", m_description);
	return false;
}

bool Error::Assign(const Error& other)
{
	m_description = other.m_description;
	return false;
}

bool Error::Assign(std::string message)
{
	setDescription(std::move(message));
	return false;
}

Error Error::CreateString(std::string description)
{
	Error ret;
	ret.setDescription(std::move(description));
	return ret;
}

bool Error::Fail(Error* errptr, std::string message)
{
	if (errptr)
		return errptr->Fail(std::move(message));
	Logger::error("{}", message);
	return false;
}
