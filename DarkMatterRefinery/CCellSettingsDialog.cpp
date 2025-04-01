#include "pch.h"
#include "CCellSettingsDialog.h"

#include "CEngineManager.h"
#include "DMEUtilities.h"

BEGIN_MESSAGE_MAP(CCellSettingsDialog, CDialogEx)

END_MESSAGE_MAP()

CCellSettingsDialog::CCellSettingsDialog(uint32_t cellId, CWnd* pParent) : CDialogEx(IDD_DIALOG_CELL_SETTINGS, pParent)
{
	auto world = GlobalEngineManager->Engine()->GetWorld();

	if (world->cellStore.size() <= cellId)
	{
		throw std::runtime_error("Dialog: Invalid CellID");
	}

	_cell = &world->cellStore[cellId];
}

void CCellSettingsDialog::OnOK()
{
	UpdateData(TRUE);

	uint32_t newTex1Idx = GetBoxDataU32(_comboTex1);
	uint32_t newTex2Idx = GetBoxDataU32(_comboTex2);
	uint32_t newTex3Idx = GetBoxDataU32(_comboTex3);
	uint32_t newTex4Idx = GetBoxDataU32(_comboTex4);

	uint32_t oldTex1Idx = _cell->GetTerrainTexture(0);
	uint32_t oldTex2Idx = _cell->GetTerrainTexture(1);
	uint32_t oldTex3Idx = _cell->GetTerrainTexture(2);
	uint32_t oldTex4Idx = _cell->GetTerrainTexture(3);

	dme::EditTransaction transaction;

	auto pCell = _cell;

	transaction.commit = [pCell, newTex1Idx, newTex2Idx, newTex3Idx, newTex4Idx]()
		{
			pCell->SetTerrainTexture(0, newTex1Idx);
			pCell->SetTerrainTexture(1, newTex2Idx);
			pCell->SetTerrainTexture(2, newTex3Idx);
			pCell->SetTerrainTexture(3, newTex4Idx);
		};

	transaction.rollback = [pCell, oldTex1Idx, oldTex2Idx, oldTex3Idx, oldTex4Idx]()
		{
			pCell->SetTerrainTexture(0, oldTex1Idx);
			pCell->SetTerrainTexture(1, oldTex2Idx);
			pCell->SetTerrainTexture(2, oldTex3Idx);
			pCell->SetTerrainTexture(3, oldTex4Idx);
		};

	GlobalEngineManager->SubmitTransaction(transaction);

	DestroyWindow();
}

BOOL CCellSettingsDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	auto textures = GlobalEngineManager->GetTextureAssets();

	

	int idx = 0;
	for (auto& texture : textures)
	{
		_comboTex1.AddString(dme::ToCString(texture.name));
		_comboTex1.SetItemData(idx, texture.id);
		_comboTex2.AddString(dme::ToCString(texture.name));
		_comboTex2.SetItemData(idx, texture.id);
		_comboTex3.AddString(dme::ToCString(texture.name));
		_comboTex3.SetItemData(idx, texture.id);
		_comboTex4.AddString(dme::ToCString(texture.name));
		_comboTex4.SetItemData(idx, texture.id);
		idx++;
	}

	_comboTex1.SetCurSel(LocateTextureIdx(textures, _cell->GetTerrainTexture(0)));
	_comboTex2.SetCurSel(LocateTextureIdx(textures, _cell->GetTerrainTexture(1)));
	_comboTex3.SetCurSel(LocateTextureIdx(textures, _cell->GetTerrainTexture(2)));
	_comboTex4.SetCurSel(LocateTextureIdx(textures, _cell->GetTerrainTexture(3)));

	return TRUE;
}

void CCellSettingsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO_CELL_TEX1, _comboTex1);
	DDX_Control(pDX, IDC_COMBO_CELL_TEX2, _comboTex2);
	DDX_Control(pDX, IDC_COMBO_CELL_TEX3, _comboTex3);
	DDX_Control(pDX, IDC_COMBO_CELL_TEX4, _comboTex4);
}

void CCellSettingsDialog::PostNcDestroy()
{
	CDialogEx::PostNcDestroy();
	delete this;
}

int CCellSettingsDialog::LocateTextureIdx(std::vector<dme::TextureAssetMeta>& textures, uint32_t textureId)
{
	for (size_t i = 0; i < textures.size(); i++)
	{
		if (textures[i].id == textureId)
			return static_cast<int>(i);
	}

	return -1;
}

uint32_t CCellSettingsDialog::GetBoxDataU32(CComboBox& box)
{
	auto sel = box.GetCurSel();
	auto data = box.GetItemData(sel);
	return static_cast<uint32_t>(data);
}

