// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "RendererCommon.h"
#include "core/formats/PS2Icon.h"
#include "core/formats/PS2IconSys.h"
#include "common/Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

LightingState::LightingState()
	: ambientLight(0.5f)
	, iconSysAmbient(0.5f)
	, hasIconSysLighting(false)
	, mode(LightingMode::Icon)
{
	for (int i = 0; i < 3; ++i)
	{
		lightDirs[i] = glm::vec3(0.0f);
		lightColors[i] = glm::vec3(1.0f);
		iconSysLightDirs[i] = glm::vec3(0.0f);
		iconSysLightColors[i] = glm::vec3(1.0f);
	}
	applyMode();
}

void LightingState::applyMode()
{
	switch (mode)
	{
		case LightingMode::Off:
			lightDirs[0] = glm::vec3(0.0f);
			lightDirs[1] = glm::vec3(0.0f);
			lightDirs[2] = glm::vec3(0.0f);
			lightColors[0] = glm::vec3(1.0f);
			lightColors[1] = glm::vec3(1.0f);
			lightColors[2] = glm::vec3(1.0f);
			ambientLight = glm::vec3(1.0f);
			break;

		case LightingMode::Alternate1:
			lightDirs[0] = glm::normalize(glm::vec3(0.7f, 0.9f, 0.4f));
			lightDirs[1] = glm::normalize(glm::vec3(-0.6f, 0.8f, 0.5f));
			lightDirs[2] = glm::normalize(glm::vec3(0.0f, -0.7f, 0.8f));
			lightColors[0] = glm::vec3(1.0f, 0.95f, 0.9f);
			lightColors[1] = glm::vec3(0.85f, 0.9f, 1.0f);
			lightColors[2] = glm::vec3(0.35f);
			ambientLight = glm::vec3(0.55f);
			break;

		case LightingMode::Alternate2:
			lightDirs[0] = glm::normalize(glm::vec3(0.3f, 1.0f, 0.6f));
			lightDirs[1] = glm::normalize(glm::vec3(-0.8f, 0.6f, 0.4f));
			lightDirs[2] = glm::normalize(glm::vec3(0.0f, -1.0f, 0.4f));
			lightColors[0] = glm::vec3(1.1f, 1.05f, 0.95f);
			lightColors[1] = glm::vec3(0.7f, 0.8f, 1.0f);
			lightColors[2] = glm::vec3(0.25f);
			ambientLight = glm::vec3(0.5f);
			break;

		case LightingMode::Icon:
		default:
			if (hasIconSysLighting)
			{
				for (int i = 0; i < 3; ++i)
				{
					lightDirs[i] = iconSysLightDirs[i];
					lightColors[i] = iconSysLightColors[i];
				}
				ambientLight = iconSysAmbient;
			}
			else
			{
				lightDirs[0] = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
				lightDirs[1] = glm::normalize(glm::vec3(-1.0f, 1.0f, 1.0f));
				lightDirs[2] = glm::normalize(glm::vec3(0.0f, -1.0f, 1.0f));
				lightColors[0] = glm::vec3(1.0f);
				lightColors[1] = glm::vec3(1.0f);
				lightColors[2] = glm::vec3(1.0f);
				ambientLight = glm::vec3(0.5f);
			}
			break;
	}
}

void LightingState::loadFromIconSys(PS2IconSys* iconSys)
{
	if (!iconSys)
	{
		hasIconSysLighting = false;
		applyMode();
		return;
	}

	const auto& dirs = iconSys->getLightDirections();
	const auto& colors = iconSys->getLightColors();
	const auto& ambient = iconSys->getAmbientLight();

	for (int i = 0; i < 3 && i < static_cast<int>(dirs.size()); ++i)
	{
		glm::vec3 dir = glm::vec3(dirs[i].x, -dirs[i].y, -dirs[i].z);
		float len = glm::length(dir);
		if (len > 0.0001f)
		{
			iconSysLightDirs[i] = dir / len;
		}
		else
		{
			iconSysLightDirs[i] = glm::vec3(0.0f, 1.0f, 0.0f);
		}
		Logger::debug("LightingState: Light[{}] raw=({:.4f},{:.4f},{:.4f}), processed=({:.4f},{:.4f},{:.4f})",
			i, dirs[i].x, dirs[i].y, dirs[i].z,
			iconSysLightDirs[i].x, iconSysLightDirs[i].y, iconSysLightDirs[i].z);
	}

	for (int i = 0; i < 3 && i < static_cast<int>(colors.size()); ++i)
	{
		iconSysLightColors[i] = glm::vec3(colors[i].r, colors[i].g, colors[i].b);
	}

	iconSysAmbient = glm::vec3(ambient.r, ambient.g, ambient.b);
	const float MIN_AMBIENT = 0.3f;
	iconSysAmbient.r = glm::max(iconSysAmbient.r, MIN_AMBIENT);
	iconSysAmbient.g = glm::max(iconSysAmbient.g, MIN_AMBIENT);
	iconSysAmbient.b = glm::max(iconSysAmbient.b, MIN_AMBIENT);

	bool allZero = true;
	for (int i = 0; i < 3 && allZero; ++i)
	{
		if (iconSysLightColors[i].r > 0.1f || iconSysLightColors[i].g > 0.1f || iconSysLightColors[i].b > 0.1f)
		{
			allZero = false;
		}
	}

	if (allZero)
	{
		Logger::debug("LightingState: All-zero lighting detected, using default fallback");
		hasIconSysLighting = false; // Fall back to default lighting
	}
	else
	{
		hasIconSysLighting = true;
	}
	applyMode();

	Logger::info("LightingState: Loaded lighting from icon.sys");
	Logger::debug("LightingState: Ambient=({:.2f},{:.2f},{:.2f}), fallback={}",
		iconSysAmbient.r, iconSysAmbient.g, iconSysAmbient.b,
		allZero ? "yes" : "no");
	for (int i = 0; i < 3; ++i)
	{
		Logger::debug("LightingState: LightColor[{}]=({:.2f},{:.2f},{:.2f})",
			i, iconSysLightColors[i].r, iconSysLightColors[i].g, iconSysLightColors[i].b);
	}
}

CameraState::CameraState()
	: rotation(0.0f)
	, zoom(1.0f)
	, baseCameraDistance(5.0f)
	, cameraOffset(0.0f, 2.5f, 0.0f)
	, mode(CameraMode::Default)
{
}

void CameraState::applyMode()
{
	switch (mode)
	{
		case CameraMode::Flat:
			cameraOffset = glm::vec3(0.0f, 0.0f, 0.0f);
			baseCameraDistance = 5.0f;
			break;

		case CameraMode::Near:
			cameraOffset = glm::vec3(0.0f, 2.5f, 0.0f);
			baseCameraDistance = 3.8f;
			break;

		case CameraMode::High:
			cameraOffset = glm::vec3(0.0f, 4.0f, 0.0f);
			baseCameraDistance = 5.5f;
			break;

		case CameraMode::Default:
		default:
			cameraOffset = glm::vec3(0.0f, 2.5f, 0.0f);
			baseCameraDistance = 5.0f;
			break;
	}
}

void CameraState::setRotation(float x, float y, float z)
{
	rotation = glm::vec3(x, y, z);
}

void CameraState::setZoom(float z)
{
	zoom = glm::clamp(z, 0.1f, 10.0f);
}

void CameraState::setOffset(float x, float y, float z)
{
	cameraOffset = glm::vec3(x, y, z);
}

void CameraState::reset()
{
	rotation = glm::vec3(0.0f);
	zoom = 1.0f;
	applyMode();
}

BackgroundState::BackgroundState()
	: alpha(1.0f)
	, shouldRender(false)
{
	for (int i = 0; i < 4; ++i)
	{
		colors[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	}
}

bool BackgroundState::loadFromIconSys(PS2IconSys* iconSys)
{
	if (!iconSys)
	{
		shouldRender = false;
		return false;
	}

	try
	{
		const auto& bgColors = iconSys->getBgColors();
		float bgTransparency = iconSys->getBgTransparency();

		// icon.sys loader already normalized to 0-1
		alpha = glm::clamp(bgTransparency, 0.0f, 1.0f);

		// Check if all background colors are zero
		bool allZero = true;
		for (size_t i = 0; i < 4 && i < bgColors.size(); ++i)
		{
			if (bgColors[i].r > 0.001f || bgColors[i].g > 0.001f || bgColors[i].b > 0.001f)
			{
				allZero = false;
				break;
			}
		}

		if (allZero)
		{
			const float gray = 0.31f;
			for (int i = 0; i < 4; ++i)
			{
				colors[i] = glm::vec4(gray, gray, gray, 1.0f);
			}
			alpha = 1.0f;
			Logger::debug("BackgroundState: All-zero colors detected, using gray fallback");
		}
		else
		{
			for (size_t i = 0; i < 4 && i < bgColors.size(); ++i)
			{
				float r = glm::clamp(bgColors[i].r, 0.0f, 1.0f);
				float g = glm::clamp(bgColors[i].g, 0.0f, 1.0f);
				float b = glm::clamp(bgColors[i].b, 0.0f, 1.0f);
				colors[i] = glm::vec4(r, g, b, alpha);
			}
		}

		shouldRender = true;

		Logger::info("BackgroundState: Loaded background from icon.sys\n  Color[0]: ({},{},{},{})\n  Alpha: {}",
			colors[0].x, colors[0].y, colors[0].z, colors[0].w, alpha);

		return true;
	}
	catch (const std::exception& e)
	{
		Logger::error("BackgroundState: Exception loading from icon.sys: {}", e.what());
		shouldRender = false;
		return false;
	}
}

void BackgroundState::setColor(float r, float g, float b, float a)
{
	glm::vec4 color(r, g, b, a);
	for (int i = 0; i < 4; ++i)
	{
		colors[i] = color;
	}
	alpha = a;
	shouldRender = true;
}

namespace LightingPresets
{

	glm::vec3 getAlt1LightDir(int index)
	{
		static const glm::vec3 dirs[3] = {
			glm::normalize(glm::vec3(0.7f, 0.9f, 0.4f)),
			glm::normalize(glm::vec3(-0.6f, 0.8f, 0.5f)),
			glm::normalize(glm::vec3(0.0f, -0.7f, 0.8f))};
		return dirs[index % 3];
	}

	glm::vec3 getAlt1LightColor(int index)
	{
		static const glm::vec3 colors[3] = {
			glm::vec3(1.0f, 0.95f, 0.9f),
			glm::vec3(0.85f, 0.9f, 1.0f),
			glm::vec3(0.35f)};
		return colors[index % 3];
	}

	glm::vec3 getAlt2LightDir(int index)
	{
		static const glm::vec3 dirs[3] = {
			glm::normalize(glm::vec3(0.3f, 1.0f, 0.6f)),
			glm::normalize(glm::vec3(-0.8f, 0.6f, 0.4f)),
			glm::normalize(glm::vec3(0.0f, -1.0f, 0.4f))};
		return dirs[index % 3];
	}

	glm::vec3 getAlt2LightColor(int index)
	{
		static const glm::vec3 colors[3] = {
			glm::vec3(1.1f, 1.05f, 0.95f),
			glm::vec3(0.7f, 0.8f, 1.0f),
			glm::vec3(0.25f)};
		return colors[index % 3];
	}

	glm::vec3 getDefaultLightDir(int index)
	{
		static const glm::vec3 dirs[3] = {
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(-1.0f, 1.0f, 1.0f),
			glm::vec3(0.0f, -1.0f, 1.0f)};
		return dirs[index % 3];
	}

} // namespace LightingPresets

namespace TextureUtils
{

	std::vector<uint8_t> convertPS2TextureToRGBA(const uint16_t* textureData, uint32_t width, uint32_t height)
	{
		if (!textureData || width == 0 || height == 0)
		{
			Logger::warn("TextureUtils: Invalid texture input (ptr={}, width={}, height={})", (const void*)textureData, width, height);
			return {};
		}

		const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
		if (pixelCount > std::numeric_limits<size_t>::max() / 4)
		{
			Logger::error("TextureUtils: Texture size overflow (width={}, height={})", width, height);
			return {};
		}

		std::vector<uint8_t> rgba(pixelCount * 4);

		for (size_t i = 0; i < pixelCount; ++i)
		{
			uint16_t pixel = textureData[i];
			uint8_t r5 = static_cast<uint8_t>((pixel >> 0) & 0x1F); // Red (bits 0-4)
			uint8_t g5 = static_cast<uint8_t>((pixel >> 5) & 0x1F); // Green (bits 5-9)
			uint8_t b5 = static_cast<uint8_t>((pixel >> 10) & 0x1F); // Blue (bits 10-14)
			rgba[i * 4 + 0] = (r5 << 3) | (r5 >> 2); // Red
			rgba[i * 4 + 1] = (g5 << 3) | (g5 >> 2); // Green
			rgba[i * 4 + 2] = (b5 << 3) | (b5 >> 2); // Blue
			rgba[i * 4 + 3] = 0xFF; // Alpha
		}

		return rgba;
	}

} // namespace TextureUtils

namespace CameraUtils
{

	MatrixSet computeMatrices(const CameraState& camera, uint32_t width, uint32_t height, float autoRotateY)
	{
		MatrixSet result;

		result.model = glm::mat4(1.0f);
		result.model = glm::translate(result.model,
			glm::vec3(-camera.cameraOffset.x, -camera.cameraOffset.y, -camera.cameraOffset.z) + glm::vec3(0.0f, 2.5f, 0.0f));
		result.model = glm::rotate(result.model, camera.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		result.model = glm::rotate(result.model, camera.rotation.y + autoRotateY, glm::vec3(0.0f, 1.0f, 0.0f));
		result.model = glm::rotate(result.model, camera.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		result.model = glm::translate(result.model, glm::vec3(0.0f, -2.5f, 0.0f));

		float cameraDistance = camera.baseCameraDistance * camera.zoom;
		result.view = glm::lookAt(
			glm::vec3(0.0f, 0.0f, cameraDistance),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));

		float aspect = static_cast<float>(width) / static_cast<float>(height);
		result.projection = glm::perspective(glm::radians(80.0f), aspect, 0.1f, 500.0f);

		return result;
	}

} // namespace CameraUtils

namespace UniformBufferUtils
{

	UniformBufferData buildUniformBufferData(
		const CameraUtils::MatrixSet& matrices,
		const LightingState& lighting,
		bool flipProjectionY)
	{
		UniformBufferData data;

		data.model = matrices.model;
		data.view = matrices.view;
		data.projection = matrices.projection;

		if (flipProjectionY)
		{
			data.projection[1][1] *= -1;
		}

		for (int i = 0; i < 3; ++i)
		{
			data.lightDirs[i] = glm::vec4(lighting.lightDirs[i], 1.0f);
			data.lightColors[i] = glm::vec4(lighting.lightColors[i], 1.0f);
		}

		data.ambient = glm::vec4(lighting.ambientLight, 1.0f);
		data.time = glm::vec4(0.0f);

		return data;
	}

} // namespace UniformBufferUtils

namespace AnimationUtils
{

	float computeAnimationTime(double elapsedSeconds, float frameLength, float speed)
	{
		if (frameLength <= 0.0f)
			return 0.0f;

		float finalSpeed = speed;
		if (finalSpeed <= 0.0001f)
			finalSpeed = 1.0f;

		float unitsPerSecond = finalSpeed * 60.0f;

		return std::fmod(static_cast<float>(elapsedSeconds) * unitsPerSecond, frameLength);
	}

	std::unordered_map<uint32_t, float> computeShapeWeights(
		const PS2Icon::Icon* icon,
		float animTime,
		float duration)
	{
		std::unordered_map<uint32_t, float> shapeValues;

		if (!icon)
			return shapeValues;

		for (const auto& frame : icon->getFrames())
		{
			std::vector<PS2Icon::AnimationKey> keys = frame.keys;
			if (frame.shapeId == 0)
			{
				keys.push_back(PS2Icon::AnimationKey{0.0f, 1.0f});
			}

			if (keys.empty())
				continue;

			const PS2Icon::AnimationKey* last = nullptr;
			const PS2Icon::AnimationKey* next = nullptr;
			float lastTime = 0.0f;
			float nextTime = 0.0f;

			for (const auto& key : keys)
			{
				float tBelow = (key.time <= animTime) ? key.time : key.time - duration;
				float tAbove = (key.time >= animTime) ? key.time : key.time + duration;

				if (!last || tBelow > lastTime)
				{
					last = &key;
					lastTime = tBelow;
				}

				if (!next || tAbove < nextTime)
				{
					next = &key;
					nextTime = tAbove;
				}
			}

			float progress = (nextTime > lastTime) ? ((animTime - lastTime) / (nextTime - lastTime)) : 0.0f;
			float value = 0.0f;
			if (last && next)
			{
				value = (1.0f - progress) * last->value + progress * next->value;
			}
			else if (last)
			{
				value = last->value;
			}

			shapeValues[frame.shapeId] = value;
		}

		if (shapeValues.empty())
		{
			shapeValues[0] = 1.0f;
		}

		float sum = 0.0f;
		for (const auto& kv : shapeValues)
			sum += kv.second;

		if (sum <= 0.0f)
		{
			shapeValues.clear();
			shapeValues[0] = 1.0f;
			sum = 1.0f;
		}

		for (auto& kv : shapeValues)
			kv.second /= sum;

		return shapeValues;
	}

	std::vector<BlendedVertex> blendVertices(
		const PS2Icon::Icon* icon,
		const std::unordered_map<uint32_t, float>& shapeWeights,
		uint32_t vertexCount)
	{
		std::vector<BlendedVertex> vertices(vertexCount);

		if (!icon)
			return vertices;

		const auto* normalData = icon->getNormalUVData();
		const auto* colorData = icon->getColorData();

		for (uint32_t i = 0; i < vertexCount; ++i)
		{
			float px = 0.0f, py = 0.0f, pz = 0.0f;

			for (const auto& kv : shapeWeights)
			{
				const auto* vdata = icon->getVertexData(kv.first);
				if (!vdata)
					continue;
				px += kv.second * static_cast<float>(vdata[i].x);
				py += kv.second * static_cast<float>(vdata[i].y);
				pz += kv.second * static_cast<float>(vdata[i].z);
			}

			vertices[i].pos[0] = px;
			vertices[i].pos[1] = py;
			vertices[i].pos[2] = pz;

			vertices[i].normal[0] = static_cast<float>(normalData[i].nx);
			vertices[i].normal[1] = static_cast<float>(normalData[i].ny);
			vertices[i].normal[2] = static_cast<float>(normalData[i].nz);
			vertices[i].texCoord[0] = static_cast<float>(normalData[i].u);
			vertices[i].texCoord[1] = static_cast<float>(normalData[i].v);
			vertices[i].color[0] = colorData[i].r;
			vertices[i].color[1] = colorData[i].g;
			vertices[i].color[2] = colorData[i].b;
			vertices[i].color[3] = colorData[i].a;
		}

		return vertices;
	}

} // namespace AnimationUtils
