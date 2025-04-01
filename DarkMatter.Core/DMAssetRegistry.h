#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>

#include "DM3DEnums.h"
#include "DM3DTypes.h"
#include "json.hpp"

namespace dm::core
{
	enum class AssetType
	{
		Texture,
		Mesh
	};

	class Asset
	{
		friend class AssetRegistry;
	public:
		Asset() = default;

		Asset(const uint32_t id, const AssetType type, const std::string& path, size_t size)
		{
			_id = id;
			_path = path;
			_type = type;
			_size = size;
		}

		void SetId(uint32_t id) { _id = id; }
		[[nodiscard]] uint32_t GetId() const { return _id; }
		[[nodiscard]] std::string GetPath() const { return _path; }
		[[nodiscard]] AssetType GetType() const { return _type; }
		[[nodiscard]] size_t GetSize() const { return _size; }

	private:
		uint32_t _id;
		std::string _path;
		AssetType _type;
		size_t _size;
	};

	class TextureAsset : public Asset
	{
		friend class AssetRegistry;
	public:
		TextureAsset() = default;

		TextureAsset(const uint32_t id, const AssetType type, const std::string& path, size_t size, dm3d::ImageFormat format, dm3d::Extent3D extent, size_t mipLevels) : Asset(id, type, path, size)
		{
			_format = format;
			_extent = extent;
			_mipLevels = mipLevels;
		}

		[[nodiscard]] dm3d::ImageFormat GetFormat() const { return _format; }
		[[nodiscard]] dm3d::Extent3D GetExtent() const { return _extent; }
		[[nodiscard]] size_t GetMipLevels() const { return _mipLevels; }

	private:
		size_t _mipLevels;
		dm3d::ImageFormat _format;
		dm3d::Extent3D _extent;
	};

	class MeshAsset : public Asset
	{
		friend class AssetRegistry;
	public:
		MeshAsset() = default;

		MeshAsset(const uint32_t id, const AssetType type, const std::string& path, size_t size) : Asset(id, type, path, size)
		{
			
		}
	private:
	};

	class AssetRegistry
	{
	public:
		void Register(const TextureAsset& asset);
		void Register(const MeshAsset& asset);
		std::string SerializeRegistry();
		void DeserializeRegistry(const std::string& json);
		uint32_t Allocate();
		TextureAsset GetTexture(uint32_t id);
		MeshAsset GetMesh(uint32_t id);
		const std::unordered_map<uint32_t, TextureAsset>& GetTextures();
		const std::unordered_map<uint32_t, MeshAsset>& GetMeshes();

	private:
		void ParseAsset(Asset& asset, const nlohmann::json& json);
		void ParseTexture(TextureAsset& asset, const nlohmann::json& json);
		void ParseMesh(MeshAsset& asset, const nlohmann::json& json);

		std::atomic<uint32_t> _idCounter = 0;
		std::unordered_map<uint32_t, TextureAsset> _textureAssets;
		std::unordered_map<uint32_t, MeshAsset> _meshAssets;
		std::mutex _lock;
	};
}
