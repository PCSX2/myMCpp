// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "ps2icon.h"
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
		: valid(false)
		, animationShapes(0)
		, textureFlags(0)
		, vertexCount(0)
		, frameLength(0)
		, animSpeed(0.0f)
		, playOffset(0)
		, frameCount(0)
		, enableAlpha(false)
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
		valid = false;
		errorMsg.clear();

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

			valid = true;
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
		animationShapes = readLE<uint32_t>(data, offset + 4);
		uint32_t texType = readLE<uint32_t>(data, offset + 8);
		vertexCount = readLE<uint32_t>(data, offset + 16);

		if (magic != ICON_MAGIC)
		{
			throw std::runtime_error("Invalid icon magic");
		}

		textureFlags = texType;

		return offset + ICON_HDR_SIZE;
	}

	size_t Icon::loadVertexData(const uint8_t* data, size_t length, size_t offset)
	{
		size_t stride = VERTEX_COORDS_SIZE * animationShapes + NORMAL_UV_COLOR_SIZE;

		if (length < offset + vertexCount * stride)
		{
			throw std::runtime_error("Icon file too small for vertex data");
		}

		vertexData.resize(animationShapes * vertexCount);
		normalUVData.resize(vertexCount);
		colorData.resize(vertexCount);

		for (uint32_t i = 0; i < vertexCount; ++i)
		{
			for (uint32_t s = 0; s < animationShapes; ++s)
			{
				size_t vertexIdx = s * vertexCount + i;
				vertexData[vertexIdx].x = readLE<int16_t>(data, offset);
				vertexData[vertexIdx].y = readLE<int16_t>(data, offset + 2);
				vertexData[vertexIdx].z = readLE<int16_t>(data, offset + 4);
				offset += VERTEX_COORDS_SIZE;
			}

			normalUVData[i].nx = readLE<int16_t>(data, offset);
			normalUVData[i].ny = readLE<int16_t>(data, offset + 2);
			normalUVData[i].nz = readLE<int16_t>(data, offset + 4);
			normalUVData[i].u = readLE<int16_t>(data, offset + 8);
			normalUVData[i].v = readLE<int16_t>(data, offset + 10);

			colorData[i].r = data[offset + 12];
			colorData[i].g = data[offset + 13];
			colorData[i].b = data[offset + 14];
			colorData[i].a = data[offset + 15];

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
		frameLength = readLE<uint32_t>(data, offset + 4);

		uint32_t animSpeedBits = readLE<uint32_t>(data, offset + 8);
		std::memcpy(&animSpeed, &animSpeedBits, sizeof(float));

		playOffset = readLE<uint32_t>(data, offset + 12);
		frameCount = readLE<uint32_t>(data, offset + 16);

		offset += ANIM_HDR_SIZE;

		if (animIdTag != 0x01)
		{
			throw std::runtime_error("Invalid animation ID tag");
		}


		if (static_cast<size_t>(frameCount) > (length - offset) / FRAME_DATA_SIZE)
		{
			throw std::runtime_error("Icon file too small for animation frames");
		}


		frames.resize(frameCount);

		uint32_t loadedFrames = 0;

		for (uint32_t i = 0; i < frameCount; ++i)
		{
			if (length < offset + FRAME_DATA_SIZE)
			{
				Logger::warn("PS2Icon: EOF reached reading frame header {}/{}, stopping.", i, frameCount);
				break;
			}

			uint32_t shapeId = readLE<uint32_t>(data, offset);
			uint32_t rawKeyCount = readLE<uint32_t>(data, offset + 4);

			uint32_t keyCount = (rawKeyCount > 0) ? (rawKeyCount - 1) : 0;

			if (static_cast<size_t>(keyCount) > (length - (offset + FRAME_DATA_SIZE)) / FRAME_KEY_SIZE)
			{
				Logger::warn("PS2Icon: Invalid key count {} at frame {}/{} (avail bytes: {}). Truncating animation.",
					keyCount, i, frameCount, length - (offset + FRAME_DATA_SIZE));
				break;
			}

			offset += FRAME_DATA_SIZE;

			frames[i].shapeId = shapeId;
			frames[i].keys.resize(keyCount);

			for (uint32_t k = 0; k < keyCount; ++k)
			{
				uint32_t timeBits = readLE<uint32_t>(data, offset);
				uint32_t valueBits = readLE<uint32_t>(data, offset + 4);

				std::memcpy(&frames[i].keys[k].time, &timeBits, sizeof(float));
				std::memcpy(&frames[i].keys[k].value, &valueBits, sizeof(float));

				offset += FRAME_KEY_SIZE;
			}

			loadedFrames++;
		}

		if (loadedFrames < frameCount)
		{
			frames.resize(loadedFrames);
			Logger::info("PS2Icon: Loaded {}/{} frames", loadedFrames, frameCount);
		}

		return offset;
	}

	size_t Icon::loadTexture(const uint8_t* data, size_t length, size_t offset)
	{
		// Check if texture data exists (bit 2 = 0x04)
		if (!(textureFlags & 0x04))
		{
			Logger::info("PS2Icon: No texture data (textureFlags=0x{:02X})", textureFlags);
			texture.resize(1);
			texture[0] = 0xFFFF;
			return offset;
		}

		if (offset >= length)
		{
			Logger::warn("PS2Icon: Texture flag set but no data remaining");
			texture.resize(1);
			texture[0] = 0xFFFF;
			return offset;
		}

		// Check if texture is compressed (bit 3 = 0x08)
		bool isCompressed = (textureFlags & 0x08) != 0;
		Logger::info("PS2Icon: textureFlags=0x{:02X}, compressed={}", textureFlags, isCompressed);

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

		texture.resize(TEXTURE_WIDTH * TEXTURE_HEIGHT);

		size_t pixelsToRead = std::min(avail, static_cast<size_t>(TEXTURE_SIZE)) / 2;

		for (size_t i = 0; i < pixelsToRead; ++i)
		{
			texture[i] = readLE<uint16_t>(data, offset + i * 2);
		}

		for (size_t i = pixelsToRead; i < TEXTURE_WIDTH * TEXTURE_HEIGHT; ++i)
		{
			texture[i] = 0;
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
		texture.resize(TEXTURE_WIDTH * TEXTURE_HEIGHT);

		size_t texOffset = 0;
		size_t rleOffset = 0;

		while (rleOffset < compressedSize)
		{
			uint16_t rleCode = readLE<uint16_t>(data, offset + rleOffset);
			rleOffset += 2;

			if ((rleCode & 0xFF00) == 0xFF00)
			{
				uint32_t subLength = (0x10000 - rleCode);

				if (compressedSize < rleOffset + subLength * 2)
				{
					throw std::runtime_error("Compressed texture data too short");
				}

				if (texOffset + subLength > texture.size())
				{
					throw std::runtime_error("Decompressed texture exceeds size");
				}

				for (uint32_t i = 0; i < subLength; ++i)
				{
					texture[texOffset++] = readLE<uint16_t>(data, offset + rleOffset);
					rleOffset += 2;
				}
			}
			else
			{
				uint32_t rep = rleCode;

				if (compressedSize < rleOffset + 2)
				{
					Logger::warn("PS2Icon: Compressed texture data too short (Repeat).");
					break;
				}

				if (texOffset + rep > texture.size())
				{
					throw std::runtime_error("Decompressed texture exceeds size");
				}

				uint16_t value = readLE<uint16_t>(data, offset + rleOffset);
				rleOffset += 2;

				for (uint32_t i = 0; i < rep; ++i)
				{
					texture[texOffset++] = value;
				}
			}
		}

		return offset + compressedSize;
	}

	const VertexCoord* Icon::getVertexData(uint32_t shapeIndex) const
	{
		if (shapeIndex >= animationShapes || vertexData.empty())
		{
			return nullptr;
		}

		return &vertexData[shapeIndex * vertexCount];
	}

	const uint16_t* Icon::getTextureData() const
	{
		return texture.empty() ? nullptr : texture.data();
	}

	void Icon::setError(const std::string& msg)
	{
		valid = false;
		errorMsg = "Icon error: " + msg;
	}

} // namespace PS2Icon
