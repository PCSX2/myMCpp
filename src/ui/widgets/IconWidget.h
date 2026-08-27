// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QWidget>
#include <QBasicTimer>
#include "../core/formats/PS2Icon.h"
#include "../core/renderer/Renderer.h"

class Config;

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

	void setLightingFromIconSys(class PS2IconSys* iconSys);

	void setBackgroundFromIconSys(class PS2IconSys* iconSys);
	void setBackgroundColor(float r, float g, float b, float a = 1.0f);

	void applyConfigToRenderer(class PS2IconSys* iconSys);

	void startRendering();
	void stopRendering();

	void resetCamera();
	void zoomIn();
	void zoomOut();

	Renderer* getRenderer() const { return m_renderer.get(); }

signals:
	void iconLoaded();
	void iconLoadFailed(const QString& error);

protected:
	QSize sizeHint() const override;
	bool hasHeightForWidth() const override;
	int heightForWidth(int w) const override;
	void timerEvent(QTimerEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void showEvent(QShowEvent* event) override;
	void hideEvent(QHideEvent* event) override;
	QPaintEngine* paintEngine() const override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	bool ensureRenderer();
	void recreateRenderer();
	void renderFrame();

	Config* m_config;
	std::shared_ptr<PS2Icon::Icon> m_icon;
	std::unique_ptr<Renderer> m_renderer;
	QBasicTimer m_renderTimer;
	bool m_renderLoopEnabled = false;
	float m_rotX = 0.0f;
	float m_rotY = 0.0f;
	float m_zoom = 1.0f;
	QPoint m_lastDragPos;
	bool m_dragging = false;
};
