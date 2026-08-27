// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+
// PS2 icon parsing is adapted from mymc++ / mymc icon handling and community PS2 icon format documentation.

#include "PS2Icon.h"
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include "Logger.h"

namespace PS2Icon
{

	constexpr size_t ICON_HDR_SIZE = 20;
	constexpr size_t VERTEX_COORDS_SIZE = 8;
	constexpr size_t NORMAL_UV_COLOR_SIZE = 16;
	constexpr size_t ANIM_HDR_SIZE = 20;
	constexpr size_t FRAME_DATA_SIZE = 16;
	constexpr size_t FRAME_KEY_SIZE = 8;

	template <typename T>
	static T readLE(const uint8_t* data)
	{
		T value = 0;
		for (size_t i = 0; i < sizeof(T); ++i)
		{
			value |= static_cast<T>(data[i]) << (i * 8);
		}
		return value;
	}

	template <typename T>
	static T readLE(const uint8_t* data, size_t offset)
	{
		return readLE<T>(data + offset);
	}

	Icon::Icon()
		: m_valid(false)
		, m_animationShapes(0)
		, m_textureFlags(0)
		, m_vertexCount(0)
		, m_frameLength(0)
		, m_animSpeed(0.0f)
		, m_playOffset(0)
		, m_frameCount(0)
		, m_enableAlpha(false)
	{
	}

	Icon::~Icon()
	{
	}

	bool Icon::load(const std::vector<uint8_t>& data)
	{
		return load(std::string(reinterpret_cast<const char*>(data.data()), data.size()));
	}

	bool Icon::load(const std::string& data)
	{
		m_valid = false;
		m_errorMsg.clear();

		const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.data());
		size_t length = data.size();
		size_t offset = 0;

		Logger::info("PS2Icon::load: Start loading. Length: {}", length);

		try
		{
			offset = loadHeader(bytes, length, offset);
			Logger::info("PS2Icon::load: Header loaded. Offset: {}", offset);
			offset = loadVertexData(bytes, length, offset);
			Logger::info("PS2Icon::load: VertexData loaded. Offset: {}", offset);
			offset = loadAnimationData(bytes, length, offset);
			Logger::info("PS2Icon::load: AnimationData loaded. Offset: {}", offset);
			offset = loadTexture(bytes, length, offset);
			Logger::info("PS2Icon::load: Texture loaded. Offset: {}", offset);

			if (length > offset)
			{
				Logger::warn("PS2Icon::load: Extra data at end of file ({} bytes)", length - offset);
			}

			m_valid = true;
			return true;
		}
		catch (const std::exception& e)
		{
			Logger::error("PS2Icon::load: Exception caught: {}", e.what());
			setError(e.what());
			return false;
		}
	}

	size_t Icon::loadHeader(const uint8_t* data, size_t length, size_t offset)
	{
		if (length < ICON_HDR_SIZE)
		{
			throw std::runtime_error("Icon file too small");
		}

		uint32_t magic = readLE<uint32_t>(data, offset);
		m_animationShapes = readLE<uint32_t>(data, offset + 4);
		uint32_t texType = readLE<uint32_t>(data, offset + 8);
		m_vertexCount = readLE<uint32_t>(data, offset + 16);

		if (magic != ICON_MAGIC)
		{
			throw std::runtime_error("Invalid icon magic");
		}

		m_textureFlags = texType;

		return offset + ICON_HDR_SIZE;
	}

	size_t Icon::loadVertexData(const uint8_t* data, size_t length, size_t offset)
	{
		size_t stride = VERTEX_COORDS_SIZE * m_animationShapes + NORMAL_UV_COLOR_SIZE;

		if (length < offset + m_vertexCount * stride)
		{
			throw std::runtime_error("Icon file too small for vertex data");
		}

		m_vertexData.resize(static_cast<size_t>(m_animationShapes) * m_vertexCount);
		m_normalUVData.resize(m_vertexCount);
		m_colorData.resize(m_vertexCount);

		for (uint32_t i = 0; i < m_vertexCount; ++i)
		{
			for (uint32_t s = 0; s < m_animationShapes; ++s)
			{
				size_t vertexIdx = static_cast<size_t>(s) * m_vertexCount + i;
				m_vertexData[vertexIdx].x = readLE<int16_t>(data, offset);
				m_vertexData[vertexIdx].y = readLE<int16_t>(data, offset + 2);
				m_vertexData[vertexIdx].z = readLE<int16_t>(data, offset + 4);
				offset += VERTEX_COORDS_SIZE;
			}

			m_normalUVData[i].nx = readLE<int16_t>(data, offset);
			m_normalUVData[i].ny = readLE<int16_t>(data, offset + 2);
			m_normalUVData[i].nz = readLE<int16_t>(data, offset + 4);
			m_normalUVData[i].u = readLE<int16_t>(data, offset + 8);
			m_normalUVData[i].v = readLE<int16_t>(data, offset + 10);

			m_colorData[i].r = data[offset + 12];
			m_colorData[i].g = data[offset + 13];
			m_colorData[i].b = data[offset + 14];
			m_colorData[i].a = data[offset + 15];

			offset += NORMAL_UV_COLOR_SIZE;
		}

		return offset;
	}

	size_t Icon::loadAnimationData(const uint8_t* data, size_t length, size_t offset)
	{
		if (length < offset + ANIM_HDR_SIZE)
		{
			throw std::runtime_error("Icon file too small for animation header");
		}

		uint32_t animIdTag = readLE<uint32_t>(data, offset);
		m_frameLength = readLE<uint32_t>(data, offset + 4);

		uint32_t animSpeedBits = readLE<uint32_t>(data, offset + 8);
		std::memcpy(&m_animSpeed, &animSpeedBits, sizeof(float));

		m_playOffset = readLE<uint32_t>(data, offset + 12);
		m_frameCount = readLE<uint32_t>(data, offset + 16);

		offset += ANIM_HDR_SIZE;

		if (animIdTag != 0x01)
		{
			throw std::runtime_error("Invalid animation ID tag");
		}

		if (static_cast<size_t>(m_frameCount) > (length - offset) / FRAME_DATA_SIZE)
		{
			throw std::runtime_error("Icon file too small for animation frames");
		}

		m_frames.resize(m_frameCount);

		uint32_t loadedFrames = 0;

		for (uint32_t i = 0; i < m_frameCount; ++i)
		{
			if (length < offset + FRAME_DATA_SIZE)
			{
				Logger::warn("PS2Icon: EOF reached reading frame header {}/{}, stopping.", i, m_frameCount);
				break;
			}

			uint32_t shapeId = readLE<uint32_t>(data, offset);
			uint32_t rawKeyCount = readLE<uint32_t>(data, offset + 4);

			uint32_t keyCount = (rawKeyCount > 0) ? (rawKeyCount - 1) : 0;

			if (static_cast<size_t>(keyCount) > (length - (offset + FRAME_DATA_SIZE)) / FRAME_KEY_SIZE)
			{
				Logger::warn("PS2Icon: Invalid key count {} at frame {}/{} (avail bytes: {}). Truncating animation.",
					keyCount, i, m_frameCount, length - (offset + FRAME_DATA_SIZE));
				break;
			}

			offset += FRAME_DATA_SIZE;

			m_frames[i].shapeId = shapeId;
			m_frames[i].keys.resize(keyCount);

			for (uint32_t k = 0; k < keyCount; ++k)
			{
				uint32_t timeBits = readLE<uint32_t>(data, offset);
				uint32_t valueBits = readLE<uint32_t>(data, offset + 4);

				std::memcpy(&m_frames[i].keys[k].time, &timeBits, sizeof(float));
				std::memcpy(&m_frames[i].keys[k].value, &valueBits, sizeof(float));

				offset += FRAME_KEY_SIZE;
			}

			loadedFrames++;
		}

		if (loadedFrames < m_frameCount)
		{
			m_frames.resize(loadedFrames);
			Logger::info("PS2Icon: Loaded {}/{} frames", loadedFrames, m_frameCount);
		}

		return offset;
	}

	size_t Icon::loadTexture(const uint8_t* data, size_t length, size_t offset)
	{
		constexpr uint16_t DEFAULT_TEXEL = 0xFFFF;
		const size_t texturePixelCount = static_cast<size_t>(TEXTURE_WIDTH) * static_cast<size_t>(TEXTURE_HEIGHT);

		// Check if texture data exists (bit 2 = 0x04)
		if (!(m_textureFlags & 0x04))
		{
			Logger::info("PS2Icon: No texture data (textureFlags=0x{:02X})", m_textureFlags);
			m_texture.assign(texturePixelCount, DEFAULT_TEXEL);
			return offset;
		}

		if (offset >= length)
		{
			Logger::warn("PS2Icon: Texture flag set but no data remaining");
			m_texture.assign(texturePixelCount, DEFAULT_TEXEL);
			return offset;
		}

		// Check if texture is compressed (bit 3 = 0x08)
		bool isCompressed = (m_textureFlags & 0x08) != 0;
		Logger::info("PS2Icon: textureFlags=0x{:02X}, compressed={}", m_textureFlags, isCompressed);

		if (isCompressed)
		{
			return loadTextureCompressed(data, length, offset);
		}
		else
		{
			return loadTextureUncompressed(data, length, offset);
		}
	}

	size_t Icon::loadTextureUncompressed(const uint8_t* data, size_t length, size_t offset)
	{
		size_t avail = (length > offset) ? length - offset : 0;
		if (avail < TEXTURE_SIZE)
		{
			Logger::warn("PS2Icon: Texture truncated. Expected {} bytes, got {}. Padding with zeros.", TEXTURE_SIZE, avail);
		}

		m_texture.resize(TEXTURE_WIDTH * TEXTURE_HEIGHT);

		size_t pixelsToRead = std::min(avail, static_cast<size_t>(TEXTURE_SIZE)) / 2;

		for (size_t i = 0; i < pixelsToRead; ++i)
		{
			m_texture[i] = readLE<uint16_t>(data, offset + i * 2);
		}

		for (size_t i = pixelsToRead; i < TEXTURE_WIDTH * TEXTURE_HEIGHT; ++i)
		{
			m_texture[i] = 0;
		}

		return std::min(length, offset + TEXTURE_SIZE);
	}

	size_t Icon::loadTextureCompressed(const uint8_t* data, size_t length, size_t offset)
	{
		if (length < offset + 4)
		{
			throw std::runtime_error("Icon file too small for compressed texture header");
		}

		uint32_t compressedSize = readLE<uint32_t>(data, offset);
		Logger::info("PS2Icon: Compressed texture size: {}", compressedSize);
		offset += 4;

		if (length < offset + compressedSize)
		{
			throw std::runtime_error("Icon file too small for compressed texture data");
		}

		if (compressedSize % 2 != 0)
		{
			throw std::runtime_error("Compressed texture size is odd");
		}

		// Decompress RLE texture data
		m_texture.resize(TEXTURE_WIDTH * TEXTURE_HEIGHT);

		size_t texOffset = 0;
		size_t rleOffset = 0;

		while (rleOffset < compressedSize)
		{
			uint16_t rleCode = readLE<uint16_t>(data, offset + rleOffset);
			rleOffset += 2;

			if ((rleCode & 0x8000) != 0)
			{
				uint32_t subLength = (0x10000 - rleCode);

				if (compressedSize < rleOffset + subLength * 2)
				{
					throw std::runtime_error("Compressed texture data too short");
				}

				if (texOffset + subLength > m_texture.size())
				{
					throw std::runtime_error("Decompressed texture exceeds size");
				}

				for (uint32_t i = 0; i < subLength; ++i)
				{
					m_texture[texOffset++] = readLE<uint16_t>(data, offset + rleOffset);
					rleOffset += 2;
				}
			}
			else
			{
				uint32_t rep = rleCode;

				if (rep > 0)
				{
					if (compressedSize < rleOffset + 2)
					{
						Logger::warn("PS2Icon: Compressed texture data too short (Repeat).");
						break;
					}

					if (texOffset + rep > m_texture.size())
					{
						throw std::runtime_error("Decompressed texture exceeds size");
					}

					uint16_t value = readLE<uint16_t>(data, offset + rleOffset);
					rleOffset += 2;

					for (uint32_t i = 0; i < rep; ++i)
					{
						m_texture[texOffset++] = value;
					}
				}
			}
		}

		return offset + compressedSize;
	}

	const VertexCoord* Icon::getVertexData(uint32_t shapeIndex) const
	{
		if (shapeIndex >= m_animationShapes || m_vertexData.empty())
		{
			return nullptr;
		}

		return &m_vertexData[static_cast<size_t>(shapeIndex) * m_vertexCount];
	}

	const uint16_t* Icon::getTextureData() const
	{
		return m_texture.empty() ? nullptr : m_texture.data();
	}

	void Icon::setError(const std::string& msg)
	{
		m_valid = false;
		m_errorMsg = "Icon error: " + msg;
	}

} // namespace PS2Icon
