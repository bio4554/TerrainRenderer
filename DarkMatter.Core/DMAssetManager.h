#pragma once
#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include "DMAssetRegistry.h"
#include "DMFileSystem.h"
#include "DMLogger.h"
#include "DMMeshRenderable.h"

namespace dm::core
{
	class AssetManager
	{
	public:
		void SetRegistry(AssetRegistry* pRegistry);
		void SetFileSystem(FileSystem* pFileSystem);
		void SetGpuContext(dm3d::Context* pContext);
		MeshRenderable* TryGetMesh(uint32_t assetId, bool queueLoad);
		std::shared_ptr<dm3d::Image> TryGetImage(uint32_t imageId, bool queueLoad);
		void TryUnloadMesh(uint32_t assetId);
		void TryUnloadImage(uint32_t imageId);

	private:
		bool IsTextureLoading(uint32_t id);
		bool IsMeshLoading(uint32_t id);
		bool ExchangeTextureLoading(uint32_t id, bool loading);
		bool ExchangeMeshLoading(uint32_t id, bool loading);
		std::shared_ptr<MeshRenderable> ParseAndLoadMeshToGPU(char* pData, size_t size);
		std::shared_ptr<dm3d::Image> LoadImageToGPU(char* pData, size_t size, dm3d::ImageFormat format, dm3d::Extent3D extent) const;

		LoggerContext _log = LoggerContext("AssetManager");
		std::unordered_map<uint32_t, std::shared_ptr<MeshRenderable>> _meshStore;
		std::unordered_map<uint32_t, std::shared_ptr<dm3d::Image>> _textureStore;
		std::unordered_map<uint32_t, bool> _meshLoads;
		std::unordered_map<uint32_t, bool> _textureLoads;
		AssetRegistry* _currentRegistry = nullptr;
		FileSystem* _fileSystem = nullptr;
		dm3d::Context* _gpuContext = nullptr;
		std::shared_mutex _rwLockMesh, _rwLockTexture;
		std::mutex _loadLockMesh, _loadLockTexture;
	};
}
