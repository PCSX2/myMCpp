// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QObject>
#include <QTimer>
#include <QString>

class DiscordRPCManager : public QObject
{
	Q_OBJECT

public:
	explicit DiscordRPCManager(QObject* parent = nullptr);
	~DiscordRPCManager() override;

	void setEnabled(bool enabled);
	void setBrowsingContext(const QString& cardName, const QString& highlightedName, const QString& highlightedPath);
	void setPresenceText(const QString& details, const QString& state);
	void setTemporaryPresence(const QString& details, const QString& state, int durationMs = 2200);
	void setCardOpenContext(const QString& cardName);
	void resetToIdle();
	bool isEnabled() const { return m_enabled; }
	bool isRunning() const { return m_initialized; }

private:
	void start();
	void stop();
	void updateIdlePresence();
	void updatePresence(const QString& details, const QString& state);
	QString trimForPresence(const QString& value, int maxLen) const;
	const char* getClientId() const;

	QTimer m_callbackTimer;
	QTimer m_actionTimer;
	bool m_enabled = false;
	bool m_initialized = false;
	bool m_isBurstActive = false;
	long long m_startTimestamp = 0;
	QString m_stableDetails;
	QString m_stableState;
};
