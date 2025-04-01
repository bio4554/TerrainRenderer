#include "pch.h"
#include "DM3DResource.h"

#include "DM3DContext.h"

namespace dm3d
{
	Resource::Resource(D3D12MA::Allocation* alloc, ID3D12Resource* resource, Context* context, const D3D12_RESOURCE_DESC& resourceDesc, const std::string& name)
	{
		_allocation = alloc;
		_d3d12Resource = resource;
		_resourceDesc = resourceDesc;
		_currentState = D3D12_RESOURCE_STATE_COMMON;
		_context = context;
		_name = name;

		if (context != nullptr)
		{
			context->alloc_resource_internal();
		}
	}

	Resource::~Resource()
	{
		if (_context != nullptr)
		{
			_context->release_resource_internal(this);
		}
	}

}
