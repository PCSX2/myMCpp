// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

// PS2 3D icon file format (.icn)

namespace PS2Icon
{

	constexpr uint32_t ICON_MAGIC = 0x010000;
	constexpr int TEXTURE_WIDTH = 128;
	constexpr int TEXTURE_HEIGHT = 128;
	constexpr int TEXTURE_SIZE = TEXTURE_WIDTH * TEXTURE_HEIGHT * 2;
	constexpr float FIXED_POINT_FACTOR = 4096.0f;
	constexpr uint32_t TEX_FLAG_EXISTS = 0x04;
	constexpr uint32_t TEX_FLAG_COMPRESSED = 0x08;

	struct AnimationKey
	{
		float time;
		float value;
	};

	struct AnimationFrame
	{
		uint32_t shapeId; // Which animation shape to use
		std::vector<AnimationKey> keys; // Interpolation keys for the frame
	};

	struct VertexCoord
	{
		int16_t x, y, z;
	};

	struct NormalUV
	{
		int16_t nx, ny, nz; // Normal vector
		int16_t u, v; // Texture coordinates
	};

	struct VertexColor
	{
		uint8_t r, g, b, a;
	};

	class Icon
	{
	public:
		Icon();
		~Icon();

		// Load icon from binary data
		bool load(const std::vector<uint8_t>& data);
		bool load(const std::string& data);

		// Getters for icon properties
		uint32_t getAnimationShapes() const { return animationShapes; }
		uint32_t getTextureFlags() const { return textureFlags; }
		bool hasTexture() const { return (textureFlags & TEX_FLAG_EXISTS) != 0; }
		bool isTextureCompressed() const { return (textureFlags & TEX_FLAG_COMPRESSED) != 0; }
		uint32_t getVertexCount() const { return vertexCount; }
		uint32_t getFrameLength() const { return frameLength; }
		float getAnimSpeed() const { return animSpeed; }
		uint32_t getPlayOffset() const { return playOffset; }
		uint32_t getFrameCount() const { return frameCount; }
		bool hasAlpha() const { return enableAlpha; }

		// Get vertex data for specific animation shape
		const VertexCoord* getVertexData(uint32_t shapeIndex = 0) const;

		// Get normal/UV data (shared across all shapes)
		const NormalUV* getNormalUVData() const { return normalUVData.data(); }

		// Get color data (shared across all shapes)
		const VertexColor* getColorData() const { return colorData.data(); }

		// Get animation frames
		const std::vector<AnimationFrame>& getFrames() const { return frames; }

		// Get texture data (16-bit RGBA5551 format)
		const uint16_t* getTextureData() const;
		int getTextureWidth() const { return TEXTURE_WIDTH; }
		int getTextureHeight() const { return TEXTURE_HEIGHT; }

		// Check if icon loaded successfully
		bool isValid() const { return valid; }
		const std::string& getError() const { return errorMsg; }

	private:
		bool valid;
		std::string errorMsg;

		// Icon header data
		uint32_t animationShapes;
		uint32_t textureFlags;
		uint32_t vertexCount;

		// Vertex data (3D coordinates per shape)
		std::vector<VertexCoord> vertexData; // Size: animationShapes * vertexCount

		// Shared vertex attributes
		std::vector<NormalUV> normalUVData; // Size: vertexCount
		std::vector<VertexColor> colorData; // Size: vertexCount

		// Animation data
		uint32_t frameLength;
		float animSpeed;
		uint32_t playOffset;
		uint32_t frameCount;
		std::vector<AnimationFrame> frames;

		// Texture data (RGBA5551 format)
		std::vector<uint16_t> texture;
		bool enableAlpha;

		// Loading funcs
		size_t loadHeader(const uint8_t* data, size_t length, size_t offset);
		size_t loadVertexData(const uint8_t* data, size_t length, size_t offset);
		size_t loadAnimationData(const uint8_t* data, size_t length, size_t offset);
		size_t loadTexture(const uint8_t* data, size_t length, size_t offset);
		size_t loadTextureUncompressed(const uint8_t* data, size_t length, size_t offset);
		size_t loadTextureCompressed(const uint8_t* data, size_t length, size_t offset);

		void setError(const std::string& msg);
	};

} // namespace PS2Icon
