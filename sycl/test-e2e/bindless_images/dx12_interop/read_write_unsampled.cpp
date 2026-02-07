// REQUIRES: aspect-ext_oneapi_external_memory_import
// REQUIRES: windows

// UNSUPPORTED: gpu-intel-gen12
// UNSUPPORTED-INTENDED: Unknown issue with integrated GPU failing
//                       when importing memory

// XFAIL: windows && arch-intel_gpu_bmg_g21
// XFAIL-TRACKER: https://github.com/intel/llvm/issues/20384

// RUN: %{build} %link-directx -o %t.out
// RUN: %{run-unfiltered-devices} env NEOReadDebugKeys=1 UseBindlessMode=1 UseExternalAllocatorForSshAndDsh=1 %t.out

#pragma clang diagnostic ignored "-Waddress-of-temporary"

#include "read_write_unsampled.h"

#include <sycl/sycl.hpp>

#include <iostream> // for std::cout
#include <sstream>  // for std::ostringstream
#include <iomanip>  // for std::setw

#define DEBUG_SAMPLED_IMG 1
#ifdef VERBOSE_PRINT
#define VERBOSE_DEBUG DEBUG_SAMPLED_IMG
#else
#define VERBOSE_DEBUG 0
#endif

using namespace sycl_dx_img_utils;

//----------------------------------------------------------------------------//
void printString(std::string str) {
#ifdef VERBOSE_PRINT
  std::cout << str << std::endl;
#endif
}
//----------------------------------------------------------------------------//

//-==========================================================================-//
DX12SYCLDevice::DX12SYCLDevice()
              :_syclQueue{{sycl::property::queue::in_order{}}},
               _syclDev{_syclQueue.get_device()} {
  std::cerr << "ENTER: DX12SYCLDevice::DX12SYCLDevice()\n";
  _initDevice();
  _initCmdList();
  std::cerr << "LEAVE: DX12SYCLDevice::DX12SYCLDevice()\n";
}
//----------------------------------------------------------------------------//
void DX12SYCLDevice::_initDevice() {
  std::cerr << "ENTER: DX12SYCLDevice::_initDevice()\n";
  // Create DXGI factory.
  ThrowIfFailed(CreateDXGIFactory2(0 /* dxgiFactoryFlags */, IID_PPV_ARGS(&_factory)));

  // Get the hardware adapter for a suitable GPU.
  _adapter = getDXGIHardwareAdapter<dx_version::DX12>(_factory.Get(),
                                         _syclDev.get_info<sycl::info::device::name>());

  // Create a device from our hardware adapter.
  ThrowIfFailed(D3D12CreateDevice(_adapter.Get(),
                                  D3D_FEATURE_LEVEL_12_0,
                                  IID_PPV_ARGS(&_dev)));
  std::cerr << "LEAVE: DX12SYCLDevice::_initDevice()\n";
}
//----------------------------------------------------------------------------//
void DX12SYCLDevice::_initCmdList() {
  std::cerr << "ENTER: DX12SYCLDevice::_initCmdList()\n";
  // Describe and create the command queue.
  D3D12_COMMAND_QUEUE_DESC queueDesc = {D3D12_COMMAND_LIST_TYPE_DIRECT, 0,
                                        D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
  ThrowIfFailed(_dev->CreateCommandQueue(
      &queueDesc, IID_PPV_ARGS(&_cmdQueue)));

  // Create the command allocator.
  ThrowIfFailed(_dev->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAlloc)));

  // Create the command list.
  ThrowIfFailed(_dev->CreateCommandList(
      0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAlloc.Get(), NULL,
      IID_PPV_ARGS(&_cmdList)));
  std::cerr << "LEAVE: DX12SYCLDevice::_initCmdList()\n";
}
//-==========================================================================-//

//-==========================================================================-//
template <uint32_t NDims, typename DType, uint32_t NChannels>
DX12Interop<NDims, DType, NChannels>::DX12Interop(
    DX12SYCLDevice &device, sycl::image_channel_type channelType,
    Dims3D<NDims> imgDims, Dims3D<NDims> grpDims)
    :_device(device),_elemType(channelType),_imgDims(imgDims),_grpDims(grpDims) {
  _numPixs  =_imgDims.size();
  _numElems =_numPixs  * NChannels;
  _dataSize =_numElems * sizeof(DType);
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
void DX12Interop<NDims, DType, NChannels>::init() {

  // Define default heap properties.
  D3D12_HEAP_PROPERTIES defltHeapProps = {};
  defltHeapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
  defltHeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  defltHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  defltHeapProps.CreationNodeMask     = 1;
  defltHeapProps.VisibleNodeMask      = 1;

  // Define texture resource descriptor.
  D3D12_RESOURCE_DESC texDesc = {};
  if      constexpr (NDims == 1) texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
  else if constexpr (NDims == 2) texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  else                           texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
  texDesc.Alignment        = 0;
  texDesc.Width            = _imgDims.wdth;
  texDesc.Height           = _imgDims.hght;
  texDesc.DepthOrArraySize = _imgDims.dpth;
  texDesc.MipLevels        = 0;
  texDesc.Format           = toDXGIFormat<NChannels>(_elemType);
  texDesc.SampleDesc       = DXGI_SAMPLE_DESC{1, 0};
  texDesc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  texDesc.Flags            = D3D12_RESOURCE_FLAG_NONE;

  // Create the DX12 texture.
  auto *dev =_device.getDevice();
  ThrowIfFailed(dev->CreateCommittedResource(
      &defltHeapProps, D3D12_HEAP_FLAG_SHARED, &texDesc,
      D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&_texture)));

  // Create a shared handle for our texture.
  ThrowIfFailed(dev->CreateSharedHandle(_texture.Get(), nullptr,
                                               GENERIC_ALL, nullptr,
                                               &_memHandle));

  D3D12_RESOURCE_ALLOCATION_INFO texAllocInfo;
  texAllocInfo = dev->GetResourceAllocationInfo(1, 1, &texDesc);

  // Import the shared DX12 texture resource to SYCL.
  importSharedMemHandle(texAllocInfo.SizeInBytes);

  // Create the DX12 fence and map to a SYCL semaphore.
  ThrowIfFailed(dev->CreateFence(
      _fenceVal, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&_fence)));
  _fenceVal++;

#ifdef TEST_SEMAPHORE_IMPORT
  ThrowIfFailed(dev->CreateSharedHandle(_fence.Get(), nullptr,
                                               GENERIC_ALL, nullptr,
                                               &_semaphore));

  // Import our shared DX12 fence resource to SYCL.
  importSharedSemaphore();
#endif // ifdef TEST_SEMAPHORE_IMPORT

  // Create an event handle to use for synchronization.
  _fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (_fenceEvent == nullptr) {
    ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
  }

  populateDX12Texture();
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
void DX12Interop<NDims, DType, NChannels>::importSharedMemHandle(size_t allocSize) {
  std::cerr << "ENTER: DX12Interop<NDims, DType, NChannels>::importSharedMemHandle(size_t " << allocSize << ")\n";
  syclexp::external_mem_descriptor<syclexp::resource_win32_handle> extMemDesc{
      _memHandle,
      syclexp::external_mem_handle_type::win32_nt_dx12_resource, allocSize};

  auto &syclQueue =_device.getSyclQueue();
  _syclMemHandle = syclexp::import_external_memory(extMemDesc, syclQueue);

  syclexp::image_descriptor imgDesc{_imgDims, NChannels, _elemType};
  _syclImgMem    = syclexp::map_external_image_memory(_syclMemHandle, imgDesc,syclQueue);
  _syclImgHandle = syclexp::create_image(_syclImgMem, imgDesc, syclQueue);
  std::cerr << "LEAVE: DX12Interop<NDims, DType, NChannels>::importSharedMemHandle(size_t)\n";
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
void DX12Interop<NDims, DType, NChannels>::importSharedSemaphore() {
  syclexp::external_semaphore_descriptor<syclexp::resource_win32_handle>
      extSemDesc{_semaphore,
                 syclexp::external_semaphore_handle_type::win32_nt_dx12_fence};

  _syclSemaphore = syclexp::import_external_semaphore(extSemDesc,_device.getSyclQueue());
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
void DX12Interop<NDims, DType, NChannels>::callSYCLKernel() {
  auto &syclQueue =_device.getSyclQueue();
#ifdef TEST_SEMAPHORE_IMPORT
  // Wait for imported semaphore. This semaphore was signalled at the
  // end of `populateDX12Texture`.
  syclQueue.ext_oneapi_wait_external_semaphore(_syclSemaphore,
                                               _fenceVal);
#endif // ifdef TEST_SEMAPHORE_IMPORT

  // We can't capture the image handle through `this` in the lambda.
  // If we do the kernel will crash.
  auto imgHandle = _syclImgHandle;

  using VecType = sycl::vec<DType, NChannels>;

  sycl::range<NDims> syclDim =_imgDims.to_flip_range();
  sycl::range<NDims> syclGrp =_grpDims.to_flip_range();
  
  printString("Submitting SYCL kernel");
#if VERBOSE_DEBUG
  std::cout << "\timage size: " <<_imgDims.wdth << "(w)";
  if constexpr (NDims >= 2) std::cout << " x " <<_imgDims.hght << "(h)";
  if constexpr (NDims == 3) std::cout << " x " <<_imgDims.dpth << "(d)";
  std::cout << " -> group size: " <<_grpDims.wdth << "(w)";
  if constexpr (NDims >= 2) std::cout << " x " <<_grpDims.hght << "(h)";
  if constexpr (NDims == 3) std::cout << " x " <<_grpDims.dpth << "(d)";
  std::cout << "\n";
  std::cout << "\t# elements: " <<_numElems << "\n";
  std::cout << "\t# channels: " << NChannels << "\n";
  std::cout << "\telem size : " << sizeof(DType) << " --> pixel size: "
            << sizeof(VecType) << "\n";
  std::cout << "\t# bytes   : " <<_dataSize << "\n";
  if (sizeof(DType) != sizeof(DType))
    std::cout << "\t***** NOTE: Output data type different from image data type *****\n";
#endif // VERBOSE_DEBUG
  // Submit our SYCL kernel. All we do is double the value of each pixel in the
  // texture.
  using samp_t = std::conditional_t<NChannels == 1, DType, VecType>;
  try {
    syclQueue.submit([&](sycl::handler &cgh) {
      sycl::stream str(786432, 48, cgh);
      cgh.parallel_for(
          sycl::nd_range<NDims>{syclDim, syclGrp},
          [=](sycl::nd_item<NDims> it) {
            if constexpr (NDims == 3) {
              size_t dim0 = it.get_global_id(0);
              size_t dim1 = it.get_global_id(1);
              size_t dim2 = it.get_global_id(2);

              auto px = syclexp::fetch_image<samp_t>(imgHandle, sycl::int3(dim0, dim1, dim2));
              px *= static_cast<DType>(2);
              syclexp::write_image(imgHandle, sycl::int3(dim0, dim1, dim2), px);
            } else if constexpr (NDims == 2) {
              size_t y = it.get_global_id(0);  // SYCL uses dimension order height,
              size_t x = it.get_global_id(1);  //   width (flipped from Vulkan)
              size_t sy = it.get_global_range(0);
              size_t sx = it.get_global_range(1);
              const char *py = (y < 10 ? "  " : (y < 100 ? " " : ""));
              const char *px = (x < 10 ? "  " : (x < 100 ? " " : ""));

              samp_t pix = syclexp::fetch_image<samp_t>(imgHandle, sycl::int2(y, x));
              size_t gli = it.get_global_linear_id();
              DType  val = static_cast<DType>(-1);
              // constexpr bool readOkay = (NChannels < 4 || sizeof(DType) < 4);
              constexpr bool readOkay = true;
              if      constexpr (NChannels == 1) val = pix;
              else if constexpr (readOkay)       val = pix[0];
              const char *pgi = (gli < 10 ? "  " : (gli < 100 ? " " : ""));
              // if (x % 4 == 0)
                str << "(" << py << y << "/" << sy << "," << px << x << "/" << sx << "):["
                    << pgi << gli << "]->" << val << sycl::endl;
              syclexp::write_image(imgHandle, sycl::int2(y, x), pix * static_cast<DType>(2));
            } else {
              size_t x = it.get_global_id(0);
              size_t sx = it.get_global_range(0);
              const char *px = (x < 10 ? "  " : (x < 100 ? " " : ""));

              samp_t pix = syclexp::fetch_image<samp_t>(imgHandle, int(x));
              size_t gli = it.get_global_linear_id();
              DType  val = static_cast<DType>(-1);
              if constexpr (NChannels == 1) val = pix;
              else                          val = pix[0];
              const char *pgi = (gli < 10 ? "  " : (gli < 100 ? " " : ""));
              str << "(" << px << x << "/" << sx << "):[" << pgi << gli << "]->" << val << sycl::endl;
              syclexp::write_image(imgHandle, int(x), pix * static_cast<DType>(2));
            }
          });
    });
  } catch (sycl::exception e) {
    std::cerr << "\tKernel submission failed! " << e.what() << std::endl;
    exit(-1);
  } catch (...) {
    std::cerr << "\tKernel submission failed!" << std::endl;
    exit(-1);
  }

#ifdef TEST_SEMAPHORE_IMPORT
  // Increment the fence value.
  _fenceVal++;

  // Signal imported semaphore.
  syclQueue.submit([&](sycl::handler &cgh) {
    cgh.ext_oneapi_signal_external_semaphore(_syclSemaphore,_fenceVal);
  });

  // Use DX12 to wait for the semaphore signalled by SYCL above.
  waitFence();
#else  // ifdef TEST_SEMAPHORE_IMPORT
  syclQueue.wait();
#endif // else TEST_SEMAPHORE_IMPORT
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
void DX12Interop<NDims, DType, NChannels>::populateDX12Texture() {

  // Set our texture data to upload.
  m_srcData.resize(_numElems);
  auto getInputValue = [&](uint64_t i) -> DType {
    if constexpr (std::is_integral_v<DType> ||
                  std::is_same_v<DType, sycl::half>)
      i = i % (static_cast<uint64_t>(std::numeric_limits<DType>::max()) / 2);
    return i;
  };
  for (uint64_t i = 0; i <_numElems; ++i) {
    m_srcData[i] = getInputValue(i);
  }

  // Get required staging buffer size.
  uint64_t stagingBufferSize = 0;
  auto *dev =_device.getDevice();
  dev->GetCopyableFootprints(&_texture->GetDesc(), 0, 1, 0, nullptr,
                                    nullptr, nullptr, &stagingBufferSize);

  // Define upload heap properties.
  D3D12_HEAP_PROPERTIES uploadHeapProperties = {};
  uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
  uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  uploadHeapProperties.CreationNodeMask = 1;
  uploadHeapProperties.VisibleNodeMask = 1;

  // Define upload buffer resource descriptor.
  D3D12_RESOURCE_DESC uploadBufferResourceDesc = {};
  uploadBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  uploadBufferResourceDesc.Alignment = 0;
  uploadBufferResourceDesc.Width = stagingBufferSize;
  uploadBufferResourceDesc.Height = 1;
  uploadBufferResourceDesc.DepthOrArraySize = 1;
  uploadBufferResourceDesc.MipLevels = 1;
  uploadBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  uploadBufferResourceDesc.SampleDesc = DXGI_SAMPLE_DESC{1, 0};
  uploadBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  uploadBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  // Allocate the staging upload buffer.
  ComPtr<ID3D12Resource> stagingBuffer;
  ThrowIfFailed(dev->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadBufferResourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&stagingBuffer)));

  // Map the upload staging buffer to host visible memory.
  D3D12_RANGE stagingBufferRange{0, stagingBufferSize};
  DType *pStagingBufferData{};
  ThrowIfFailed(stagingBuffer->Map(
      0, &stagingBufferRange, reinterpret_cast<void **>(&pStagingBufferData)));

  // Populate the staging buffer with our upload data.
  for (int i = 0; i <_numElems; ++i) {
    pStagingBufferData[i] = m_srcData[i];
  }

  // Unmap the staging buffer.
  D3D12_RANGE emptyRange{0, 0};
  stagingBuffer->Unmap(0, &emptyRange);

  // Reset command list to inital state if necessary.
  std::ignore =_device.resetCmdList();

  // Set the copy source and destination footprint/locations.
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint = {};
  bufferFootprint.Footprint.Width = _imgDims.wdth;
  bufferFootprint.Footprint.Height = _imgDims.hght;
  bufferFootprint.Footprint.Depth = _imgDims.dpth;
  bufferFootprint.Footprint.RowPitch = _imgDims.wdth * sizeof(DType) * NChannels;
  bufferFootprint.Footprint.Format = toDXGIFormat<NChannels>(_elemType);

  D3D12_TEXTURE_COPY_LOCATION copyDst = {};
  copyDst.pResource = _texture.Get();
  copyDst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  copyDst.SubresourceIndex = 0;

  D3D12_TEXTURE_COPY_LOCATION copySrc = {};
  copySrc.pResource = stagingBuffer.Get();
  copySrc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  copySrc.PlacedFootprint = bufferFootprint;

  // Copy the upload buffer data to our texture.
  auto *cmdList =_device.getCmdList();
  cmdList->CopyTextureRegion(&copyDst, 0, 0, 0, &copySrc, nullptr);

  D3D12_RESOURCE_BARRIER transitionResourceBarrier = {};
  transitionResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  transitionResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  transitionResourceBarrier.Transition.pResource = _texture.Get();
  transitionResourceBarrier.Transition.StateBefore =
      D3D12_RESOURCE_STATE_COPY_DEST;
  transitionResourceBarrier.Transition.StateAfter =
      D3D12_RESOURCE_STATE_COPY_SOURCE;
  transitionResourceBarrier.Transition.Subresource =
      D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  cmdList->ResourceBarrier(1, &transitionResourceBarrier);

  // Execute the command list.
  ThrowIfFailed(cmdList->Close());
  ID3D12CommandList *ppCommandLists[] = {cmdList};
  auto *cmdQueue =_device.getCmdQueue();
  cmdQueue->ExecuteCommandLists(_countof(ppCommandLists),
                                        ppCommandLists);
  ThrowIfFailed(cmdQueue->Signal(_fence.Get(), _fenceVal));

#ifdef TEST_SEMAPHORE_IMPORT
  // Don't wait for the fence here. We will use the SYCL API to wait for this
  // fence in `callSYCLKernel`.
#else  // ifdef TEST_SEMAPHORE_IMPORT
  waitFence();
#endif // else TEST_SEMAPHORE_IMPORT
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
bool DX12Interop<NDims, DType, NChannels>::validateOutput() {

  using VecType = sycl::vec<DType, NChannels>;

  // Reset the command list.
  ThrowIfFailed(_device.resetCmdList());

  // Get intermediate readback buffer size.
  uint64_t bufSize = 0;
  auto *dev =_device.getDevice();
  dev->GetCopyableFootprints(&_texture->GetDesc(), 0, 1, 0, nullptr,
                             nullptr, nullptr, &bufSize);

  // Define readback heap properties.
  D3D12_HEAP_PROPERTIES heapProps = {};
  heapProps.Type                 = D3D12_HEAP_TYPE_READBACK;
  heapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapProps.CreationNodeMask     = 1;
  heapProps.VisibleNodeMask      = 1;

  // Define readback buffer resource descriptor.
  D3D12_RESOURCE_DESC bufDesc = {};
  bufDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
  bufDesc.Alignment        = 0;
  bufDesc.Width            = bufSize;
  bufDesc.Height           = 1;
  bufDesc.DepthOrArraySize = 1;
  bufDesc.MipLevels        = 1;
  bufDesc.Format           = DXGI_FORMAT_UNKNOWN;
  bufDesc.SampleDesc       = DXGI_SAMPLE_DESC{1, 0};
  bufDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  bufDesc.Flags            = D3D12_RESOURCE_FLAG_NONE;

  // Create the readback buffer.
  ComPtr<ID3D12Resource> readbackBuffer;
  ThrowIfFailed(dev->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
                                             &bufDesc,
                                             D3D12_RESOURCE_STATE_COPY_DEST,
                                             nullptr,
                                             IID_PPV_ARGS(&readbackBuffer)));

  // Set the copy source and destination footprint/locations.
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint = {};
  bufferFootprint.Footprint.Width    =_imgDims.wdth;
  bufferFootprint.Footprint.Height   =_imgDims.hght;
  bufferFootprint.Footprint.Depth    =_imgDims.dpth;
  bufferFootprint.Footprint.RowPitch =_imgDims.wdth * sizeof(DType) * NChannels;
  bufferFootprint.Footprint.Format   = toDXGIFormat<NChannels>(_elemType);

  D3D12_TEXTURE_COPY_LOCATION copyDst = {};
  copyDst.pResource = readbackBuffer.Get();
  copyDst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  copyDst.PlacedFootprint = bufferFootprint;

  D3D12_TEXTURE_COPY_LOCATION copySrc = {};
  copySrc.pResource = _texture.Get();
  copySrc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  copySrc.SubresourceIndex = 0;

  // Copy the texture to our readback buffer.
  auto *cmdList =_device.getCmdList();
  cmdList->CopyTextureRegion(&copyDst, 0, 0, 0, &copySrc, nullptr);

  // Execute the command list.
  ThrowIfFailed(cmdList->Close());
  ID3D12CommandList *ppCommandLists[] = {cmdList};
  auto *cmdQueue =_device.getCmdQueue();
  cmdQueue->ExecuteCommandLists(_countof(ppCommandLists),
                                        ppCommandLists);
  ThrowIfFailed(cmdQueue->Signal(_fence.Get(), _fenceVal));

  // Wait for the command list to finish execution and increment the fence
  // value.
  waitFence();

  // Map the readback buffer to host visible memory.
  D3D12_RANGE bufRange{0, bufSize};
  DType *pReadbackBufferData{};
  ThrowIfFailed(
      readbackBuffer->Map(0, &bufRange,
                          reinterpret_cast<void **>(&pReadbackBufferData)));

  // Wait for the GPU. Sometimes the Mapped memory isn't immediately visible to
  // the host
  ThrowIfFailed(cmdQueue->Signal(_fence.Get(), _fenceVal));
  waitFence();

  // Read back the updated texture data and validate it.
#ifdef VERBOSE_PRINT
  bool prevMismatch = false;
  DType prevVal = pReadbackBufferData[0];
  DType prevExp = prevVal;

  auto printMismatch = [&](uint64_t i, DType expect, DType value) {
    std::ostringstream x; x << std::setw(4) << i %  _imgDims.wdth;
    std::ostringstream y; y << std::setw(4) << i /  _imgDims.wdth %_imgDims.hght;
    std::ostringstream z; z << std::setw(4) << i / (_imgDims.wdth *_imgDims.hght);
    std::string p = (NDims == 1 ? "x = " + x.str() :
                     NDims == 2 ? "(y,x)=(" + y.str() + "," + x.str() + ")" :
                                  "(z,y,x)=(" + z.str() + "," + y.str() + "," + x.str() + ")");
    std::cerr << "Result mismatch [" << p << ":idx=" << std::setw(6) << i
              << "] -- Expected=" << expect << ", Actual=" << value << "\n";
  };
#endif // ifdef VERBOSE_PRINT
  bool pass = true;
  for (uint64_t i = 0; i <_numElems; ++i) {
    bool mismatch = false;
    auto expect = m_srcData[i] * 2;
    auto actual = pReadbackBufferData[i];

    if (actual != expect) {
      mismatch = true;
      pass = false;
    }

#ifdef VERBOSE_PRINT
    if (mismatch) {
      if (!prevMismatch) printMismatch(i, expect, actual);
      // std::cout << "Result mismatch at " << i << "! Expected: " << expect
      //           << ", Actual: " << actual << std::endl;
      else if (i ==_numElems - 1) {
        std::cerr << "... All results in between also mismatched.\n";
        printMismatch(i, expect, actual);
      }
    } else if (prevMismatch) {
      std::cerr << "... All results in between also mismatched.\n";
      printMismatch(i - 1, prevExp, prevVal);
    }
    prevExp = expect;
    prevVal = actual;
    prevMismatch = mismatch;
#else  // ifdef VERBOSE_PRINT
    if (mismatch) break;
#endif // else VERBOSE_PRINT
  }

  // Unmap the readback buffer.
  D3D12_RANGE emptyRange{0, 0};
  readbackBuffer->Unmap(0, &emptyRange);

  // Signal the fence to wait upon before we can clean up DX12 later.
  ThrowIfFailed(cmdQueue->Signal(_fence.Get(),_fenceVal));

  return pass;
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
void DX12Interop<NDims, DType, NChannels>::waitFence(DWORD msTimeout) {
  std::cerr << "ENTER: DX12Interop<NDims, DType, NChannels>::waitFence(DWORD " << msTimeout << ")\n";
  std::cerr << "\tWaiting for fence value: " <<_fenceVal << "\n";
  // Check the current value of the fence to check if
  // GPU has finished executing the command list.
  if (_fence->GetCompletedValue() <_fenceVal) {
    // If not, set value fence is to set on completion.
    ThrowIfFailed(_fence->SetEventOnCompletion(_fenceVal,
                                                    _fenceEvent));
    // Wait for fence to be triggered.
    WaitForSingleObject(_fenceEvent, msTimeout);
  }
  _fenceVal++;
  std::cerr << "\tUpdated fence value: " <<_fenceVal << "\n";
  std::cerr << "LEAVE: DX12Interop<NDims, DType, NChannels>::waitFence(DWORD)\n";
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
void DX12Interop<NDims, DType, NChannels>::cleanupDX12() {
  // Wait for the command list to finish execution.
  waitFence();

  // Clean up opened handles
  if (_semaphore != INVALID_HANDLE_VALUE)
    CloseNTHandle(_semaphore);
  CloseNTHandle(_memHandle);
  CloseHandle(_fenceEvent);

  // ComPtr handles will be destroyed automatically.
}
//-==========================================================================-//

//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
static bool
runTest(DX12SYCLDevice &device, sycl::image_channel_type channelType,
        Dims3D<NDims> imgDims, Dims3D<NDims> grpDims) {

  using DX12Test = DX12Interop<NDims, DType, NChannels>;

#ifdef VERBOSE_PRINT
  std::string gblSize = std::to_string(imgDims.wdth) + "(w)" +
                        (NDims > 1 ? "," + std::to_string(imgDims.hght) + "(h)" : "") +
                        (NDims > 2 ? "," + std::to_string(imgDims.dpth) + "(d)" : "");
  std::string lclSize = std::to_string(grpDims.wdth) + "(w)" +
                        (NDims > 1 ? "," + std::to_string(grpDims.hght) + "(h)" : "") +
                        (NDims > 2 ? "," + std::to_string(grpDims.dpth) + "(d)" : "");
  std::cerr << "Running test: NDims=" << NDims << "  NChannels=" << NChannels
            << "  image_channel_type="
            << bindless_helpers::channelTypeToString(channelType) << " --> size=("
            << gblSize << ":" << lclSize << ")\n";
      
#endif // ifdef VERBOSE_PRINT
  // Skip unorm_int8 tests for Level Zero backend
  auto backend = device.getSyclQueue().get_device().get_backend();
#ifdef VERBOSE_PRINT
  std::cerr << "  SYCL backend: " << static_cast<int>(backend) << "\n";
#endif // ifdef VERBOSE_PRINT
  if (channelType == sycl::image_channel_type::unorm_int8 &&
      backend == sycl::backend::ext_oneapi_level_zero) {
    std::cerr << "Skipping unorm_int8 test for Level Zero backend.\n";
    return true;
  }

#ifdef VERBOSE_PRINT
  std::cerr << "  Create SYCL image descriptor\n";
#endif // ifdef VERBOSE_PRINT
  syclexp::image_descriptor imgDesc{imgDims, NChannels, channelType};
#ifdef VERBOSE_PRINT
  std::cerr << "  SYCL image descriptor created: size=(" << imgDesc.width << "(w)," << imgDesc.height << "(h)," << imgDesc.depth << "(d))\n";
#endif // ifdef VERBOSE_PRINT

  // Verify ability to allocate the above image descriptor.
  // E.g. LevelZero does not support `unorm` channel types.
#ifdef VERBOSE_PRINT
  std::cerr << "  Get SYCL queue\n";
#endif // ifdef VERBOSE_PRINT
  sycl::queue syclQueue = device.getSyclQueue();
#ifdef VERBOSE_PRINT
  std::cerr << "  SYCL queue\n";
#endif // ifdef VERBOSE_PRINT
  bool supported = bindless_helpers::memoryAllocationSupported(
      imgDesc, syclexp::image_memory_handle_type::opaque_handle,
      syclQueue);
#ifdef VERBOSE_PRINT
  std::cerr << "  Memory allocation supported: " << supported << "\n";
#endif // ifdef VERBOSE_PRINT
  if (!supported) {
    // We cannot allocate the image memory, skip the test.
    std::cerr << "Memory allocation unsupported. Skipping test.\n";
    return true;
  }

#ifdef VERBOSE_PRINT
  std::cerr << "  Create test instance\n";
#endif // ifdef VERBOSE_PRINT
  DX12Test interopTest(device, channelType, imgDims, grpDims);

  interopTest.init();
  interopTest.callSYCLKernel();
  bool pass = interopTest.validateOutput();
  interopTest.cleanupDX12();

#ifdef VERBOSE_PRINT
  if (!pass) {
    std::cerr << "\tTest failed: NDims " << NDims << " NChannels " << NChannels
              << " image_channel_type "
              << bindless_helpers::channelTypeToString(channelType)
              << ", exiting\n";
    exit(-1);
  } else {
    std::cout << "\tTest passed: NDims " << NDims << " NChannels " << NChannels
              << " image_channel_type "
              << bindless_helpers::channelTypeToString(channelType) << "\n";
  }
#endif // ifdef VERBOSE_PRINT

  return pass;
}
//----------------------------------------------------------------------------//
int main() {
#ifdef VERBOSE_PRINT
  std::cerr << "**** Running bindless images read write unsampled tests ****\n";
#endif // ifdef VERBOSE_PRINT
  DX12SYCLDevice device;
#ifdef VERBOSE_PRINT
  std::cerr << "  Checkpoint 1.\n";
#endif // ifdef VERBOSE_PRINT

  bool pass = true;

  // Dims3D<1> imgDims1{4096};
  // Dims3D<1> grpDims1{1024};
  pass &= runTest<1, uint32_t, 1>(device, sycl_uint32, {64}, {16});
  // pass &= runTest<1, uint32_t,   1>(device, sycl_uint32, imgDims1, grpDims1);
  // pass &= runTest<1, uint8_t,    4>(device, sycl_unorm8, imgDims1, grpDims1);
  // pass &= runTest<1, float,      1>(device, sycl_float,  imgDims1, grpDims1);
  // pass &= runTest<1, sycl::half, 2>(device, sycl_half,   imgDims1, grpDims1);
  // pass &= runTest<1, sycl::half, 4>(device, sycl_half,   imgDims1, grpDims1);

#ifdef TEST_SMALL_IMAGE_SIZE
  Dims3D<2> imgDims2[] = {{64, 64}, {64, 64}, {64, 64}, {64, 64}, {64, 64}};
#else
  Dims3D<2> imgDims2[] = {{32, 16}, {1920, 1080}, {1920, 1080}, {2048, 2048}, {2048, 2048}};
#endif
  // pass &= runTest<2, uint32_t,   1>(device, sycl_uint32, imgDims2[0], { 8,  8});
  // pass &= runTest<2, uint8_t,    4>(device, sycl_unorm8, imgDims2[1], {16,  8});
  // pass &= runTest<2, float,      1>(device, sycl_float,  imgDims2[2], {16,  8});
  // pass &= runTest<2, sycl::half, 2>(device, sycl_half,   imgDims2[3], {16, 16});
  // pass &= runTest<2, sycl::half, 4>(device, sycl_half,   imgDims2[4], {16, 16});

// #ifdef TEST_SMALL_IMAGE_SIZE
//   Dims3D<3> imgDims3[] = {{64, 16, 4}, {64, 16, 4}, {64, 64, 4}, {64, 64, 4}, {64, 64, 4}};
// #else
//   Dims3D<3> imgDims3[] = {{1024, 1024, 16}, {1920, 1080, 8},
//                           {1920, 1080,  8}, {2048, 2048, 4}, {2048, 2048, 4}};
// #endif
//   pass &= runTest<3, uint32_t,   1>(device, sycl_uint32, imgDims3[0], {16, 16, 1});
//   pass &= runTest<3, uint8_t,    4>(device, sycl_unorm8, imgDims3[1], {16,  8, 2});
//   pass &= runTest<3, float,      1>(device, sycl_float,  imgDims3[2], {16,  8, 1});
//   pass &= runTest<3, sycl::half, 2>(device, sycl_half,   imgDims3[3], {16, 16, 1});
//   pass &= runTest<3, sycl::half, 4>(device, sycl_half,   imgDims3[4], {16, 16, 1});

  if (pass) {
    std::cout << "Tests passed!" << std::endl;
    return 0;
  }

  std::cerr << "Tests failed!" << std::endl;

  return 1;
}
//----------------------------------------------------------------------------//
