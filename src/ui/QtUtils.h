// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QString>
#include <QDir>
#include <QFileInfo>
#include <string>

namespace QtUtils
{
	inline QString resolveConfigFolderPath(const QString& configuredPath)
	{
		if (configuredPath.isEmpty())
			return QDir::homePath();

		const QFileInfo info(configuredPath);
		if (info.isDir())
			return info.absoluteFilePath();

		return QDir::homePath();
	}

	inline QString resolveConfigFolderPath(const std::string& configuredPath)
	{
		if (configuredPath.empty())
			return QDir::homePath();

		return resolveConfigFolderPath(QString::fromStdString(configuredPath));
	}

	inline QString sanitizeFilename(const QString& name)
	{
		QString clean = name;
		const QString forbidden = QStringLiteral("\\/:*?\"<>|");
		for (QChar ch : forbidden)
		{
			clean.replace(ch, QChar('_'));
		}
		return clean;
	}
} // namespace QtUtils
