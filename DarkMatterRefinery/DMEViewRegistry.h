#pragma once

#include <afxwin.h>

#include "CAssetView.h"
#include "CEngineView.h"
#include "CWorldView.h"

#define ADD_VIEW(className, propName) \
public: \
	className* Get##propName() const { return _##propName; }\
	void Set##propName(className* v) {\
	if(_##propName != nullptr) { \
		throw std::runtime_error("Tried to set view, view was not nullptr!");\
	}\
	_##propName = v;\
	}\
	private:\
className* _##propName = nullptr;\





class DMEViewRegistry
{
	ADD_VIEW(CEngineView, EngineView)
	ADD_VIEW(CAssetView, AssetView)
	ADD_VIEW(CWorldView, WorldView)
};

extern DMEViewRegistry GViews;