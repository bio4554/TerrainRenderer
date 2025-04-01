#include "pch.h"
#include "DMAssetRegistry.h"

#include <format>

#include "json.hpp"

namespace dm::core
{
	void AssetRegistry::Register(const TextureAsset& asset)
	{
		std::unique_lock lock(_lock);

		if (_textureAssets.contains(asset.GetId()))
		{
			throw std::runtime_error(std::format("Can't register duplicate asset. ID: {}", asset.GetId()));
		}

		_textureAssets[asset.GetId()] = asset;
	}

	void AssetRegistry::Register(const MeshAsset& asset)
	{
		std::unique_lock lock(_lock);

		if (_meshAssets.contains(asset.GetId()))
		{
			throw std::runtime_error(std::format("Can't register duplicate asset. ID: {}", asset.GetId()));
		}

		_meshAssets[asset.GetId()] = asset;
	}

	std::string AssetRegistry::SerializeRegistry()
	{
		nlohmann::json j;
		std::vector<nlohmann::json> textureAssets;
		for (const auto& val : _textureAssets | std::views::values)
		{
			nlohmann::json asset;
			asset["id"] = val.GetId();
			asset["path"] = val.GetPath();
			asset["type"] = val.GetType();
			asset["size"] = val.GetSize();
			asset["format"] = val.GetFormat();
			auto extent = val.GetExtent();
			asset["extentWidth"] = extent.width;
			asset["extentHeight"] = extent.height;
			asset["extentDepth"] = extent.depth;
			asset["mipLevels"] = val.GetMipLevels();
			textureAssets.push_back(asset);
		}

		std::vector<nlohmann::json> meshAssets;
		for (const auto& val : _meshAssets | std::views::values)
		{
			nlohmann::json asset;
			asset["id"] = val.GetId();
			asset["path"] = val.GetPath();
			asset["type"] = val.GetType();
			asset["size"] = val.GetSize();
			meshAssets.push_back(asset);
		}

		j["textureAssets"] = textureAssets;
		j["meshAssets"] = meshAssets;

		return j.dump(4);
	}

	void AssetRegistry::ParseAsset(Asset& asset, const nlohmann::json& json)
	{
		asset._id = json["id"];
		asset._path = json["path"];
		asset._type = json["type"];
		asset._size = json["size"];
	}

	void AssetRegistry::ParseMesh(MeshAsset& asset, const nlohmann::json& json)
	{
		// todo?
	}

	void AssetRegistry::ParseTexture(TextureAsset& asset, const nlohmann::json& json)
	{
		asset._mipLevels = json["mipLevels"];
		uint32_t width = json["extentWidth"];
		uint32_t height = json["extentHeight"];
		uint32_t depth = json["extentDepth"];
		asset._extent = { .width = width, .height = height, .depth = depth };
		asset._format = json["format"];
	}


	void AssetRegistry::DeserializeRegistry(const std::string& json)
	{
		nlohmann::json j = nlohmann::json::parse(json);

		_textureAssets.clear();
		_meshAssets.clear();

		auto& jsonTextureAssets = j["textureAssets"];

		for (const auto& texture : jsonTextureAssets)
		{
			TextureAsset asset;
			ParseAsset(asset, texture);
			ParseTexture(asset, texture);
			_textureAssets[asset.GetId()] = asset;
		}

		std::vector<nlohmann::json> jsonMeshAssets = j["meshAssets"];

		for (const auto& mesh : jsonMeshAssets)
		{
			MeshAsset asset;
			ParseAsset(asset, mesh);
			ParseMesh(asset, mesh);
			_meshAssets[asset.GetId()] = asset;
		}

		auto textureIdRange = _textureAssets | std::views::values | std::views::transform([](const Asset& asset) {return asset.GetId(); });
		auto meshIdRange = _meshAssets | std::views::values | std::views::transform([](const Asset& asset) {return asset.GetId(); });

		uint32_t maxTexId = textureIdRange.empty() ? 0 : std::ranges::max(textureIdRange);
		uint32_t maxMeshId = meshIdRange.empty() ? 0 : std::ranges::max(meshIdRange);
		auto maxId = std::max(maxTexId, maxMeshId);

		_idCounter = maxId + 1;
	}

	uint32_t AssetRegistry::Allocate()
	{
		const auto val = _idCounter++;

		return val;
	}

	TextureAsset AssetRegistry::GetTexture(uint32_t id)
	{
		return _textureAssets[id];
	}

	MeshAsset AssetRegistry::GetMesh(uint32_t id)
	{
		return _meshAssets[id];
	}

	const std::unordered_map<uint32_t, TextureAsset>& AssetRegistry::GetTextures()
	{
		return _textureAssets;
	}

	const std::unordered_map<uint32_t, MeshAsset>& AssetRegistry::GetMeshes()
	{
		return _meshAssets;
	}

}
