#pragma once

#include <stack>

#include "DMEngine.h"
#include "DMTwoThreadSync.h"

namespace dme
{
	struct EditTransaction
	{
		std::function<void()> commit;
		std::function<void()> rollback;
	};

	struct TextureAssetMeta
	{
		uint32_t id;
		std::string name;
		std::string extent;
	};

	struct CellMeta
	{
		uint32_t id;
		std::string name;
		std::string position;
	};
}

class CEngineManager
{
public:
	dm::Engine* Engine() const { return _engine.get(); }
	void Init(HWND hWnd);
	void Shutdown();
	void SubmitTransaction(dme::EditTransaction transaction);
	void LockEngine();
	void UnlockEngine();
    void Resize(int newWidth, int newHeight);
	std::vector<dme::TextureAssetMeta> GetTextureAssets() const;
	std::vector<dme::CellMeta> GetCellData() const;
	void ImportTexture(std::string path, std::string name);
private:
	std::stack<dme::EditTransaction> _transactionStack;
    std::unique_ptr<dm::Engine> _engine;
    std::thread _engineThread;
	dm::core::TwoThreadSync _engineSync;
    bool _doQuit = false;
    void InitNewWorld();
};

extern CEngineManager* GlobalEngineManager;
extern dm::Engine* GlobalEngine;