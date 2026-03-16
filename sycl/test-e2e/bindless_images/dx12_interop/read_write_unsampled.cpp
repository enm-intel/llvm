// REQUIRES: aspect-ext_oneapi_external_memory_import
// REQUIRES: windows

// UNSUPPORTED: gpu-intel-gen12
// UNSUPPORTED-INTENDED: Unknown issue with integrated GPU failing
//                       when importing memory

// UNSUPPORTED: gpu-intel-dg2
// UNSUPPORTED-TRACKER: https://github.com/intel/llvm/issues/21159

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
#include <format>   // for std::format

// #define VERBOSE_PRINT

#define DEBUG_SAMPLE_IMG 1
#ifdef VERBOSE_PRINT
#define VERBOSE_DEBUG DEBUG_SAMPLE_IMG
#else
#define VERBOSE_DEBUG 0
#endif

using sycl_mem_handle_t = syclexp::image_memory_handle_type;
using sycl_img_desc_t   = syclexp::image_descriptor;
using sycl_unsamp_img_t = syclexp::unsampled_image_handle;

constexpr bool LineSep = true;

constexpr uint32_t DefltPrec = 2;    // Default print precision of floats.
constexpr uint32_t DefltCols = 500;  // Default print columns (in console).
constexpr uint32_t DefltRows = 256;  // Default print rows.

template<typename T>
constexpr uint32_t Precision = std::is_floating_point_v<T> ? DefltPrec : 0;

//----------------------------------------------------------------------------//
void pause() {
  std::cin.clear(); // Clear any potential previous input left in the buffer.
  // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  std::cout << "\n\nPress Enter to continue . . .\n";   
  std::cin.get(); 
}
//----------------------------------------------------------------------------//
void printString(std::string str) {
#ifdef VERBOSE_PRINT
  std::cout << str << "\n";
#endif
}
//----------------------------------------------------------------------------//
template<typename InT, uint32_t NChannels = 1, typename AsT = InT>
void printBuf(const InT *data, const uint32_t len,
              const uint32_t prec = Precision<AsT>,
              const uint32_t cols = DefltCols,
              const uint32_t rows = DefltRows) {
  std::cerr << "[SYCL_DX12_INTEROP] ENTER: printBuf(const InT *, uint32_t, ...)\n";
  const size_t numVals = len * NChannels;

  constexpr uint32_t Last = NChannels - 1;
  constexpr uint32_t Half = Last / 2;
  constexpr AsT      Zero = static_cast<AsT>(0);
  constexpr bool     Sep  = LineSep && (NChannels > 1);

  std::cout << "\n========================================\n";
  std::cout << "Buffer size: " << len << "\n";
  std::cout << "# channels : " << NChannels << "\n";
  std::cout << "Data ptr   : " << data << "\n";

  AsT minVal = static_cast<AsT>(data[0]);
  AsT maxVal = minVal;
  for (uint32_t x = 0; x < numVals; ++x) {
    AsT val = static_cast<AsT>(data[x]);
    if      (val > maxVal) maxVal = val;
    else if (val < minVal) minVal = val;
  }
  std::cout << "Extrema   : " << minVal << " .. " << maxVal << "\n";

  if (maxVal < Zero) maxVal = -maxVal;
  AsT maxMag = (minVal < Zero ? -minVal : minVal);
  if (maxVal > maxMag) maxMag = maxVal;

  if (std::is_integral_v<AsT> && maxMag < len)
    maxMag = static_cast<AsT>(len); // Ensure that column indices fit.

  uint32_t digits = 1;
  maxVal = static_cast<AsT>(10);
  for (AsT val = maxMag; val >= AsT(10) && digits < 10; ++digits) {
    // std::cout << "   Val: " << val << "  Digits: " << digits << "\n";
    val    /= static_cast<AsT>(10);
    maxVal *= static_cast<AsT>(10);
  }
  uint32_t elemW = digits + prec;        // +1 for pad between elements.
  elemW += (prec > 0) + (minVal < Zero); // Decimal point & negative.
  // std::cout << "Max. print: " << maxVal << "\n";
  // std::cout << "# digits  : " << digits << "\n";
  // std::cout << "Col. width: " << elemW << "\n";

  const uint32_t maxW  = cols / (elemW + 1); // +1 for space between columns.
  const uint32_t maxH  = rows / (NChannels + Sep);
  const uint32_t halfW = maxW / 2;
  const uint32_t halfH = maxH / 2;
  const bool allX = (len <= maxW);

  std::cout << "     ";
  for (uint32_t x = 0, x_e = len; x_e--; ++x) {
    if (allX || x < halfW || x_e < halfW)
      std::cout << std::format("{:^{}d}", x, elemW + 1);
    else if (x == halfW) std::cout << "...  ";
  }
  std::cout << "\n    ";
  for (uint32_t x = 0, x_e = len; x_e--; ++x) {
    if (allX || x < halfW || x_e < halfW) {
      const char sep = (Sep ? '+' : '-');
      std::cout << std::format("{}{:->{}s}", sep, "-", elemW);
    }
    else if (x == halfW) std::cout << " ... ";
  }
  std::cout << "+\n";
  for (uint32_t c = 0; c < NChannels; ++c) {
    std::cout << "    ";
    for (uint32_t x = 0, x_e = len, i = 0; x_e--; ++x, i += NChannels) {
      if (allX || x < halfW || x_e < halfW) {
        AsT val = static_cast<AsT>(data[i]);
        if (Sep || !Last || c != Last) {
          const char sep = (Sep || x == 0) ? '|' : ' ';
          if constexpr (std::is_floating_point_v<AsT>) {
            if (std::abs(val) >= maxVal)
              std::cout << std::format("{}{:{}e}", sep, val, elemW);
            else
              std::cout << std::format("{}{:{}.{}f}", sep, val, elemW, prec);
          }
          else if constexpr (std::is_integral_v<AsT>)
            std::cout << std::format("{}{:{}d}", sep, val, elemW);
        }
        else {
          if constexpr (std::is_floating_point_v<AsT>)
            if (std::abs(val) >= maxVal)
              std::cout << std::format("_{:_>{}e}", val, elemW);
            else
              std::cout << std::format("_{:_>{}.{}f}", val, elemW, prec);
          else if constexpr (std::is_integral_v<AsT>)
            std::cout << std::format("_{:_>{}d}", val, elemW);
        }
      }
      else if (x == halfW) std::cout << " ... ";
    }
    data++;
    std::cout << "|\n";
  }

  std::cout << "    ";
  for (uint32_t x = 0, x_e = len; x_e--; ++x) {
    if (allX || x < halfW || x_e < halfW) {
      const char sep = (Sep ? '+' : '-');
      std::cout << std::format("{}{:->{}s}", sep, "-", elemW);
    }
    else if (x == halfW) std::cout << " ... ";
  }
  std::cout << "+\n\n";
  std::cerr << "[SYCL_DX12_INTEROP] LEAVE: printBuf(const InT *, uint32_t, ...)\n";
}
//----------------------------------------------------------------------------//
template<typename InT, uint32_t NChannels = 1, typename AsT = InT>
void printImg(const InT *data, const uint32_t wdth, const uint32_t hght,
              const uint32_t prec = Precision<AsT>,
              const uint32_t cols = DefltCols,
              const uint32_t rows = DefltRows) {
  std::cerr << "[SYCL_DX12_INTEROP] ENTER: printImg(const InT *, uint32_t, uint32_t, ...)\n";
  const size_t rowVals = wdth * NChannels;

  constexpr uint32_t Last = NChannels - 1;
  constexpr uint32_t Half = Last / 2;
  constexpr AsT      Zero = static_cast<AsT>(0);
  constexpr bool     Sep  = LineSep && (NChannels > 1);

  std::cout << "\n========================================\n";
  std::cout << "Image size: " << wdth << "(w)" << " x " << hght << "(h)\n";
  std::cout << "# channels: " << NChannels << "\n";
  std::cout << "Data ptr  : " << data << "\n";

  AsT minVal = static_cast<AsT>(data[0]);
  AsT maxVal = minVal;
  for (uint32_t y = 0; y < hght; ++y) {
      const InT *row = data + y * rowVals;
      for (uint32_t x = 0; x < rowVals; ++x) {
          AsT val = static_cast<AsT>(row[x]);
          if      (val > maxVal) maxVal = val;
          else if (val < minVal) minVal = val;
      }
  }
  std::cout << "Extrema   : " << minVal << " .. " << maxVal << "\n";

  if (maxVal < Zero) maxVal = -maxVal;
  AsT maxMag = (minVal < Zero ? -minVal : minVal);
  if (maxVal > maxMag) maxMag = maxVal;

  if (std::is_integral_v<AsT> && maxMag < wdth)
      maxMag = static_cast<AsT>(wdth); // Ensure that column indices fit.

  uint32_t digits = 1;
  maxVal = static_cast<AsT>(10);
  for (AsT val = maxMag; val >= AsT(10) && digits < 10; ++digits) {
      // std::cout << "   Val: " << val << "  Digits: " << digits << "\n";
      val    /= static_cast<AsT>(10);
      maxVal *= static_cast<AsT>(10);
  }
  uint32_t elemW = digits + prec;        // +1 for pad between elements.
  elemW += (prec > 0) + (minVal < Zero); // Decimal point & negative.
  // std::cout << "Max. print: " << maxVal << "\n";
  // std::cout << "# digits  : " << digits << "\n";
  // std::cout << "Col. width: " << elemW << "\n";

  const uint32_t maxW  = cols / (elemW + 1); // +1 for space between columns.
  const uint32_t maxH  = rows / (NChannels + Sep);
  const uint32_t halfW = maxW / 2;
  const uint32_t halfH = maxH / 2;
  const bool allX = (wdth <= maxW);
  const bool allY = (hght <= maxH);

  std::cout << "     ";
  for (uint32_t x = 0, x_e = wdth; x_e--; ++x) {
      if (allX || x < halfW || x_e < halfW)
          std::cout << std::format("{:^{}d}", x, elemW + 1);
      else if (x == halfW) std::cout << "...  ";
  }
  std::cout << "\n    ";
  for (uint32_t x = 0, x_e = wdth; x_e--; ++x) {
      if (allX || x < halfW || x_e < halfW) {
          const char sep = (Sep ? '+' : '-');
          std::cout << std::format("{}{:->{}s}", sep, "-", elemW);
      }
      else if (x == halfW) std::cout << " ... ";
  }
  std::cout << "+\n";
  for (uint32_t y = 0, y_e = hght; y_e--; ++y) {
    if (allY || y < halfH || y_e < halfH) {
      const InT *row = data + y * rowVals;
      for (uint32_t c = 0; c < NChannels; ++c) {
        if (c == Half) std::cout << std::format("{:4}", y);
        else           std::cout << "    ";
        for (uint32_t x = 0, x_e = wdth, i = 0; x_e--; ++x, i += NChannels) {
          if (allX || x < halfW || x_e < halfW) {
            AsT val = static_cast<AsT>(row[i]);
            if (Sep || !Last || c != Last) {
              const char sep = (Sep || x == 0) ? '|' : ' ';
              if constexpr (std::is_floating_point_v<AsT>) {
                if (std::abs(val) >= maxVal)
                  std::cout << std::format("{}{:{}e}", sep, val, elemW);
                else
                  std::cout << std::format("{}{:{}.{}f}", sep, val, elemW, prec);
              }
              else if constexpr (std::is_integral_v<AsT>)
                std::cout << std::format("{}{:{}d}", sep, val, elemW);
            }
            else {
              if constexpr (std::is_floating_point_v<AsT>)
                if (std::abs(val) >= maxVal)
                  std::cout << std::format("_{:_>{}e}", val, elemW);
                else
                  std::cout << std::format("_{:_>{}.{}f}", val, elemW, prec);
              else if constexpr (std::is_integral_v<AsT>)
                std::cout << std::format("_{:_>{}d}", val, elemW);
            }
          }
          else if (x == halfW) std::cout << " ... ";
        }
        row++;
        std::cout << "|\n";
      }
      if (Sep && y_e) {
        std::cout << "    ";
        for (uint32_t x = 0, x_e = wdth; x_e--; ++x) {
          if (allX || x < halfW || x_e < halfW)
            std::cout << std::format("+{:->{}s}", "-", elemW);
          else if (x == halfW) std::cout << " ... ";
        }
        std::cout << "+\n";
      }
    }
    else if (y == halfH) std::cout << "\n    .\n    .\n    .\n";
  }
  std::cout << "    ";
  for (uint32_t x = 0, x_e = wdth; x_e--; ++x) {
    if (allX || x < halfW || x_e < halfW) {
      const char sep = (Sep ? '+' : '-');
      std::cout << std::format("{}{:->{}s}", sep, "-", elemW);
    }
    else if (x == halfW) std::cout << " ... ";
  }
  std::cout << "+\n\n";
  std::cerr << "[SYCL_DX12_INTEROP] LEAVE: printImg(const InT *, uint32_t, uint32_t, ...)\n";
}
//----------------------------------------------------------------------------//

//-==========================================================================-//
DX12SYCLDevice::DX12SYCLDevice()
    :_syclQueue{{sycl::property::queue::in_order{}}},
     _syclDev{_syclQueue.get_device()} {
  std::cerr << "[SYCL_DX12_INTEROP] ENTER: DX12SYCLDevice::DX12SYCLDevice()\n";
  _initDevice();
  _initCmdList();
  std::cerr << "[SYCL_DX12_INTEROP] LEAVE: DX12SYCLDevice::DX12SYCLDevice()\n";
}
//----------------------------------------------------------------------------//
void DX12SYCLDevice::_initDevice() {
  std::cerr << "[SYCL_DX12_INTEROP] ENTER: DX12SYCLDevice::_initDevice()\n";
  // Create DXGI factory.
  ThrowIfFailed(CreateDXGIFactory2(0 /* flags */, IID_PPV_ARGS(&_factory)));

  // Get the hardware adapter for a suitable GPU.
  _adapter = getDXAdapter<dx_version::DX12>(_factory.Get(),
                                            _syclDev.get_info<sycl::info::device::name>());

  // Create a device from our hardware adapter.
  ThrowIfFailed(D3D12CreateDevice(_adapter.Get(), D3D_FEATURE_LEVEL_12_0,
                                  IID_PPV_ARGS(&_device)));
  std::cerr << "[SYCL_DX12_INTEROP] LEAVE: DX12SYCLDevice::_initDevice()\n";
}
//----------------------------------------------------------------------------//
void DX12SYCLDevice::_initCmdList() {
  std::cerr << "[SYCL_DX12_INTEROP] ENTER: DX12SYCLDevice::_initCmdList()\n";
  // Describe and create the command queue.
  D3D12_COMMAND_QUEUE_DESC qDesc = {D3D12_COMMAND_LIST_TYPE_DIRECT, 0,
                                    D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
  ThrowIfFailed(_device->CreateCommandQueue(&qDesc, IID_PPV_ARGS(&_dxQueue)));

  // Create the command allocator.
  ThrowIfFailed(_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                IID_PPV_ARGS(&_cmdAlloc)));

  // Create the command list.
  ThrowIfFailed(_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                           _cmdAlloc.Get(), nullptr,
                                           IID_PPV_ARGS(&_cmdList)));
  std::cerr << "[SYCL_DX12_INTEROP] LEAVE: DX12SYCLDevice::_initCmdList()\n";
}
//-==========================================================================-//

//-==========================================================================-//
template <uint32_t NDims, typename DType, uint32_t NChannels>
DX12Interop<NDims, DType, NChannels>::DX12Interop(DX12SYCLDevice &device,
                                                  sycl_channel_t channelType,
                                                  const Dims3D<NDims> dataDims,
                                                  const Dims3D<NDims> groupDims)
    :_device(device),
     _elemType(channelType),
     _dataDims(dataDims),
     _groupDims(groupDims) {
  _wdth =_dataDims.wdth;
  _hght =_dataDims.hght;
  _dpth =_dataDims.dpth;
  _numElems = to_size(_wdth) * to_size(_hght) * to_size(_dpth) * NChannels;
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
void DX12Interop<NDims, DType, NChannels>::initDX12Resources() {

  // Define default heap properties.
  D3D12_HEAP_PROPERTIES heapProps = {};
  heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
  heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapProps.CreationNodeMask = 1;
  heapProps.VisibleNodeMask = 1;

  // Define texture resource descriptor.
  D3D12_RESOURCE_DESC texDesc = {};
  if constexpr (NDims == 1)
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
  else if constexpr (NDims == 2)
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  else
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
  texDesc.Alignment        = 0;
  texDesc.Width            =_wdth;
  texDesc.Height           =_hght;
  texDesc.DepthOrArraySize =_dpth;
  texDesc.MipLevels        = 0;
  texDesc.Format           = toDXGIFormat<NChannels>(_elemType);
  texDesc.SampleDesc       = DXGI_SAMPLE_DESC{1, 0};
  texDesc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  texDesc.Flags            = D3D12_RESOURCE_FLAG_NONE;

  // Create the DX12 texture.
  auto *dx12Device =_device.getDxDevice();
  ThrowIfFailed(dx12Device->CreateCommittedResource(
      &heapProps, D3D12_HEAP_FLAG_SHARED, &texDesc,
      D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&_texture)));

  // Create a shared handle for our texture.
  ThrowIfFailed(dx12Device->CreateSharedHandle(_texture.Get(), nullptr,
                                               GENERIC_ALL, nullptr,
                                               &_memHandle));

  D3D12_RESOURCE_ALLOCATION_INFO texAllocInfo;
  texAllocInfo = dx12Device->GetResourceAllocationInfo(1, 1, &texDesc);
  size_t allocSize = texAllocInfo.SizeInBytes;

  // Import our shared DX12 texture resource to SYCL.
  importMemHandle(allocSize);

  // Create the DX12 fence and map to a SYCL semaphore.
  ThrowIfFailed(dx12Device->CreateFence(_fenceVal, D3D12_FENCE_FLAG_SHARED,
                                        IID_PPV_ARGS(&_fence)));
  _fenceVal++;

#ifdef TEST_SEMAPHORE_IMPORT
  ThrowIfFailed(dx12Device->CreateSharedHandle(_fence.Get(), nullptr,
                                               GENERIC_ALL, nullptr,
                                               &_semaphore));

  // Import our shared DX12 fence resource to SYCL.
  importSemaphore();
#endif

  // Create an event handle to use for synchronization.
  _fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (_fenceEvent == nullptr) {
    ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
  }

  populateDX12Texture();
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
void DX12Interop<NDims, DType, NChannels>::importMemHandle(size_t allocSize) {
  std::cerr << "[SYCL_DX12_INTEROP] ENTER: DX12Interop<NDims, DType, NChannels>::importMemHandle(size_t)\n";
  syclexp::external_mem_descriptor<syclexp::resource_win32_handle> extMemDesc{
      _memHandle,
      syclexp::external_mem_handle_type::win32_nt_dx12_resource,
      allocSize};

  auto &queue =_device.getSyclQueue();
  _syclMemHandle = syclexp::import_external_memory(extMemDesc, queue);

  syclexp::image_descriptor imgDesc{_dataDims, NChannels,_elemType};
  _syclImgMem = syclexp::map_external_image_memory(_syclMemHandle, imgDesc, queue);

  _syclImgHandle = syclexp::create_image(_syclImgMem, imgDesc, queue);
  std::cerr << "[SYCL_DX12_INTEROP] LEAVE: DX12Interop<NDims, DType, NChannels>::importMemHandle(size_t)\n";
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
void DX12Interop<NDims, DType, NChannels>::importSemaphore() {
  syclexp::external_semaphore_descriptor<syclexp::resource_win32_handle>
      semDesc{_semaphore, syclexp::external_semaphore_handle_type::win32_nt_dx12_fence};

  _syclSemaphore = syclexp::import_external_semaphore(semDesc,_device.getSyclQueue());
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
void DX12Interop<NDims, DType, NChannels>::callSYCLKernel() {
  using VecT = sycl::vec<DType, NChannels>;
  using PixT = std::conditional_t<NChannels == 1, DType, VecT>;

  auto &queue =_device.getSyclQueue();
#ifdef TEST_SEMAPHORE_IMPORT
  // Wait for imported semaphore. This semaphore was signalled at the
  // end of `populateDX12Texture`.
  queue.ext_oneapi_wait_external_semaphore(_syclSemaphore,_fenceVal);
#endif // ifdef TEST_SEMAPHORE_IMPORT

  printString("Submitting SYCL kernel");
#if VERBOSE_DEBUG
  std::cout << "\timage size: " <<_dataDims.wdth << "(w)";
  if constexpr (NDims >= 2) std::cout << " x " <<_dataDims.hght << "(h)";
  if constexpr (NDims == 3) std::cout << " x " <<_dataDims.dpth << "(d)";
  std::cout << " -> group size: " <<_groupDims.wdth << "(w)";
  if constexpr (NDims >= 2) std::cout << " x " <<_groupDims.hght << "(h)";
  if constexpr (NDims == 3) std::cout << " x " <<_groupDims.dpth << "(d)";
  std::cout << "\n";
  std::cout << "\t# elements: " <<_numElems << "\n";
  std::cout << "\t# channels: " << NChannels << "\n";
  std::cout << "\telem size : " << sizeof(DType) << " --> pixel size: "
            << sizeof(VecT) << "\n";
  std::cout << "\t# bytes   : " <<_numElems * sizeof(DType) << "\n";
  if (sizeof(DType) != sizeof(DType))
    std::cout << "\t***** NOTE: Output data type different from image data type *****\n";
#endif // VERBOSE_DEBUG  // Submit our SYCL kernel. All we do is double the value of each pixel in the

  // We can't capture the image handle through `this` in the lambda.
  // If we do the kernel will crash.
  auto imgHandle =_syclImgHandle;

  constexpr DType two = static_cast<DType>(2);
  sycl::range<NDims> gridDims = _dataDims.to_flip_range();
  sycl::range<NDims> blckDims =_groupDims.to_flip_range();

  constexpr DType two = static_cast<DType>(2);

  // Submit SYCL kernel to update texture data.
  try {
    queue.submit([&](sycl::handler &cgh) {
      cgh.parallel_for(
          sycl::nd_range<NDims>{gridDims,blckDims},
          [=](sycl::nd_item<NDims> it) {
            if constexpr (NDims == 3) {
              size_t z = it.get_global_id(0);
              size_t y = it.get_global_id(1);
              size_t x = it.get_global_id(2);
              sycl::int3 pos(x, y, z);
              auto px = syclexp::fetch_image<PixT>(imgHandle, pos);
              px *= two;
              syclexp::write_image(imgHandle, pos, px);
            } else if constexpr (NDims == 2) {
              size_t y = it.get_global_id(0);
              size_t x = it.get_global_id(1);
              sycl::int2 pos(x, y);
              auto px = syclexp::fetch_image<PixT>(imgHandle, pos);
              px *= two;
              syclexp::write_image(imgHandle, pos, px);
            } else {
              size_t x = it.get_global_id(0);
              int pos(x);
              auto px = syclexp::fetch_image<PixT>(imgHandle, pos);
              px *= two;
              syclexp::write_image(imgHandle, pos, px);
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
  queue.submit([&](sycl::handler &cgh) {
    cgh.ext_oneapi_signal_external_semaphore(_syclSemaphore,
                                             _fenceVal);
  });

  // Use DX12 to wait for the semaphore signalled by SYCL above.
  waitDxFence();
  _fenceVal++;
#else
  queue.wait();
#endif
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
void DX12Interop<NDims, DType, NChannels>::populateDX12Texture() {

  // Set our texture data to upload.
  _srcData.resize(_numElems);
  auto getInputValue = [&](size_t i) -> DType {
    if constexpr (std::is_integral_v<DType> ||
                  std::is_same_v<DType, sycl::half>)
      i = i % (static_cast<uint64_t>(std::numeric_limits<DType>::max()) / 2);
    return i;
  };
  for (size_t i = 0; i <_numElems; ++i) _srcData[i] = getInputValue(i);

  // Get required staging buffer size.
  uint64_t stagingBufferSize = 0;
  auto *dx12Device =_device.getDxDevice();
  dx12Device->GetCopyableFootprints(&_texture->GetDesc(), 0, 1, 0, nullptr,
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
  ThrowIfFailed(dx12Device->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadBufferResourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&stagingBuffer)));

  // Map the upload staging buffer to host visible memory.
  D3D12_RANGE stagingBufferRange{0, stagingBufferSize};
  DType *pStagingBufferData{};
  ThrowIfFailed(stagingBuffer->Map(
      0, &stagingBufferRange, reinterpret_cast<void **>(&pStagingBufferData)));

  // Populate the staging buffer with our upload data.
  for (size_t i = 0; i <_numElems; ++i) pStagingBufferData[i] =_srcData[i];

  // Unmap the staging buffer.
  D3D12_RANGE emptyRange{0, 0};
  stagingBuffer->Unmap(0, &emptyRange);

  // Reset command list to inital state if necessary.
  std::ignore =_device.resetCmdList();

  // Set the copy source and destination footprint/locations.
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint = {};
  bufferFootprint.Footprint.Width = _wdth;
  bufferFootprint.Footprint.Height = _hght;
  bufferFootprint.Footprint.Depth = _dpth;
  bufferFootprint.Footprint.RowPitch = _wdth * sizeof(DType) * NChannels;
  bufferFootprint.Footprint.Format = toDXGIFormat<NChannels>(_elemType);

  D3D12_TEXTURE_COPY_LOCATION copyDest = {};
  copyDest.pResource =_texture.Get();
  copyDest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  copyDest.SubresourceIndex = 0;

  D3D12_TEXTURE_COPY_LOCATION copySrc = {};
  copySrc.pResource = stagingBuffer.Get();
  copySrc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  copySrc.PlacedFootprint = bufferFootprint;

  // Copy the upload buffer data to our texture.
  auto *dx12CommandList =_device.getCmdList();
  dx12CommandList->CopyTextureRegion(&copyDest, 0, 0, 0, &copySrc, nullptr);

  D3D12_RESOURCE_BARRIER transitionResourceBarrier = {};
  transitionResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  transitionResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  transitionResourceBarrier.Transition.pResource =_texture.Get();
  transitionResourceBarrier.Transition.StateBefore =
      D3D12_RESOURCE_STATE_COPY_DEST;
  transitionResourceBarrier.Transition.StateAfter =
      D3D12_RESOURCE_STATE_COPY_SOURCE;
  transitionResourceBarrier.Transition.Subresource =
      D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  dx12CommandList->ResourceBarrier(1, &transitionResourceBarrier);

  // Execute the command list.
  ThrowIfFailed(dx12CommandList->Close());
  ID3D12CommandList *ppCommandLists[] = {dx12CommandList};
  auto *dx12CommandQueue =_device.getDxCmdQueue();
  dx12CommandQueue->ExecuteCommandLists(_countof(ppCommandLists),
                                        ppCommandLists);
  ThrowIfFailed(
      dx12CommandQueue->Signal(_fence.Get(),_fenceVal));

#ifdef TEST_SEMAPHORE_IMPORT
  // Don't wait for the fence here. We will use the SYCL API to wait for this
  // fence in `callSYCLKernel`.
#else
  waitDxFence();
  _fenceVal++;
#endif
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
bool DX12Interop<NDims, DType, NChannels>::validateOutput() {

  // Reset the command list.
  ThrowIfFailed(_device.resetCmdList());

  // Get intermediate readback buffer size.
  uint64_t readbackBufferSize = 0;
  auto *dx12Device =_device.getDxDevice();
  dx12Device->GetCopyableFootprints(&_texture->GetDesc(), 0, 1, 0, nullptr,
                                    nullptr, nullptr, &readbackBufferSize);

  // Define readback heap properties.
  D3D12_HEAP_PROPERTIES heapProps = {};
  heapProps.Type = D3D12_HEAP_TYPE_READBACK;
  heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapProps.CreationNodeMask = 1;
  heapProps.VisibleNodeMask = 1;

  // Define readback buffer resource descriptor.
  D3D12_RESOURCE_DESC bufDesc = {};
  bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  bufDesc.Alignment = 0;
  bufDesc.Width = readbackBufferSize;
  bufDesc.Height = 1;
  bufDesc.DepthOrArraySize = 1;
  bufDesc.MipLevels = 1;
  bufDesc.Format = DXGI_FORMAT_UNKNOWN;
  bufDesc.SampleDesc = DXGI_SAMPLE_DESC{1, 0};
  bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  bufDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  // Create the readback buffer.
  ComPtr<ID3D12Resource> readbackBuffer;
  ThrowIfFailed(dx12Device->CreateCommittedResource(
      &heapProps, D3D12_HEAP_FLAG_NONE,
      &bufDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
      IID_PPV_ARGS(&readbackBuffer)));

  // Set the copy source and destination footprint/locations.
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint = {};
  bufferFootprint.Footprint.Width = _wdth;
  bufferFootprint.Footprint.Height = _hght;
  bufferFootprint.Footprint.Depth = _dpth;
  bufferFootprint.Footprint.RowPitch = _wdth * sizeof(DType) * NChannels;
  bufferFootprint.Footprint.Format = toDXGIFormat<NChannels>(_elemType);

  D3D12_TEXTURE_COPY_LOCATION copyDest = {};
  copyDest.pResource = readbackBuffer.Get();
  copyDest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  copyDest.PlacedFootprint = bufferFootprint;

  D3D12_TEXTURE_COPY_LOCATION copySrc = {};
  copySrc.pResource =_texture.Get();
  copySrc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  copySrc.SubresourceIndex = 0;

  // Copy the texture to our readback buffer.
  auto *dx12CommandList =_device.getCmdList();
  dx12CommandList->CopyTextureRegion(&copyDest, 0, 0, 0, &copySrc, nullptr);

  // Execute the command list.
  ThrowIfFailed(dx12CommandList->Close());
  ID3D12CommandList *ppCommandLists[] = {dx12CommandList};
  auto *dx12CommandQueue =_device.getDxCmdQueue();
  dx12CommandQueue->ExecuteCommandLists(_countof(ppCommandLists),
                                        ppCommandLists);
  ThrowIfFailed(dx12CommandQueue->Signal(_fence.Get(),_fenceVal));

  // Wait for the command list to finish execution and increment the fence
  // value.
  waitDxFence();
  _fenceVal++;

  // Map the readback buffer to host visible memory.
  D3D12_RANGE readbackBufferRange{0, readbackBufferSize};
  DType *pReadbackBufferData{};
  ThrowIfFailed(
      readbackBuffer->Map(0, &readbackBufferRange,
                          reinterpret_cast<void **>(&pReadbackBufferData)));

  // Wait for the GPU. Sometimes the Mapped memory isn't immediately visible to
  // the host
  ThrowIfFailed(
      dx12CommandQueue->Signal(_fence.Get(),_fenceVal));
  waitDxFence();
  _fenceVal++;

  // Read back the updated texture data and validate it.
  bool valid = true;
  for (size_t i = 0; i <_numElems; ++i) {
    bool mismatch = false;
    auto expect   = _srcData[i] * 2;
    auto actual   = pReadbackBufferData[i];

    if (actual != expect) {
      mismatch = true;
      valid = false;
    }

    if (mismatch) {
#ifdef VERBOSE_PRINT
      std::cout << "Result mismatch at " << i << "! Expected: " << expected
                << ", Actual: " << actual << std::endl;
#else
      break;
#endif
    }
  }

  // Unmap the readback buffer.
  D3D12_RANGE emptyRange{0, 0};
  readbackBuffer->Unmap(0, &emptyRange);

  // Signal the fence to wait upon before we can clean up DX12 later.
  ThrowIfFailed(
      dx12CommandQueue->Signal(_fence.Get(),_fenceVal));

  return valid;
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
void DX12Interop<NDims, DType, NChannels>::waitDxFence(DWORD msecTimeout) {
  // Check the current value of the fence to check if
  // GPU has finished executing the command list.
  if (_fence->GetCompletedValue() <_fenceVal) {
    // If not, set value fence is to set on completion.
    ThrowIfFailed(_fence->SetEventOnCompletion(_fenceVal, _fenceEvent));
    // Wait for fence to be triggered.
    WaitForSingleObject(_fenceEvent, msecTimeout);
  }
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels>
void DX12Interop<NDims, DType, NChannels>::cleanupDX12() {
  // Wait for the command list to finish execution.
  waitDxFence();

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
runTest(DX12SYCLDevice &device, sycl_channel_t channelType,
        const Dims3D<NDims> dataDims, const Dims3D<NDims> groupDims) {

  // Skip unorm_int8 tests for Level Zero backend
  if (channelType == sycl_unorm8 &&
      device.getSyclQueue().get_device().get_backend() ==
          sycl::backend::ext_oneapi_level_zero) {
    std::cout << "Skipping unorm_int8 test for Level Zero backend.\n";
    return true;
  }

  syclexp::image_descriptor imgDesc{dataDims.to_sycl_range(), NChannels, channelType};

  // Verify ability to allocate the above image descriptor.
  // E.g. LevelZero does not support `unorm` channel types.
  if (!bindless_helpers::memoryAllocationSupported(
          imgDesc, syclexp::image_memory_handle_type::opaque_handle,
          device.getSyclQueue())) {
    // We cannot allocate the image memory, skip the test.
    std::cout << "Memory allocation unsupported. Skipping test.\n";
    return true;
  }

  DX12Interop<NDims, DType, NChannels> test(device, channelType, dataDims, groupDims);

  test.initDX12Resources();
  test.callSYCLKernel();
  bool valid = test.validateOutput();
  test.cleanupDX12();

#ifdef VERBOSE_PRINT
  if (!valid) {
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
#endif

  return valid;
}
//----------------------------------------------------------------------------//
int main() {
  DX12SYCLDevice device;

  pause();

  bool valid = true;

  //======== RUN 1D TESTS ========//
  valid &= runTest<1, uint32_t  , 1>(device, sycl_uint32, {1024}, {256});
  valid &= runTest<1, uint8_t   , 4>(device, sycl_unorm8, {1024}, {256});
  valid &= runTest<1, float     , 1>(device, sycl_float , {1024}, {256});
  valid &= runTest<1, sycl::half, 2>(device, sycl_half  , {1024}, {256});
  valid &= runTest<1, sycl::half, 4>(device, sycl_half  , {1024}, {256});

  //======== RUN 2D TESTS ========//
  // Dims3D<2> grid2D = {128, 64};
  // valid &= runTest<2, uint32_t, 1>(device, sycl_uint32, grid2D, {16, 16});
#ifdef TEST_SMALL_IMAGE_SIZE
  Dims3D<2> grid2D[] = {{64, 64}, {128, 64}, {64, 128},
                        {64, 64}, { 64, 64}};
#else
  Dims3D<2> grid2D[] = {{1024, 1024}, {1920, 1080}, {1920, 1080},
                        {2048, 2048}, {2048, 2048}};
#endif
  valid &= runTest<2, uint32_t  , 1>(device, sycl_uint32, grid2D[0], {16, 16});
  valid &= runTest<2, uint8_t   , 4>(device, sycl_unorm8, grid2D[1], {16,  8});
  valid &= runTest<2, float     , 1>(device, sycl_float , grid2D[2], {16,  8});
  valid &= runTest<2, sycl::half, 2>(device, sycl_half  , grid2D[3], {16, 16});
  valid &= runTest<2, sycl::half, 4>(device, sycl_half  , grid2D[4], {16, 16});

  //======== RUN 3D TESTS ========//
#ifdef TEST_SMALL_IMAGE_SIZE
  Dims3D<3> grid3D[] = {{64, 16, 4}, {64, 16, 4}, {64, 64, 4},
                        {64, 64, 4}, {64, 64, 4}};
#else
  Dims3D<3> grid3D[] = {{1024, 1024, 16}, {1920, 1080, 8}, {1920, 1080, 8},
                        {2048, 2048,  4}, {2048, 2048, 4}};
#endif
  const Dims3D<3> grp3D[] = {{16, 16, 1}, {16,  8, 2}, {16,  8, 1},
                             {16, 16, 1}, {16, 16, 1}};
  valid &= runTest<3, uint32_t  , 1>(device, sycl_uint32, grid3D[0], grp3D[0]);
  valid &= runTest<3, uint8_t   , 4>(device, sycl_unorm8, grid3D[1], grp3D[1]);
  valid &= runTest<3, float     , 1>(device, sycl_float , grid3D[2], grp3D[2]);
  valid &= runTest<3, sycl::half, 2>(device, sycl_half  , grid3D[3], grp3D[3]);
  valid &= runTest<3, sycl::half, 4>(device, sycl_half  , grid3D[4], grp3D[4]);

  if (valid) {
    std::cout << "Test passed!" << std::endl;
    return 0;
  }

  std::cerr << "Test failed!" << std::endl;

  return 1;
}
//----------------------------------------------------------------------------//
