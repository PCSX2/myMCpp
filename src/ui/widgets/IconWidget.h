// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QWidget>
#include <QBasicTimer>
#include "core/formats/PS2Icon.h"

struct Config;
class PS2IconSys;
class Renderer;
class QWindow;

class IconWidget : public QWidget
{
	Q_OBJECT

public:
	explicit IconWidget(Config* config, QWidget* parent = nullptr);
	~IconWidget();

	bool loadIcon(const std::vector<uint8_t>& data);
	bool loadIcon(const std::string& data);

	void setAnimationEnabled(bool enabled);
	bool isAnimationEnabled() const;

	void setRotation(float x, float y, float z);
	void setZoom(float zoom);

	void setLightingFromIconSys(PS2IconSys* iconSys);

	void setBackgroundFromIconSys(PS2IconSys* iconSys);
	void setBackgroundColor(float r, float g, float b, float a = 1.0f);

	void applyConfigToRenderer(PS2IconSys* iconSys);

	void resetCamera();
	void zoomIn();
	void zoomOut();

signals:
	void iconLoaded();
	void iconLoadFailed(const QString& error);

protected:
	QSize sizeHint() const override;
	bool hasHeightForWidth() const override;
	int heightForWidth(int w) const override;
	bool eventFilter(QObject* watched, QEvent* event) override;
	void timerEvent(QTimerEvent* event) override;
	void showEvent(QShowEvent* event) override;
	void hideEvent(QHideEvent* event) override;

private:
	bool ensureRenderer();
	void recreateRenderer();
	void requestRender();
	void renderFrame();
	void startRendering();
	void stopRendering();

	Config* m_config;
	QWindow* m_renderWindow;
	std::shared_ptr<PS2Icon::Icon> m_icon;
	std::unique_ptr<Renderer> m_renderer;
	void* m_windowHandle = nullptr;
	QBasicTimer m_renderTimer;
	float m_rotX = 0.0f;
	float m_rotY = 0.0f;
	float m_zoom = 1.0f;
	QPoint m_lastDragPos;
	bool m_dragging = false;
};
