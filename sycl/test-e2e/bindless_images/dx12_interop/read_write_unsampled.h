
#pragma once

#include <iostream>
#include <string>

#include "../helpers/common.hpp"
#include "../helpers/dx_interop_common.hpp"

#include <sycl/ext/oneapi/bindless_images.hpp>

#include <sycl/properties/queue_properties.hpp>

using namespace dx_helpers;
using namespace sycl_dx_img_utils;

namespace syclexp = sycl::ext::oneapi::experimental;

//-==========================================================================-//
class DX12SYCLDevice {
public:
  DX12SYCLDevice();

  DX12SYCLDevice(const DX12SYCLDevice &) = delete;
  DX12SYCLDevice &operator=(const DX12SYCLDevice &) = delete;

private:
  void _initDevice();
  void _initCmdList();

public:
  ID3D12Device              *getDevice  () { return _dev.Get(); }
  ID3D12CommandQueue        *getCmdQueue() { return _cmdQueue.Get(); }
  ID3D12GraphicsCommandList *getCmdList () { return _cmdList.Get(); }

  sycl::queue &getSyclQueue() { return _syclQueue; }

  HRESULT resetCmdList() { return _cmdList->Reset(_cmdAlloc.Get(), nullptr); }

private:
  // DX12 Objects
  ComPtr<IDXGIFactory4> _factory;
  ComPtr<IDXGIAdapter1> _adapter;
  ComPtr<ID3D12Device3> _dev;
  
  ComPtr<ID3D12CommandQueue>        _cmdQueue;
  ComPtr<ID3D12GraphicsCommandList> _cmdList;
  ComPtr<ID3D12CommandAllocator>    _cmdAlloc;

  // SYCL Objects
  sycl::queue  _syclQueue;
  sycl::device _syclDev;
};
//-==========================================================================-//

//-==========================================================================-//
template <uint32_t NDims, typename DType, uint32_t NChannels>
class DX12Interop {

  using VecType = sycl::vec<DType, NChannels>;
  
  static constexpr uint32_t PixSize = VecType::byte_size();

  static_assert(NDims >= 1 && NDims <= 3, "NDims must be 1, 2, or 3.");
  static_assert(PixSize == sizeof(DType) * NChannels);

public:
  DX12Interop(DX12SYCLDevice &device, sycl::image_channel_type channelType,
              Dims3D<NDims> imgDims, Dims3D<NDims> grpDims);

  ~DX12Interop() {}

  void init();
  void cleanupDX12();

  void callSYCLKernel();

  bool validateOutput();

private:
  void waitFence(DWORD msTimeout = INFINITE);
  void populateDX12Texture();
  void importSharedMemHandle(size_t allocSize);
  void importSharedSemaphore();

  // Dimensions of image
  std::vector<DType> m_srcData;

  Dims3D<NDims> _imgDims{};
  Dims3D<NDims> _grpDims{};
  uint64_t _numPixs{1};
  uint64_t _numElems{NChannels};
  uint64_t _dataSize{sizeof(DType) * NChannels};

  sycl::image_channel_type _elemType;

  // sycl::range<NDims> m_dataDims;
  // sycl::range<NDims> m_localSize;

  DX12SYCLDevice &_device;

  // DX12 Objects
  ComPtr<ID3D12Resource> _texture;
  ComPtr<ID3D12Fence>    _fence;
  HANDLE                 _fenceEvent;
  std::atomic<uint64_t>  _fenceVal{0};

  // Shared handles and values
  HANDLE _memHandle{INVALID_HANDLE_VALUE};
  HANDLE _semaphore{INVALID_HANDLE_VALUE};

  // SYCL Objects
  syclexp::external_mem           _syclMemHandle;
  syclexp::external_semaphore     _syclSemaphore;
  syclexp::image_mem_handle       _syclImgMem;
  syclexp::unsampled_image_handle _syclImgHandle;
};
//-==========================================================================-//
