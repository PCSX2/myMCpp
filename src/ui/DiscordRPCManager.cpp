// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "DiscordRPCManager.h"
#include "common/Logger.h"

#include <discord_rpc.h>
#include <QDateTime>
#include <cstring>

namespace
{
	const char* kFallbackClientId =
#if defined(MYMCPP_DISCORD_CLIENT_ID)
		MYMCPP_DISCORD_CLIENT_ID;
#else
		"1478869507119513670";
#endif
} // namespace

DiscordRPCManager::DiscordRPCManager(QObject* parent)
	: QObject(parent)
{
	m_callbackTimer.setInterval(2000);
	QObject::connect(&m_callbackTimer, &QTimer::timeout, this, []() {
		Discord_RunCallbacks();
	});

	m_actionTimer.setSingleShot(true);
	QObject::connect(&m_actionTimer, &QTimer::timeout, this, [this]() {
		m_isBurstActive = false;
		updatePresence(m_stableDetails, m_stableState);
	});
}

DiscordRPCManager::~DiscordRPCManager()
{
	stop();
}

void DiscordRPCManager::setEnabled(bool enabled)
{
	m_enabled = enabled;
	if (m_enabled)
	{
		start();
	}
	else
	{
		stop();
	}
}

void DiscordRPCManager::setBrowsingContext(const QString& cardName, const QString& highlightedName, const QString& highlightedPath)
{
	if (!m_enabled || !m_initialized)
	{
		return;
	}

	const QString cleanCard = cardName.trimmed().isEmpty() ? tr("Memory Card") : trimForPresence(cardName.trimmed(), 120);
	const QString cleanName = highlightedName.trimmed().isEmpty() ? tr("Browsing") : trimForPresence(highlightedName.trimmed(), 120);

	QString state = tr("Highlighted: %1").arg(cleanName);
	if (!highlightedPath.trimmed().isEmpty())
	{
		state = trimForPresence(state, 120);
	}

	const QString details = tr("Card: %1").arg(cleanCard);
	m_stableDetails = details;
	m_stableState = state;

	if (!m_isBurstActive)
	{
		updatePresence(details, state);
	}
}

void DiscordRPCManager::setPresenceText(const QString& details, const QString& state)
{
	if (!m_enabled || !m_initialized)
	{
		return;
	}

	m_stableDetails = details;
	m_stableState = state;

	if (m_isBurstActive)
	{
		return;
	}

	updatePresence(details, state);
}

void DiscordRPCManager::setTemporaryPresence(const QString& details, const QString& state, int durationMs)
{
	if (!m_enabled || !m_initialized)
	{
		return;
	}

	m_isBurstActive = true;
	updatePresence(details, state);
	m_actionTimer.start(durationMs > 0 ? durationMs : 2200);
}

void DiscordRPCManager::setCardOpenContext(const QString& cardName)
{
	if (!m_enabled || !m_initialized)
	{
		return;
	}

	const QString cleanCard = cardName.trimmed().isEmpty() ? tr("Memory Card") : trimForPresence(cardName.trimmed(), 120);
	updatePresence(tr("Card: %1").arg(cleanCard), tr("Browsing Saves"));
}

void DiscordRPCManager::resetToIdle()
{
	if (!m_enabled || !m_initialized)
	{
		return;
	}

	m_actionTimer.stop();
	m_isBurstActive = false;

	updateIdlePresence();
}

void DiscordRPCManager::start()
{
	if (m_initialized || !m_enabled)
	{
		return;
	}

	const char* clientId = getClientId();
	if (!clientId || clientId[0] == '\0')
	{
		Logger::info("DiscordRPC: Disabled because no client ID is configured");
		return;
	}

	DiscordEventHandlers handlers = {};
	Discord_Initialize(clientId, &handlers, 1, nullptr);

	m_startTimestamp = QDateTime::currentSecsSinceEpoch();
	m_stableDetails = tr("Using myMCpp");
	m_stableState = tr("Managing Saves");
	updateIdlePresence();
	m_callbackTimer.start();
	m_initialized = true;

	Logger::info("DiscordRPC: Initialized");
}

void DiscordRPCManager::stop()
{
	if (!m_initialized)
	{
		return;
	}

	m_callbackTimer.stop();
	m_actionTimer.stop();
	Discord_ClearPresence();
	Discord_Shutdown();
	m_initialized = false;
	m_isBurstActive = false;

	Logger::info("DiscordRPC: Shutdown");
}

void DiscordRPCManager::updateIdlePresence()
{
	if (!m_initialized || !m_enabled)
	{
		return;
	}

	setPresenceText(tr("Using myMCpp"), tr("Managing Saves"));
}

void DiscordRPCManager::updatePresence(const QString& details, const QString& state)
{
	if (!m_initialized || !m_enabled)
	{
		return;
	}

	DiscordRichPresence presence = {};
	QByteArray stateUtf8 = trimForPresence(state, 128).toUtf8();
	QByteArray detailsUtf8 = trimForPresence(details, 128).toUtf8();
	QByteArray largeImageKeyUtf8("myMCpp");
	QByteArray largeImageTextUtf8 = trimForPresence(details, 128).toUtf8();
	QByteArray smallImageKeyUtf8;
	QByteArray smallImageTextUtf8;

	const QString lowerState = state.toLower();
	if (lowerState.contains(QStringLiteral("selected file")) ||
		lowerState.contains(tr("Selected File").toLower()))
	{
		smallImageKeyUtf8 = QByteArray("File");
		smallImageTextUtf8 = tr("File Selected").toUtf8();
	}
	else if (lowerState.contains(QStringLiteral("selected folder")) ||
			 lowerState.contains(tr("Selected Folder").toLower()) ||
			 lowerState.contains(QStringLiteral("browsing files")) ||
			 lowerState.contains(tr("Browsing Files").toLower()))
	{
		smallImageKeyUtf8 = QByteArray("folder");
		smallImageTextUtf8 = tr("Browsing files").toUtf8();
	}
	else if (lowerState.contains(QStringLiteral("highlighted save")) ||
			 lowerState.contains(tr("Highlighted Save").toLower()) ||
			 lowerState.contains(QStringLiteral("browsing saves")) ||
			 lowerState.contains(tr("Browsing Saves").toLower()))
	{
		smallImageKeyUtf8 = QByteArray("Save");
		smallImageTextUtf8 = tr("Save browser").toUtf8();
	}
	else
	{
		smallImageKeyUtf8 = QByteArray("Idle");
		smallImageTextUtf8 = tr("Idle").toUtf8();
	}

	presence.state = stateUtf8.constData();
	presence.details = detailsUtf8.constData();
	presence.largeImageKey = largeImageKeyUtf8.constData();
	presence.largeImageText = largeImageTextUtf8.constData();
	presence.smallImageKey = smallImageKeyUtf8.constData();
	presence.smallImageText = smallImageTextUtf8.constData();
	presence.startTimestamp = m_startTimestamp;
	Discord_UpdatePresence(&presence);
}

QString DiscordRPCManager::trimForPresence(const QString& value, int maxLen) const
{
	if (value.toUtf8().size() <= maxLen)
	{
		return value;
	}

	QString trimmed = value;
	while (trimmed.toUtf8().size() > maxLen - 3)
	{
		trimmed.chop(1);
	}

	return trimmed + QStringLiteral("…");
}

const char* DiscordRPCManager::getClientId() const
{
	static const QByteArray envClientId = qgetenv("MYMCPP_DISCORD_CLIENT_ID");
	if (!envClientId.isEmpty())
	{
		return envClientId.constData();
	}

	if (kFallbackClientId && std::strlen(kFallbackClientId) > 0)
	{
		return kFallbackClientId;
	}

	return "";
}
