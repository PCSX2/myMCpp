// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <string>
#include <vector>
#include <cstdint>

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
		uint32_t getAnimationShapes() const { return m_animationShapes; }
		uint32_t getTextureFlags() const { return m_textureFlags; }
		bool hasTexture() const { return (m_textureFlags & TEX_FLAG_EXISTS) != 0; }
		bool isTextureCompressed() const { return (m_textureFlags & TEX_FLAG_COMPRESSED) != 0; }
		uint32_t getVertexCount() const { return m_vertexCount; }
		uint32_t getFrameLength() const { return m_frameLength; }
		float getAnimSpeed() const { return m_animSpeed; }
		uint32_t getPlayOffset() const { return m_playOffset; }
		uint32_t getFrameCount() const { return m_frameCount; }
		bool hasAlpha() const { return m_enableAlpha; }

		// Get vertex data for specific animation shape
		const VertexCoord* getVertexData(uint32_t shapeIndex = 0) const;

		// Get normal/UV data (shared across all shapes)
		const NormalUV* getNormalUVData() const { return m_normalUVData.data(); }

		// Get color data (shared across all shapes)
		const VertexColor* getColorData() const { return m_colorData.data(); }

		// Get animation frames
		const std::vector<AnimationFrame>& getFrames() const { return m_frames; }

		// Get texture data (16-bit RGBA5551 format)
		const uint16_t* getTextureData() const;
		int getTextureWidth() const { return TEXTURE_WIDTH; }
		int getTextureHeight() const { return TEXTURE_HEIGHT; }

		// Check if icon loaded successfully
		bool isValid() const { return m_valid; }
		const std::string& getError() const { return m_errorMsg; }

	private:
		bool m_valid;
		std::string m_errorMsg;

		// Icon header data
		uint32_t m_animationShapes;
		uint32_t m_textureFlags;
		uint32_t m_vertexCount;

		// Vertex data (3D coordinates per shape)
		std::vector<VertexCoord> m_vertexData; // Size: m_animationShapes * m_vertexCount

		// Shared vertex attributes
		std::vector<NormalUV> m_normalUVData; // Size: m_vertexCount
		std::vector<VertexColor> m_colorData; // Size: m_vertexCount

		// Animation data
		uint32_t m_frameLength;
		float m_animSpeed;
		uint32_t m_playOffset;
		uint32_t m_frameCount;
		std::vector<AnimationFrame> m_frames;

		// Texture data (RGBA5551 format)
		std::vector<uint16_t> m_texture;
		bool m_enableAlpha;

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
