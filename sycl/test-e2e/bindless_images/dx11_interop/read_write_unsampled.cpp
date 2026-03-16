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

#include "dx11_interop.h"

#include <sycl/sycl.hpp>
#include <sycl/ext/oneapi/bindless_images.hpp>

// #define DEFINE_TEXTURE_LAYOUT

#ifdef TEST_SEMAPHORE_IMPORT
# include <d3d11_4.h> // Used for ID3D11Device5 / ID3D11DeviceContext4 / ID3D11Fence
#else
# ifdef DEFINE_TEXTURE_LAYOUT
#  include <d3d11_3.h> // Used for ID3D11Device3
# else
#  include <d3d11_1.h> // Used for ID3D11Device1
# endif // DEFINE_TEXTURE_LAYOUT
#endif // TEST_SEMAPHORE_IMPORT

#include <limits>

#include <iostream> // for std::cout, std::cerr
#include <sstream>  // for std::ostringstream
#include <iomanip>  // for std::setw
#include <format>   // for std::format

using namespace dx11_interop;

namespace syclexp = sycl::ext::oneapi::experimental;

using sycl_mem_handle_t = syclexp::image_memory_handle_type;
using sycl_img_desc_t   = syclexp::image_descriptor;
using sycl_unsamp_img_t = syclexp::unsampled_image_handle;

#ifdef DEFINE_TEXTURE_LAYOUT
using DX11TextureDesc = D3D11_TEXTURE2D_DESC1;
using DX11Texture     = ID3D11Texture2D1;
#else // DEFINE_TEXTURE_LAYOUT
using DX11TextureDesc = D3D11_TEXTURE2D_DESC;
using DX11Texture     = ID3D11Texture2D;
#endif // DEFINE_TEXTURE_LAYOUT

constexpr sycl_mem_handle_t sycl_opaque_mem = sycl_mem_handle_t::opaque_handle;

#define TEST_SMALL_IMAGE_SIZE

// #define VERBOSE_PRINT
#define DEBUG_IMG_COPY 0
#ifdef VERBOSE_PRINT
#define VERBOSE_DEBUG DEBUG_IMG_COPY
#else
#define VERBOSE_DEBUG 0
#endif

constexpr bool LineSep = true;

constexpr uint32_t DefltPrec = 2;    // Default print precision of floats.
constexpr uint32_t DefltCols = 480;  // Default print columns (in console).
constexpr uint32_t DefltRows = 256;  // Default print rows.

template<typename T>
constexpr uint32_t Precision = std::is_floating_point_v<T> ? DefltPrec : 0;

// This is a global counter to keep track of the number of verified tests.
static int TotalNumVerifiedTests = 0;

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
  std::cerr << "[SYCL_DX11_INTEROP] ENTER: printBuf(const InT *, uint32_t, ...)\n";
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

  std::cout << ' ';
  for (uint32_t x = 0, x_e = len; x_e--; ++x) {
    if (allX || x < halfW || x_e < halfW)
      std::cout << std::format("{:^{}d}", x, elemW + 1);
    else if (x == halfW) std::cout << "...  ";
  }
  std::cout << '\n';
  for (uint32_t x = 0, x_e = len; x_e--; ++x) {
    if (allX || x < halfW || x_e < halfW) {
      const char sep = (Sep || x == 0) ? '+' : '-';
      std::cout << std::format("{}{:->{}s}", sep, "-", elemW);
    }
    else if (x == halfW) std::cout << " ... ";
  }
  std::cout << "+\n";
  for (uint32_t c = 0; c < NChannels; ++c) {
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

  for (uint32_t x = 0, x_e = len; x_e--; ++x) {
    if (allX || x < halfW || x_e < halfW) {
      const char sep = (Sep || x == 0) ? '+' : '-';
      std::cout << std::format("{}{:->{}s}", sep, "-", elemW);
    }
    else if (x == halfW) std::cout << " ... ";
  }
  std::cout << "+\n\n";
  std::cerr << "[SYCL_DX11_INTEROP] LEAVE: printBuf(const InT *, uint32_t, ...)\n";
}
//----------------------------------------------------------------------------//
template<typename InT, uint32_t NChannels = 1, typename AsT = InT>
void printImg(const InT *data, const uint32_t wdth, const uint32_t hght,
              const uint32_t dpth = 1,
              const uint32_t prec = Precision<AsT>,
              const uint32_t cols = DefltCols,
              const uint32_t rows = DefltRows) {
  std::cerr << "[SYCL_DX11_INTEROP] ENTER: printImg(const InT *, uint32_t, uint32_t, ...)\n";
  const size_t rowVals = wdth * NChannels;

  constexpr uint32_t Last = NChannels - 1;
  constexpr uint32_t Half = Last / 2;
  constexpr AsT      Zero = static_cast<AsT>(0);
  constexpr bool     Sep  = LineSep && (NChannels > 1);

  std::cout << "\n========================================\n";
  if (dpth > 1) std::cout << "Image size: " << wdth << "(w) x " << hght << "(h) x " << dpth << "(d)\n";
  else          std::cout << "Image size: " << wdth << "(w) x " << hght << "(h)\n";
  std::cout << "# channels: " << NChannels << "\n";
  std::cout << "Data ptr  : " << data << "\n";

  AsT minVal = static_cast<AsT>(data[0]);
  AsT maxVal = minVal;
  for (uint32_t d = 0; d < dpth; ++d) {
    for (uint32_t y = 0; y < hght; ++y) {
        const InT *row = data + y * rowVals;
        for (uint32_t x = 0; x < rowVals; ++x) {
            AsT val = static_cast<AsT>(row[x]);
            if      (val > maxVal) maxVal = val;
            else if (val < minVal) minVal = val;
        }
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

  for (uint32_t d = 0; d < dpth; ++d) {
    const InT *img = data + d * rowVals * hght;
    if (dpth > 1) {
      std::cout << "----------------------------------------\n";
      std::cout << "Image: " << d << "\n";
    }
    std::cout << "     ";
    for (uint32_t x = 0, x_e = wdth; x_e--; ++x) {
        if (allX || x < halfW || x_e < halfW)
            std::cout << std::format("{:^{}d}", x, elemW + 1);
        else if (x == halfW) std::cout << "...  ";
    }
    std::cout << "\n    ";
    for (uint32_t x = 0, x_e = wdth; x_e--; ++x) {
        if (allX || x < halfW || x_e < halfW) {
            const char sep = (Sep || x == 0) ? '+' : '-';
            std::cout << std::format("{}{:->{}s}", sep, "-", elemW);
        }
        else if (x == halfW) std::cout << " ... ";
    }
    std::cout << "+\n";
    for (uint32_t y = 0, y_e = hght; y_e--; ++y) {
      if (allY || y < halfH || y_e < halfH) {
        const InT *row = img + y * rowVals;
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
        const char sep = (Sep || x == 0) ? '+' : '-';
        std::cout << std::format("{}{:->{}s}", sep, "-", elemW);
      }
      else if (x == halfW) std::cout << " ... ";
    }
    std::cout << "+\n\n";
  }
  if (dpth > 1) std::cout << "----------------------------------------\n";
  std::cerr << "[SYCL_DX11_INTEROP] LEAVE: printImg(const InT *, uint32_t, uint32_t, ...)\n";
}
//----------------------------------------------------------------------------//
template<uint32_t NChannels = 1>
inline void printImgU08(const uint8_t *data,
                        const uint32_t wdth, const uint32_t hght,
                        const uint32_t cols = DefltCols,
                        const uint32_t rows = DefltRows) {
  printImg<uint8_t, NChannels>(data, wdth, hght, 0, cols, rows);
}
//----------------------------------------------------------------------------//
template<uint32_t NChannels = 1>
inline void printImgU32(const uint32_t *data,
                        const uint32_t wdth, const uint32_t hght,
                        const uint32_t cols = DefltCols,
                        const uint32_t rows = DefltRows) {
  printImg<uint32_t, NChannels>(data, wdth, hght, 0, cols, rows);
}
//----------------------------------------------------------------------------//
template<uint32_t NChannels = 1>
inline void printImgS32(const  int32_t *data,
                        const uint32_t wdth, const uint32_t hght,
                        const uint32_t cols = DefltCols,
                        const uint32_t rows = DefltRows) {
  printImg<int32_t, NChannels>(data, wdth, hght, 0, cols, rows);
}
//----------------------------------------------------------------------------//
template<uint32_t NChannels = 1, typename AsT = float>
inline void printImgFlt(const float *data,
                        const uint32_t wdth, const uint32_t hght,
                        const uint32_t prec = DefltPrec,
                        const uint32_t cols = DefltCols,
                        const uint32_t rows = DefltRows) {
  printImg<float, NChannels, AsT>(data, wdth, hght, prec, cols, rows);
}
//----------------------------------------------------------------------------//
template<uint32_t NChannels = 1>
inline void printImgFltAsU32(const float *data,
                             const uint32_t wdth, const uint32_t hght,
                             const uint32_t prec = DefltPrec,
                             const uint32_t cols = DefltCols,
                             const uint32_t rows = DefltRows) {
  printImg<float, NChannels, uint32_t>(data, wdth, hght, prec, cols, rows);
}
//----------------------------------------------------------------------------//
/*
void printImg(const void *data, DXGI_FORMAT frmt,
              const uint32_t wdth, const uint32_t hght,
              const uint32_t prec = DefltPrec,
              const uint32_t cols = DefltCols,
              const uint32_t rows = DefltRows) {
  switch (frmt) {
    case DXGI_FORMAT_R8G8B8A8_UINT:
      return printImgU08<4>((const uint8_t *)data, wdth, hght, cols, rows);
    case DXGI_FORMAT_R8G8_UINT:
      return printImgU08<2>(data, wdth, hght, cols, rows);
    case DXGI_FORMAT_R8_UINT:
      return printImgU08<1>(data, wdth, hght, cols, rows);
    case DXGI_FORMAT_R32G32B32A32_UINT:
      return printImgU32<4>(data, wdth, hght, cols, rows);
    case DXGI_FORMAT_R32G32B32_UINT:
      return printImgU32<3>(data, wdth, hght, cols, rows);
    case DXGI_FORMAT_R32G32_UINT:
      return printImgU32<2>(data, wdth, hght, cols, rows);
    case DXGI_FORMAT_R32_UINT:
      return printImgU32<1>(data, wdth, hght, cols, rows);
    case DXGI_FORMAT_R32G32B32A32_SINT:
      return printImgS32<4>(data, wdth, hght, cols, rows);
    case DXGI_FORMAT_R32G32B32_SINT:
      return printImgS32<3>(data, wdth, hght, cols, rows);
    case DXGI_FORMAT_R32G32_SINT:
      return printImgS32<2>(data, wdth, hght, cols, rows);
    case DXGI_FORMAT_R32_SINT:
      return printImgS32<1>(data, wdth, hght, cols, rows);
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
      return printImgFlt<4>(data, wdth, hght, prec, cols, rows);
    case DXGI_FORMAT_R32G32B32_FLOAT:
      return printImgFlt<3>(data, wdth, hght, prec, cols, rows);
    case DXGI_FORMAT_R32G32_FLOAT:
      return printImgFlt<2>(data, wdth, hght, prec, cols, rows);
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
      return printImgFlt<1>(data, wdth, hght, prec, cols, rows);
    default:
      std::cerr << "Unsupport DirectX data format: " << frmt << "\n";
      break;
  }
}
*/
//----------------------------------------------------------------------------//
template <typename DType, int NChannels>
void populateD3D11Texture(DX11State &state, ID3D11Resource *pResource,
                          uint32_t width, uint32_t height, uint32_t depth,
                          DXGI_FORMAT format, const DType *data,
                          IDXGIKeyedMutex *keyedMutex) {
  assert(state.contxt);
  assert(keyedMutex);
  // There are more efficient ways than using UpdateSubresource (ie
  // Map/Unmap). However, this test application is not a realtime
  // performance-critical one, so this is good enough for our needs since we
  // aren't calling it in a loop.
  D3D11_BOX dstRegion;
  dstRegion.left = 0;
  dstRegion.right = width;
  dstRegion.top = 0;
  dstRegion.bottom = height;
  dstRegion.front = 0;
  dstRegion.back = 1;
  ThrowIfFailed(keyedMutex->AcquireSync(state.key++, INFINITE));
  const UINT rowPitch = width * NChannels * sizeof(DType);
  const UINT depthPitch = height * rowPitch;
  ID3D11DeviceContext *contxt = state.contxt;
  contxt->UpdateSubresource(pResource, 0, &dstRegion,
                            static_cast<const void *>(data),
                            rowPitch, depthPitch);
  ThrowIfFailed(keyedMutex->ReleaseSync(state.key));
}
//----------------------------------------------------------------------------//
sycl_unsamp_img_t syclImportTextureMem(HANDLE sharedHandle, size_t allocSize,
                                       const sycl_img_desc_t &imgDesc,
                                       sycl::queue queue) {
  // Import the memory from the shared handle into SYCL
  syclexp::external_mem_descriptor<syclexp::resource_win32_handle> extMemDesc{
      sharedHandle, syclexp::external_mem_handle_type::win32_nt_dx11_resource,
      allocSize};

  auto extMem = syclexp::import_external_memory(extMemDesc, queue);
  auto imgMem = syclexp::map_external_image_memory(extMem, imgDesc, queue);

  return syclexp::create_image(imgMem, imgDesc, queue);
}
//----------------------------------------------------------------------------//
#ifdef TEST_SEMAPHORE_IMPORT
syclexp::external_semaphore syclImportDX11FenceSemaphore(HANDLE sharedHandle,
                                                         sycl::queue queue) {
  std::cerr << "[SYCL_DX11_INTEROP] ENTER: syclImportDX11FenceSemaphore(HANDLE, sycl::queue)\n";
  syclexp::external_semaphore_descriptor<syclexp::resource_win32_handle>
      semDesc{sharedHandle,
              syclexp::external_semaphore_handle_type::win32_nt_dx11_fence};

  std::cout << "[SYCL_DX11_INTEROP]   syclImportDX11FenceSemaphore -- Calling import_external_semaphore.\n";
  auto ret = syclexp::import_external_semaphore(semDesc, queue);
  std::cerr << "[SYCL_DX11_INTEROP] LEAVE: syclImportDX11FenceSemaphore(HANDLE, sycl::queue)\n";
  return ret;
}
//----------------------------------------------------------------------------//
void waitD3D11Fence(ID3D11Fence *fence, UINT64 value, HANDLE eventHandle,
                    DWORD msecTimeout = INFINITE) {
  std::cerr << "[SYCL_DX11_INTEROP] ENTER: waitD3D11Fence(ID3D11Fence *, UINT64, ...)\n";
  ThrowIfFailed(fence->SetEventOnCompletion(value, eventHandle));
  if (WaitForSingleObject(eventHandle, msecTimeout) != WAIT_OBJECT_0) {
    throw std::runtime_error("Timed out waiting for D3D11 fence.");
  }
  std::cerr << "[SYCL_DX11_INTEROP] LEAVE: waitD3D11Fence(ID3D11Fence *, UINT64, ...)\n";
}
#endif // TEST_SEMAPHORE_IMPORT
//----------------------------------------------------------------------------//
template <int NDims, typename DType, int NChannels, DType scale>
void callSyclKernel(sycl::queue queue, sycl_unsamp_img_t imgHandle,
                    const sycl::range<NDims> &imgDims,
                    const sycl::range<NDims> &grpDims) {
  std::cerr << "[SYCL_DX11_INTEROP] ENTER: callSyclKernel(sycl::queue, sycl_unsamp_img_t, ...)\n";
  using VecT = sycl::vec<DType, NChannels>;
  using PixT = std::conditional_t<NChannels == 1, DType, VecT>;

  // constexpr DType two = static_cast<DType>(2);

  // sycl_unsamp_img_t imgHandle = syclImgHandle;

  try {
    // All we are doing is doubling the value of each pixel in the texture.
    auto e = queue.submit([&](sycl::handler &cgh) {
      sycl::stream str(786432, 64, cgh);
      cgh.parallel_for(
          sycl::nd_range<NDims>{imgDims, grpDims}, [=](sycl::nd_item<NDims> it) {
            if constexpr (NDims == 3) {
              size_t z = it.get_global_id(0);
              size_t y = it.get_global_id(1);
              size_t x = it.get_global_id(2);
              sycl::int3 pos(x, y, z);
              auto pix = syclexp::fetch_image<PixT>(imgHandle, pos);
              // pix *= two;
              pix *= scale;
              syclexp::write_image(imgHandle, pos, pix);
            } else if constexpr (NDims == 2) {
              size_t y  = it.get_global_id(0);
              size_t x  = it.get_global_id(1);
              size_t sy = it.get_global_range(0);
              size_t sx = it.get_global_range(1);
              const char *py = (y < 10 ? "  " : (y < 100 ? " " : ""));
              const char *px = (x < 10 ? "  " : (x < 100 ? " " : ""));
              sycl::int2 pos(x, y);
              auto pix = syclexp::fetch_image<PixT>(imgHandle, pos);
              size_t gli = it.get_global_linear_id();
              DType val = static_cast<DType>(-1);
              constexpr bool readOkay = true;
              if      constexpr (NChannels == 1) val = pix;
              else if constexpr (readOkay)       val = pix[0];
              const char *pgi = (gli < 10 ? "  " : (gli < 100 ? " " : ""));
              str << "(" << py << y << "/" << sy << "," << px << x << "/" << sx << "):"
                  << pgi << gli << "->" << val << sycl::endl;
              // pix *= two;
              pix *= scale;
              syclexp::write_image(imgHandle, pos, pix);
            } else {
              size_t x  = it.get_global_id(0);
              size_t sx = it.get_global_range(0);
              int pos = static_cast<int>(x);
              const char *px = (x < 10 ? "   " : (x < 100 ? "  " : (x < 1000 ? " " : "")));
              auto pix = syclexp::fetch_image<PixT>(imgHandle, pos);
              size_t gli = it.get_global_linear_id();
              DType val = static_cast<DType>(-1);
              constexpr bool readOkay = true;
              if      constexpr (NChannels == 1) val = pix;
              else if constexpr (readOkay)       val = pix[0];
              const char *pgi = (gli < 10 ? "   " : (gli < 100 ? "  " : (gli < 1000 ? " " : "")));
              str << "(" << px << x << "/" << sx << "):"
                  << pgi << gli << "->" << val << sycl::endl;
              // pix *= two;
              pix *= scale;
              syclexp::write_image(imgHandle, int(x), pix);
            }
          });
    });
#ifndef TEST_SEMAPHORE_IMPORT
    e.wait_and_throw();
#endif
  } catch (sycl::exception e) {
    std::cerr << "\tSYCL kernel submission error: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "\tSYCL kernel submission error." << std::endl;
  }
  std::cerr << "[SYCL_DX11_INTEROP] LEAVE: callSyclKernel(sycl::queue, sycl_unsamp_img_t, ...)\n";
}
//----------------------------------------------------------------------------//
template <typename DType, int NChannels>
bool verifyResult(DX11State &state, ID3D11Resource *pResource,
                  const DX11TextureDesc &texDesc, const DType *orig,
                  IDXGIKeyedMutex *keyedMutex) {
  std::cerr << "[SYCL_DX11_INTEROP] ENTER: verifyResult(DX11State &, ID3D11Resource *, ...)\n";
  assert(state.device && state.contxt);
  auto *pDevice = state.device;
  auto *pContxt = state.contxt;

  static constexpr UINT bindFlags = 0;
  static constexpr UINT miscFlags = 0;

  const uint32_t wdth = texDesc.Width;
  const uint32_t hght = texDesc.Height;
  const uint32_t dpth = texDesc.ArraySize;

  // Create the staging texture
  DX11TextureDesc stageDesc = texDesc;
  stageDesc.Usage          = D3D11_USAGE_STAGING;
  stageDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  stageDesc.BindFlags      = bindFlags;
  stageDesc.MiscFlags      = miscFlags;
  ComPtr<DX11Texture> stageTex;
# ifdef DEFINE_TEXTURE_LAYOUT
  ComPtr<ID3D11Device3> device3;
  ThrowIfFailed(pDevice->QueryInterface(IID_PPV_ARGS(&device3)));
  ThrowIfFailed(device3->CreateTexture2D1(&stageDesc, nullptr, &stageTex));
# else // DEFINE_TEXTURE_LAYOUT
  ThrowIfFailed(pDevice->CreateTexture2D(&stageDesc, nullptr, &stageTex));
# endif // DEFINE_TEXTURE_LAYOUT

  // Copy the texture subresource
  ThrowIfFailed(keyedMutex->AcquireSync(state.key++, INFINITE));
  pContxt->CopyResource(stageTex.Get(), pResource);
  ThrowIfFailed(keyedMutex->ReleaseSync(state.key));

  // Map the staging texture to CPU memory
  D3D11_MAPPED_SUBRESOURCE mappedResource;
  ZeroMemory(&mappedResource, sizeof(mappedResource));
  ThrowIfFailed(pContxt->Map(stageTex.Get(), 0, D3D11_MAP_READ, 0,
                             &mappedResource));
  auto data = reinterpret_cast<DType *>(mappedResource.pData);
  if (hght * dpth <= 1) printBuf<DType, NChannels>(data, wdth);
  else                  printImg<DType, NChannels>(data, wdth, hght, dpth);
  const size_t rowElems = static_cast<size_t>(NChannels) * wdth;
  const size_t bufSize  = rowElems * hght * dpth;
  const size_t rowPitch = mappedResource.RowPitch / sizeof(DType);
  bool mismatch = false;
  uint32_t offst = 0;
  for (size_t i = 0, idx = 0; i < bufSize; ++i, ++idx) {
    if (i != 0 && (i % rowElems == 0)) {
      offst += rowPitch;
      idx = offst; // Reset the buffer index to start from the offset location
    }
    auto actual = data[idx];
    auto expect = orig[i] * 2;
    if (actual != expect) {
      mismatch = true;
#ifdef VERBOSE_PRINT
      std::cerr << actual << " not matching " << expect << "\n";
      std::cerr << "Pixel value[" << idx << "] = "
                << static_cast<std::conditional_t<
                       std::is_integral_v<decltype(actual)>, int, float>>(actual)
                << "\n";
      std::cerr << "Expected value[" << i << "] = "
                << static_cast<std::conditional_t<
                       std::is_integral_v<decltype(expect)>, int, float>>(expect)
                << "\n";

      break;
#endif
    }
  }
  // Unmap the staging texture
  pContxt->Unmap(stageTex.Get(), 0);

  std::cerr << "[SYCL_DX11_INTEROP] LEAVE: verifyResult(DX11State &, ID3D11Resource *, ...)\n";
  return !mismatch;
}
//----------------------------------------------------------------------------//
/// @brief Runner for the DX11-SYCL memory interopability functionality.
/// @return 0 on success and 1 on failure
template <int NDims, typename DType, int NChannels>
int runTest(DX11State &state, sycl::queue queue,
            sycl::image_channel_type channelType,
            const Dims3D<NDims> &imgDims, const Dims3D<NDims> &grpDims) {
  std::cerr << "[SYCL_DX11_INTEROP] ENTER: runTest(DX11State &, sycl::queue, sycl::image_channel_type, ...)\n";
  assert(state.device && state.contxt);
  auto *pDevice = state.device;

  sycl_img_desc_t imgDesc{imgDims, NChannels, channelType};
  // Verify ability to allocate the above image descriptor.
  // E.g. LevelZero does not support `unorm` channel types.
  std::cout << "[SYCL_DX11_INTEROP]   runTest -- Checking memory allocation support\n";
  if (!bindless_helpers::memoryAllocationSupported(imgDesc, sycl_opaque_mem, queue) ||
      (channelType == sycl_unorm8 && queue.get_device().get_backend() ==
           sycl::backend::ext_oneapi_level_zero)) {
    // We cannot allocate the image memory, skip the test.
#ifdef VERBOSE_PRINT
    std::cout << "Memory allocation unsupported. Skipping test.\n";
#endif
    // Early-exit successfully since this is not an error.
    return 0;
  }

  // setup the texture dimensions and resource size.
  const uint32_t wdth = imgDims.wdth;
  const uint32_t hght = imgDims.hght;
  const uint32_t dpth = imgDims.dpth;

  DXGI_FORMAT texFormat = toDXGIFormat<NChannels>(channelType);

  // DirectX 11 does not allow us to specify a row major layout for 2D textures
  // that have ArraySize > 1 and we would like to specify it in order to
  // accurately calculate the allocation size for the texture so that we can
  // import it from SYCL side. Hence, in light of this restriction, instead of
  // using ArraySize > 1 to simulate 3D textures, we simulate them by simply
  // collapsing the depth dimension onto the height dimension and set ArraySize
  // to 1.
  // Create a shared texture
  ComPtr<DX11Texture> texture;
  // Initialize the texture description.
  // D3D11_TEXTURE2D_DESC1 texDesc{};
  DX11TextureDesc texDesc{};
  texDesc.Width          = wdth;
  texDesc.Height         = hght; // if height is 1, we can mimic sharing 1D mem
  texDesc.MipLevels      = 1;    // one mip level, so no sub-textures
  texDesc.ArraySize      = dpth; // array slices used for sharing 3D memory
  texDesc.Format         = texFormat;
  texDesc.SampleDesc     = {.Count = 1, .Quality = 0};
  texDesc.Usage          = D3D11_USAGE_DEFAULT;
  texDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
  // texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
  texDesc.CPUAccessFlags = 0;
  // Note: Direct3D 11 does not support the
  // D3D11_RESOURCE_MISC_SHARED_NTHANDLE flag for 3D or 1D textures. This flag
  // is mainly used for sharing resources between different D3D11 devices, but
  // it is only applicable to 2D textures.
  texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_NTHANDLE |
                      D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
# ifdef DEFINE_TEXTURE_LAYOUT
  texDesc.TextureLayout = D3D11_TEXTURE_LAYOUT_ROW_MAJOR;

  std::cout << "[SYCL_DX11_INTEROP]   runTest -- Extract ID3D11Device3 interface\n";
  ComPtr<ID3D11Device3> device3;
  HRESULT hr = pDevice->QueryInterface(IID_PPV_ARGS(&device3));
  if (FAILED(hr) || !device3) {
    std::cerr << "Failed to get ID3D11Device3 interface (HRESULT: 0x" << std::hex << hr << ").\n";
    return 1;
  }
  std::cout << "[SYCL_DX11_INTEROP]   runTest -- Create texture\n";
  hr = device3->CreateTexture2D1(&texDesc, nullptr, &texture);
  if (FAILED(hr)) {
    std::cerr << "Failed to create texture (HRESULT: 0x" << std::hex << hr << ").\n";
    return 1;
  }
# else // DEFINE_TEXTURE_LAYOUT
  std::cout << "[SYCL_DX11_INTEROP]   runTest -- Create texture\n";
  ThrowIfFailed(pDevice->CreateTexture2D(&texDesc, nullptr, &texture));
# endif // DEFINE_TEXTURE_LAYOUT

  // Create the keyed mutex for synchronising the shared resource.
  std::cout << "[SYCL_DX11_INTEROP]   runTest -- Extract keyed mutex interface from texture\n";
  ComPtr<IDXGIKeyedMutex> keyedMutex;
  ThrowIfFailed(texture.As(&keyedMutex));
  state.key = 0;

#ifdef TEST_SEMAPHORE_IMPORT
  ComPtr<ID3D11Device5> device5;
  std::cout << "[SYCL_DX11_INTEROP]   runTest -- Create ID3D11Device5 ComPtr.\n";
  ThrowIfFailed(pDevice->QueryInterface(IID_PPV_ARGS(&device5)));

  ComPtr<ID3D11DeviceContext4> context4;
  std::cout << "[SYCL_DX11_INTEROP]   runTest -- Create ID3D11DeviceContext4 ComPtr\n";
  ThrowIfFailed(state.contxt->QueryInterface(IID_PPV_ARGS(&context4)));

  ComPtr<ID3D11Fence> fence;
  uint64_t fenceVal = 0;
  std::cout << "[SYCL_DX11_INTEROP]   runTest -- Create ID3D11Fence ComPtr\n";
  ThrowIfFailed(device5->CreateFence(fenceVal, D3D11_FENCE_FLAG_SHARED,
                                     IID_PPV_ARGS(&fence)));

  HANDLE sharedFence = INVALID_HANDLE_VALUE;
  std::cout << "[SYCL_DX11_INTEROP]   runTest -- Create shared fence handle\n";
  ThrowIfFailed(fence->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr,
                                          &sharedFence));

  syclexp::external_semaphore syclSemaphore = syclImportDX11FenceSemaphore(sharedFence, queue);

  HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (fenceEvent == nullptr) {
    ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
  }
#endif

  // Create an NT handle to a shared resource referring to our texture.
  // Opening the shared resource gives access to it for use on the SYCL device.
  std::cout << "[SYCL_DX11_INTEROP]   runTest -- Create shared resource handle for texture\n";
  ComPtr<IDXGIResource1> sharedResource;
  ThrowIfFailed(texture.As(&sharedResource));
  HANDLE sharedHandle = nullptr;
  ThrowIfFailed(sharedResource->CreateSharedHandle(
      nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr,
      &sharedHandle));

  // Obtain a pointer to the shared resource for use in subsequent operations.
  std::cout << "[SYCL_DX11_INTEROP]   runTest -- Open shared resource\n";
# ifdef DEFINE_TEXTURE_LAYOUT
  ThrowIfFailed(device3->OpenSharedResource1(sharedHandle,
                                             IID_PPV_ARGS(sharedResource.GetAddressOf())));
# else // DEFINE_TEXTURE_LAYOUT
  ComPtr<ID3D11Device1> device1;
  ThrowIfFailed(pDevice->QueryInterface(IID_PPV_ARGS(&device1)));
  ThrowIfFailed(device1->OpenSharedResource1(sharedHandle,
                                             IID_PPV_ARGS(sharedResource.GetAddressOf())));
# endif // DEFINE_TEXTURE_LAYOUT

  // Populate the texture on the CPU
  std::cout << "[SYCL_DX11_INTEROP]   runTest -- Populate texture on CPU\n";
  std::vector<DType> srcData(wdth * hght * dpth * NChannels, 0);
  const DType *data = srcData.data();
  if (ComPtr<ID3D11Resource> resource; SUCCEEDED(texture.As(&resource))) {
    // Initialize the texture data to upload.
    auto srcVal = [&](int i) -> DType {
      if constexpr (std::is_integral_v<DType> ||
                    std::is_same_v<DType, sycl::half>) {
        i = i % (static_cast<uint64_t>(std::numeric_limits<DType>::max()) / 2);
      }
      return i;
    };
    for (int i = 0; i < srcData.size(); ++i) {
      srcData[i] = srcVal(i);
    }
    if constexpr (NDims == 1) printBuf<DType, NChannels>(data, wdth);
    else                      printImg<DType, NChannels>(data, wdth, hght, dpth);
    populateD3D11Texture<DType, NChannels>(state, resource.Get(), wdth, hght, dpth,
                                           texFormat, data, keyedMutex.Get());
  }

  // Unfortunately, DX11 does not expose the texture allocation information
  // like DX12, so we have to calculate it manually the best we can (no mips).
  // The fact that the texture has been requested to have a row major layout
  // should support this speculative calculation.  
  std::cout << "[SYCL_DX11_INTEROP]   runTest -- Populate texture on CPU\n";
  const size_t allocSize = imgDims.size() * NChannels * sizeof(DType);
  sycl_unsamp_img_t imgHandle = syclImportTextureMem(sharedHandle, allocSize,
                                                     imgDesc, queue);

  // Submit the SYCL kernel.
  // When IDXGIKeyedMutex importing into SYCL is implemented, we'll be able to
  // call it from the SYCL API. All it does is ensuring only one device has
  // exclusive access.
  ThrowIfFailed(keyedMutex->AcquireSync(state.key++, INFINITE));
#ifdef TEST_SEMAPHORE_IMPORT
  ThrowIfFailed(context4->Signal(fence.Get(), fenceVal));
  queue.ext_oneapi_wait_external_semaphore(syclSemaphore, fenceVal);
  fenceVal++;
#endif

  constexpr DType two = static_cast<DType>(2);

  std::cout << "[SYCL_DX11_INTEROP]   runTest -- Call SYCL kernel\n";
  // if constexpr (NDims == 1) {
    // constexpr DType one = static_cast<DType>(1);
    // callSyclKernel<NDims, DType, NChannels, one>(queue, imgHandle,
    //                                              imgDims.to_flip_range(),
    //                                              grpDims.to_flip_range());
    // std::cout << "----------------------------------------\n";
    callSyclKernel<NDims, DType, NChannels, two>(queue, imgHandle,
                                                 imgDims.to_flip_range(),
                                                 grpDims.to_flip_range());
  // } else {
  //   callSyclKernel<NDims, DType, NChannels, two>(queue, imgHandle,
  //                                                imgDims.to_flip_range(),
  //                                                grpDims.to_flip_range());
  // }

#ifdef TEST_SEMAPHORE_IMPORT
  queue.submit([&](sycl::handler &cgh) {
    cgh.ext_oneapi_signal_external_semaphore(syclSemaphore, fenceVal);
  });
  waitD3D11Fence(fence.Get(), fenceVal, fenceEvent);
  fenceVal++;
#endif

  // Back to the D3D11 process
  ThrowIfFailed(keyedMutex->ReleaseSync(state.key));

  // Read-back and verify
  int errc = 1;
  if (ComPtr<ID3D11Resource> resource; SUCCEEDED(texture.As(&resource))) {
    if (verifyResult<DType, NChannels>(state, resource.Get(),
                                       texDesc, data, keyedMutex.Get())) {
      errc = 0;
    }
  }

  // Cleanup of the shared handle.
  CloseNTHandle(sharedHandle);

#ifdef TEST_SEMAPHORE_IMPORT
  CloseNTHandle(sharedFence);
  CloseNTHandle(fenceEvent);
#endif

#ifdef VERBOSE_PRINT
  if (errc == 1) {
    std::cerr << "\tTest failed: NDims " << NDims << " NChannels " << NChannels
              << " image_channel_type "
              << bindless_helpers::channelTypeToString(channelType) << "\n";
  } else {
    std::cout << "\tTest passed: NDims " << NDims << " NChannels " << NChannels
              << " image_channel_type "
              << bindless_helpers::channelTypeToString(channelType) << "\n";
  }
#endif
  TotalNumVerifiedTests++;
  std::cerr << "[SYCL_DX11_INTEROP] LEAVE: runTest(DX11State &, sycl::queue, sycl::image_channel_type, ...) --> " << errc << "\n";
  return errc;
}
//----------------------------------------------------------------------------//
int main() {
  // Create SYCL queue, relying on SYCL device selection
  sycl::queue queue;
  sycl::device syclDevice = queue.get_device();

  // Initialize D3D11 and create DX11 programs state from the SYCL device
  DX11State state{syclDevice};

  // pause();

  int errors = 0;

  // Test 1D texture interop
// #ifdef TEST_SMALL_IMAGE_SIZE
//   const Dims3D<1> grid1D{1024};
// #else
//   const Dims3D<1> grid1D{4096};
// #endif
  // errors += runTest<1, uint32_t  , 1>(state, queue, sycl_uint32, grid1D, {64});
  // errors += runTest<1, uint8_t   , 4>(state, queue, sycl_unorm8, grid1D, {256});
  // errors += runTest<1, float     , 1>(state, queue, sycl_float , grid1D, {256});
  // errors += runTest<1, sycl::half, 2>(state, queue, sycl_half  , grid1D, {256});
  // errors += runTest<1, sycl::half, 4>(state, queue, sycl_half  , grid1D, {256});

  // Test 2D texture interop
#ifdef TEST_SMALL_IMAGE_SIZE
  const Dims3D<2> grid2D[] = {{64, 48}, {64, 64}, {64, 64},
                              {64, 64}, {64, 64}};
#else
  const Dims3D<2> grid2D[] = {{1024, 1024}, {1920, 1080}, {1920, 1080},
                              {1280,  720}, {1280,  720}};
#endif
  errors += runTest<2, uint32_t  , 1>(state, queue, sycl_uint32, grid2D[0], {16, 16});
  // errors += runTest<2, uint8_t   , 4>(state, queue, sycl_unorm8, grid2D[1], {16,  8});
  // errors += runTest<2, float     , 1>(state, queue, sycl_float , grid2D[2], {16,  8});
  // errors += runTest<2, sycl::half, 2>(state, queue, sycl_half  , grid2D[3], {16, 16});
  // errors += runTest<2, sycl::half, 4>(state, queue, sycl_half  , grid2D[4], {16, 16});

// Test 3D texture interop
// #ifdef TEST_SMALL_IMAGE_SIZE
//   const Dims3D<3> grid3D[] = {{64, 16, 4}, {64, 16, 4}, {64, 64, 4},
//                               {64, 64, 4}, {64, 64, 4}};
// #else
//   const Dims3D<3> grid3D[] = {{ 256, 256, 32}, {1920, 1080, 8}, {512, 256, 8},
//                               {1280, 720,  4}, {1280,  720, 4}};
// #endif
//   const Dims3D<3> grp3D[] = {{16, 16, 1}, {16,  8, 2}, {16,  8, 1},
//                              {16, 16, 1}, {16, 16, 1}};
//   errors += runTest<3, uint32_t  , 1>(state, queue, sycl_uint32, grid3D[0], grp3D[0]);
//   // errors += runTest<3, uint8_t   , 4>(state, queue, sycl_unorm8, grid3D[1], grp3D[1]);
//   errors += runTest<3, float     , 1>(state, queue, sycl_float , grid3D[2], grp3D[2]);
//   errors += runTest<3, sycl::half, 2>(state, queue, sycl_half  , grid3D[3], grp3D[3]);
//   errors += runTest<3, sycl::half, 4>(state, queue, sycl_half  , grid3D[4], grp3D[4]);

#ifdef VERBOSE_PRINT
  std::string deviceName = syclDevice.get_info<sycl::info::device::name>();
  std::cout << "Tests pass rate for SYCL device: " << deviceName << "\n";
  const auto numPassedTests = (TotalNumVerifiedTests - errors);
  std::cerr << ((errors > 0) ? errors : numPassedTests) << " out of "
            << TotalNumVerifiedTests << " tested configurations were "
            << ((errors > 0) ? "unsuccessful" : "successful") << ".\n";
#endif

  return errors;
}
//----------------------------------------------------------------------------//
