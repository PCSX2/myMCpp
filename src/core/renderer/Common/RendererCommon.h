// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "../Renderer.h"
#include <glm/glm.hpp>
#include <array>
#include <vector>
#include <unordered_map>
#include <cstdint>

class PS2IconSys;

struct LightingState
{
	std::array<glm::vec3, 3> lightDirs;
	std::array<glm::vec3, 3> lightColors;
	glm::vec3 ambientLight;

	std::array<glm::vec3, 3> iconSysLightDirs;
	std::array<glm::vec3, 3> iconSysLightColors;
	glm::vec3 iconSysAmbient;
	bool hasIconSysLighting;
	LightingMode mode;

	LightingState();

	void applyMode();
	void loadFromIconSys(PS2IconSys* iconSys);
};

struct CameraState
{
	glm::vec3 rotation;
	float zoom;
	float baseCameraDistance;
	glm::vec3 cameraOffset;
	CameraMode mode;

	CameraState();

	void applyMode();
	void setRotation(float x, float y, float z);
	void setZoom(float zoom);
	void setOffset(float x, float y, float z);
	void reset();
};

struct BackgroundState
{
	std::array<glm::vec4, 4> colors; // 4 corner colors with alpha
	float alpha;
	bool shouldRender;

	BackgroundState();

	bool loadFromIconSys(PS2IconSys* iconSys);
	void setColor(float r, float g, float b, float a = 1.0f);
};

namespace LightingPresets
{
	constexpr glm::vec3 OFF_AMBIENT{1.0f, 1.0f, 1.0f};

	glm::vec3 getAlt1LightDir(int index);
	glm::vec3 getAlt1LightColor(int index);
	constexpr glm::vec3 ALT1_AMBIENT{0.55f, 0.55f, 0.55f};

	glm::vec3 getAlt2LightDir(int index);
	glm::vec3 getAlt2LightColor(int index);
	constexpr glm::vec3 ALT2_AMBIENT{0.5f, 0.5f, 0.5f};

	glm::vec3 getDefaultLightDir(int index);
	constexpr glm::vec3 DEFAULT_LIGHT_COLOR{1.0f, 1.0f, 1.0f};
	constexpr glm::vec3 DEFAULT_AMBIENT{0.5f, 0.5f, 0.5f};
} // namespace LightingPresets

namespace CameraPresets
{
	struct CameraConfig
	{
		glm::vec3 offset;
		float distance;
	};

	constexpr CameraConfig DEFAULT_CAMERA{{0.0f, 2.5f, 0.0f}, 5.0f};
	constexpr CameraConfig FLAT_CAMERA{{0.0f, 0.0f, 0.0f}, 5.0f};
	constexpr CameraConfig NEAR_CAMERA{{0.0f, 2.5f, 0.0f}, 3.8f};
	constexpr CameraConfig HIGH_CAMERA{{0.0f, 4.0f, 0.0f}, 5.5f};
} // namespace CameraPresets

namespace PS2Icon
{
	class Icon;
	struct VertexCoord;
	struct NormalUV;
	struct VertexColor;
} // namespace PS2Icon

namespace TextureUtils
{
	std::vector<uint8_t> convertPS2TextureToRGBA(const uint16_t* textureData, uint32_t width, uint32_t height);
} // namespace TextureUtils

namespace CameraUtils
{
	struct MatrixSet
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
	};

	MatrixSet computeMatrices(const CameraState& camera, uint32_t width, uint32_t height, float autoRotateY = 0.0f);
} // namespace CameraUtils

namespace UniformBufferUtils
{
	struct UniformBufferData
	{
		glm::mat4 model; // offset 0
		glm::mat4 view; // offset 64
		glm::mat4 projection; // offset 128
		glm::vec4 lightDirs[3]; // offset 192 (vec3 padded to vec4)
		glm::vec4 lightColors[3]; // offset 240
		glm::vec4 ambient; // offset 288
		glm::vec4 time; // offset 304
	};
	static_assert(sizeof(UniformBufferData) == 320, "UniformBufferData must be 320 bytes");

	UniformBufferData buildUniformBufferData(
		const CameraUtils::MatrixSet& matrices,
		const LightingState& lighting,
		bool flipProjectionY = false);
} // namespace UniformBufferUtils

namespace AnimationUtils
{
	struct BlendedVertex
	{
		float pos[3];
		float normal[3];
		float texCoord[2];
		uint8_t color[4];
	};

	std::unordered_map<uint32_t, float> computeShapeWeights(
		const PS2Icon::Icon* icon,
		float animTime,
		float duration);

	std::vector<BlendedVertex> blendVertices(
		const PS2Icon::Icon* icon,
		const std::unordered_map<uint32_t, float>& shapeWeights,
		uint32_t vertexCount);

	float computeAnimationTime(double elapsedSeconds, float frameLength);
} // namespace AnimationUtils
