#pragma once
#include <afxwin.h>

#include "CEngineManager.h"
#include "DMCell.h"
#include "resource.h"

class CCellSettingsDialog : public CDialogEx
{
public:
	CCellSettingsDialog(uint32_t cellId, CWnd* pParent = nullptr);

	enum { IDD = IDD_DIALOG_CELL_SETTINGS };

protected:
	virtual void OnOK() override;
	BOOL OnInitDialog() override;
	virtual void DoDataExchange(CDataExchange* pDX) override;
	virtual void PostNcDestroy() override;
	DECLARE_MESSAGE_MAP()

protected:
	dm::model::Cell* _cell;
public:
	CComboBox _comboTex1;
	CComboBox _comboTex2;
	CComboBox _comboTex3;
	CComboBox _comboTex4;

private:
	int LocateTextureIdx(std::vector<dme::TextureAssetMeta>& textures, uint32_t textureId);
	uint32_t GetBoxDataU32(CComboBox& box);
};
