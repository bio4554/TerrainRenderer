#pragma once
#include <exception>

namespace dm3d
{
	inline void check_result(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw std::exception();
		}
	}

	inline bool check_result_safe(HRESULT hr)
	{
		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}

	// helper methods taken from DXTK

	//------------------------------------------------------------------------------------------------
	// Returns required size of a buffer to be used for data upload
	inline UINT64 get_required_intermediate_size(
		_In_ ID3D12Resource* pDestinationResource,
		_In_range_(0, D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
		_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) UINT NumSubresources) noexcept
	{
#if defined(_MSC_VER) || !defined(_WIN32)
		const auto Desc = pDestinationResource->GetDesc();
#else
		D3D12_RESOURCE_DESC tmpDesc;
		const auto& Desc = *pDestinationResource->GetDesc(&tmpDesc);
#endif
		UINT64 RequiredSize = 0;

		ID3D12Device* pDevice = nullptr;
		pDestinationResource->GetDevice(IID_ID3D12Device, reinterpret_cast<void**>(&pDevice));
		pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, 0, nullptr, nullptr, nullptr, &RequiredSize);
		pDevice->Release();

		return RequiredSize;
	}

	//------------------------------------------------------------------------------------------------
	// Row-by-row memcpy
	inline void memcpy_subresource(
		_In_ const D3D12_MEMCPY_DEST* pDest,
		_In_ const D3D12_SUBRESOURCE_DATA* pSrc,
		SIZE_T RowSizeInBytes,
		UINT NumRows,
		UINT NumSlices) noexcept
	{
		for (UINT z = 0; z < NumSlices; ++z)
		{
			auto pDestSlice = static_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
			auto pSrcSlice = static_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * LONG_PTR(z);
			for (UINT y = 0; y < NumRows; ++y)
			{
				memcpy(pDestSlice + pDest->RowPitch * y,
					pSrcSlice + pSrc->RowPitch * LONG_PTR(y),
					RowSizeInBytes);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// All arrays must be populated (e.g. by calling GetCopyableFootprints)
	inline UINT64 update_subresources(
		_In_ ID3D12GraphicsCommandList* pCmdList,
		_In_ ID3D12Resource* pDestinationResource,
		_In_ ID3D12Resource* pIntermediate,
		_In_range_(0, D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
		_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) UINT NumSubresources,
		UINT64 RequiredSize,
		_In_reads_(NumSubresources) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		_In_reads_(NumSubresources) const UINT* pNumRows,
		_In_reads_(NumSubresources) const UINT64* pRowSizesInBytes,
		_In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData) noexcept
	{
		// Minor validation
#if defined(_MSC_VER) || !defined(_WIN32)
		const auto IntermediateDesc = pIntermediate->GetDesc();
		const auto DestinationDesc = pDestinationResource->GetDesc();
#else
		D3D12_RESOURCE_DESC tmpDesc1, tmpDesc2;
		const auto& IntermediateDesc = *pIntermediate->GetDesc(&tmpDesc1);
		const auto& DestinationDesc = *pDestinationResource->GetDesc(&tmpDesc2);
#endif
		if (IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
			IntermediateDesc.Width < RequiredSize + pLayouts[0].Offset ||
			RequiredSize > SIZE_T(-1) ||
			(DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
				(FirstSubresource != 0 || NumSubresources != 1)))
		{
			return 0;
		}

		BYTE* pData;
		HRESULT hr = pIntermediate->Map(0, nullptr, reinterpret_cast<void**>(&pData));
		if (FAILED(hr))
		{
			return 0;
		}

		for (UINT i = 0; i < NumSubresources; ++i)
		{
			if (pRowSizesInBytes[i] > SIZE_T(-1)) return 0;
			D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, SIZE_T(pLayouts[i].Footprint.RowPitch) * SIZE_T(pNumRows[i]) };
			memcpy_subresource(&DestData, &pSrcData[i], static_cast<SIZE_T>(pRowSizesInBytes[i]), pNumRows[i], pLayouts[i].Footprint.Depth);
		}
		pIntermediate->Unmap(0, nullptr);

		if (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			pCmdList->CopyBufferRegion(
				pDestinationResource, 0, pIntermediate, pLayouts[0].Offset, pLayouts[0].Footprint.Width);
		}
		else
		{
			for (UINT i = 0; i < NumSubresources; ++i)
			{
				D3D12_TEXTURE_COPY_LOCATION Dst = {};
				Dst.pResource = pDestinationResource;
				Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				Dst.PlacedFootprint = {};
				Dst.SubresourceIndex = i + FirstSubresource;
				D3D12_TEXTURE_COPY_LOCATION Src = {};
				Src.pResource = pIntermediate;
				Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				Src.PlacedFootprint = pLayouts[i];

				pCmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
			}
		}
		return RequiredSize;
	}

	//------------------------------------------------------------------------------------------------
	// Heap-allocating UpdateSubresources implementation
	inline UINT64 update_subresources(
		_In_ ID3D12GraphicsCommandList* pCmdList,
		_In_ ID3D12Resource* pDestinationResource,
		_In_ ID3D12Resource* pIntermediate,
		UINT64 IntermediateOffset,
		_In_range_(0, D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
		_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) UINT NumSubresources,
		_In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData) noexcept
	{
		UINT64 RequiredSize = 0;
		const auto MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * NumSubresources;
		if (MemToAlloc > SIZE_MAX)
		{
			return 0;
		}
		void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
		if (pMem == nullptr)
		{
			return 0;
		}
		auto pLayouts = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
		auto pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + NumSubresources);
		auto pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + NumSubresources);

#if defined(_MSC_VER) || !defined(_WIN32)
		const auto Desc = pDestinationResource->GetDesc();
#else
		D3D12_RESOURCE_DESC tmpDesc;
		const auto& Desc = *pDestinationResource->GetDesc(&tmpDesc);
#endif
		ID3D12Device* pDevice = nullptr;
		pDestinationResource->GetDevice(IID_ID3D12Device, reinterpret_cast<void**>(&pDevice));
		pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);
		pDevice->Release();

		const UINT64 Result = update_subresources(pCmdList, pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, pLayouts, pNumRows, pRowSizesInBytes, pSrcData);
		HeapFree(GetProcessHeap(), 0, pMem);
		return Result;
	}
}
