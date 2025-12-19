// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "ps2icon.h"
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include "../../common/Logger.h"

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
				// Warning: Icon file larger than expected.
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

		frames.resize(frameCount);

		for (uint32_t i = 0; i < frameCount; ++i)
		{
			if (length < offset + FRAME_DATA_SIZE)
			{
				throw std::runtime_error("Icon file too small for frame data");
			}

			frames[i].shapeId = readLE<uint32_t>(data, offset);
			uint32_t keyCount = readLE<uint32_t>(data, offset + 4);

			keyCount -= 1;

			offset += FRAME_DATA_SIZE;

			frames[i].keys.resize(keyCount);

			for (uint32_t k = 0; k < keyCount; ++k)
			{
				if (length < offset + FRAME_KEY_SIZE)
				{
					throw std::runtime_error("Icon file too small for frame keys");
				}

				uint32_t timeBits = readLE<uint32_t>(data, offset);
				uint32_t valueBits = readLE<uint32_t>(data, offset + 4);

				std::memcpy(&frames[i].keys[k].time, &timeBits, sizeof(float));
				std::memcpy(&frames[i].keys[k].value, &valueBits, sizeof(float));

				offset += FRAME_KEY_SIZE;
			}
		}

		return offset;
	}

	size_t Icon::loadTexture(const uint8_t* data, size_t length, size_t offset)
	{
		if (offset == length)
		{
			texture.resize(1);
			texture[0] = 0xFFFF;
			return offset;
		}

		if (textureFlags == 0x07)
		{
			return loadTextureUncompressed(data, length, offset);
		}
		else
		{
			return loadTextureCompressed(data, length, offset);
		}
	}

	size_t Icon::loadTextureUncompressed(const uint8_t* data, size_t length, size_t offset)
	{
		if (length < offset + TEXTURE_SIZE)
		{
			throw std::runtime_error("Icon file too small for uncompressed texture");
		}

		texture.resize(TEXTURE_WIDTH * TEXTURE_HEIGHT);

		// Copy 16 bit texture data
		for (int i = 0; i < TEXTURE_WIDTH * TEXTURE_HEIGHT; ++i)
		{
			texture[i] = readLE<uint16_t>(data, offset + i * 2);
		}

		return offset + TEXTURE_SIZE;
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
