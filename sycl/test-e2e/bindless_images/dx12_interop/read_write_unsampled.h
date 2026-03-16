
#pragma once

#include <iostream>
#include <string>

#include "../helpers/common.hpp"
#include "../helpers/dx_interop_common.hpp"

#include <sycl/ext/oneapi/bindless_images.hpp>

#include <sycl/properties/queue_properties.hpp>

using namespace dx_helpers;

namespace syclexp = sycl::ext::oneapi::experimental;

using DXFactory  = IDXGIFactory4;
using DXAdapter  = IDXGIAdapter1;
using DXDevice   = ID3D12Device;
using DXCmdQueue = ID3D12CommandQueue;
using DXCmdList  = ID3D12GraphicsCommandList;
using DXCmdAlloc = ID3D12CommandAllocator;
using DXResource = ID3D12Resource;
using DXFence    = ID3D12Fence;

using DXFactoryPtr  = ComPtr<DXFactory>;
using DXAdapterPtr  = ComPtr<DXAdapter>;
using DXDevicePtr   = ComPtr<DXDevice>;
using DXCmdQueuePtr = ComPtr<DXCmdQueue>;
using DXCmdListPtr  = ComPtr<DXCmdList>;
using DXCmdAllocPtr = ComPtr<DXCmdAlloc>;
using DXResourcePtr = ComPtr<DXResource>;
using DXFencePtr    = ComPtr<DXFence>;

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
  DXDevice    *getDxDevice  () { return _device.Get(); }
  DXCmdQueue  *getDxCmdQueue() { return _dxQueue.Get(); }
  DXCmdList   *getCmdList   () { return _cmdList.Get(); }
  sycl::queue &getSyclQueue () { return _syclQueue; }

  HRESULT resetCmdList() {
    return _cmdList->Reset(_cmdAlloc.Get(), nullptr);
  }

private:
  // DX12 Objects
  DXFactoryPtr  _factory;
  DXAdapterPtr  _adapter;
  DXDevicePtr   _device;

  DXCmdQueuePtr _dxQueue;
  DXCmdListPtr  _cmdList;
  DXCmdAllocPtr _cmdAlloc;

  // SYCL Objects
  sycl::queue  _syclQueue;
  sycl::device _syclDev;
};
//-==========================================================================-//

//-==========================================================================-//
template <uint32_t NDims, typename DType, uint32_t NChannels>
class DX12Interop {
public:
  DX12Interop(DX12SYCLDevice &device, sycl_channel_t channelType,
              Dims3D<NDims> dataDims, Dims3D<NDims> groupDims);

  ~DX12Interop() {}

  void initDX12Resources();
  void cleanupDX12();

  void callSYCLKernel();

  bool validateOutput();

private:
  void waitDxFence(DWORD msecTimeout = INFINITE);
  void populateDX12Texture();
  void importMemHandle(size_t allocSize);
  void importSemaphore();

  // Dimensions of image
  uint32_t _wdth;
  uint32_t _hght;
  uint32_t _dpth;
  size_t   _numElems;

  std::vector<DType> _srcData;

  sycl_channel_t _elemType;

  // sycl::range<NDims> _dataDims;
  // sycl::range<NDims> _groupDims;
  Dims3D<NDims> _dataDims;
  Dims3D<NDims> _groupDims;

  DX12SYCLDevice &_device;

  // DX12 Objects
  DXResourcePtr _texture;
  DXFencePtr    _fence;
  HANDLE        _fenceEvent;
  uint64_t      _fenceVal = 0;

  // Shared handles and values
  HANDLE _memHandle = INVALID_HANDLE_VALUE;
  HANDLE _semaphore = INVALID_HANDLE_VALUE;

  // SYCL Objects
  syclexp::external_mem _syclMemHandle;
  syclexp::external_semaphore _syclSemaphore;
  syclexp::image_mem_handle _syclImgMem;
  syclexp::unsampled_image_handle _syclImgHandle;
};
//-==========================================================================-//
