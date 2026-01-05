#ifndef DX_INTEROP_COMMON_HPP
#define DX_INTEROP_COMMON_HPP

// Reduce the size of Win32 header files
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// Define NOMINMAX to enable compilation on Windows
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// Windows Runtime C++ Template Library (ComPtr and friends)
#include <wrl.h>

// Direct3D 11 and 12 API headers.
#include <d3d11.h>
#include <d3d12.h>

// DXGI 1.6 is included in Windows 10 as part of the Creators Update (0x0A00).
#include <dxgi1_6.h>

// Required for the DXGI debug layer (for IDXGIFactoy).
#include <dxgidebug.h>

#include <sycl/detail/core.hpp>

using Microsoft::WRL::ComPtr;

namespace dx_helpers {

enum class dx_version { DX11, DX12 };

inline std::string ResultToString(HRESULT result) {
  char s_str[64] = {};
  sprintf_s(s_str, "Error result == 0x%08X", static_cast<uint32_t>(result));
  return std::string(s_str);
}

inline void ThrowIfFailed(HRESULT result) {
  if (result != S_OK) {
    throw std::runtime_error(ResultToString(result));
  }
}

void CloseNTHandle(HANDLE handle) {
  assert(handle);
  const bool valid = handle != NULL && handle != INVALID_HANDLE_VALUE;
  if (valid) {
    if (!CloseHandle(handle)) {
      throw std::runtime_error("Error closing the shared NT handle.");
    }
  } else {
    throw std::runtime_error("Error trying to close an invalid NT handle.");
  }
}

template <uint32_t NChannels>
DXGI_FORMAT toDXGIFormat(sycl::image_channel_type channelType) {
  switch (channelType) {
  case sycl::image_channel_type::snorm_int8:
    switch (NChannels) {
    case 1:
      return DXGI_FORMAT_R8_SNORM;
    case 2:
      return DXGI_FORMAT_R8G8_SNORM;
    case 4:
      return DXGI_FORMAT_R8G8B8A8_SNORM;
    default:
      break;
    }
  case sycl::image_channel_type::snorm_int16:
    switch (NChannels) {
    case 1:
      return DXGI_FORMAT_R16_SNORM;
    case 2:
      return DXGI_FORMAT_R16G16_SNORM;
    case 4:
      return DXGI_FORMAT_R16G16B16A16_SNORM;
    default:
      break;
    }
  case sycl::image_channel_type::unorm_int8:
    switch (NChannels) {
    case 1:
      return DXGI_FORMAT_R8_UNORM;
    case 2:
      return DXGI_FORMAT_R8G8_UNORM;
    case 4:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
    default:
      break;
    }
  case sycl::image_channel_type::unorm_int16:
    switch (NChannels) {
    case 1:
      return DXGI_FORMAT_R16_UNORM;
    case 2:
      return DXGI_FORMAT_R16G16_UNORM;
    case 4:
      return DXGI_FORMAT_R16G16B16A16_UNORM;
    default:
      break;
    }
  case sycl::image_channel_type::unorm_short_565:
    return DXGI_FORMAT_B5G6R5_UNORM;
  case sycl::image_channel_type::unorm_short_555:
    return DXGI_FORMAT_B5G5R5A1_UNORM;
  case sycl::image_channel_type::unorm_int_101010:
    return DXGI_FORMAT_R10G10B10A2_UNORM;
  case sycl::image_channel_type::signed_int8:
    switch (NChannels) {
    case 1:
      return DXGI_FORMAT_R8_SINT;
    case 2:
      return DXGI_FORMAT_R8G8_SINT;
    case 4:
      return DXGI_FORMAT_R8G8B8A8_SINT;
    default:
      break;
    }
  case sycl::image_channel_type::signed_int16:
    switch (NChannels) {
    case 1:
      return DXGI_FORMAT_R16_SINT;
    case 2:
      return DXGI_FORMAT_R16G16_SINT;
    case 4:
      return DXGI_FORMAT_R16G16B16A16_SINT;
    default:
      break;
    }
  case sycl::image_channel_type::signed_int32:
    switch (NChannels) {
    case 1:
      return DXGI_FORMAT_R32_SINT;
    case 2:
      return DXGI_FORMAT_R32G32_SINT;
    case 4:
      return DXGI_FORMAT_R32G32B32A32_SINT;
    default:
      break;
    }
  case sycl::image_channel_type::unsigned_int8:
    switch (NChannels) {
    case 1:
      return DXGI_FORMAT_R8_UINT;
    case 2:
      return DXGI_FORMAT_R8G8_UINT;
    case 4:
      return DXGI_FORMAT_R8G8B8A8_UINT;
    default:
      break;
    }
  case sycl::image_channel_type::unsigned_int16:
    switch (NChannels) {
    case 1:
      return DXGI_FORMAT_R16_UINT;
    case 2:
      return DXGI_FORMAT_R16G16_UINT;
    case 4:
      return DXGI_FORMAT_R16G16B16A16_UINT;
    default:
      break;
    }
  case sycl::image_channel_type::unsigned_int32:
    switch (NChannels) {
    case 1:
      return DXGI_FORMAT_R32_UINT;
    case 2:
      return DXGI_FORMAT_R32G32_UINT;
    case 4:
      return DXGI_FORMAT_R32G32B32A32_UINT;
    default:
      break;
    }
  case sycl::image_channel_type::fp16:
    switch (NChannels) {
    case 1:
      return DXGI_FORMAT_R16_FLOAT;
    case 2:
      return DXGI_FORMAT_R16G16_FLOAT;
    case 4:
      return DXGI_FORMAT_R16G16B16A16_FLOAT;
    default:
      break;
    }
  case sycl::image_channel_type::fp32:
    switch (NChannels) {
    case 1:
      return DXGI_FORMAT_R32_FLOAT;
    case 2:
      return DXGI_FORMAT_R32G32_FLOAT;
    case 4:
      return DXGI_FORMAT_R32G32B32A32_FLOAT;
    default:
      break;
    }
  default:
    break;
  }
  std::cerr << "Unsupported image_channel_type in toDXGIFormat\n";
  exit(-1);
}

namespace detail {

bool isDXGIDebugLayerEnabled(IDXGIFactory1 *pFactory) {
  ComPtr<IDXGIFactory3> factory3;
  if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory3)))) {
    return (factory3->GetCreationFlags() & DXGI_CREATE_FACTORY_DEBUG) != 0;
  }
  return false;
}

/// @brief  This helper function does not output a device. It just tests if the
///         creating a logical device from the chosen adapter parameters works.
template <dx_version dxVer>
bool canCreateDxDevice(IDXGIAdapter1 *pAdapter, bool withDebugDevice) {
  // We don't need any device features of the Direct3D API beyond that version.
  static constexpr D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_0;
  if constexpr (dxVer == dx_version::DX12) {
    // Check to see if the adapter supports Direct3D 12, but don't create
    // the actual device yet. All Direct3D 12 drivers support at least
    // feature level 11_0, making it a safe and reliable choice for device
    // creation. It is also perfectly sufficient for our interopability
    // feature testing purposes.
    if (SUCCEEDED(D3D12CreateDevice(pAdapter, minFeatureLevel,
                                    _uuidof(ID3D12Device), nullptr))) {
      return true;
    }
  } else {
    // Check to see if the adapter supports Direct3D 11, but don't create
    // the actual device yet.
    ComPtr<ID3D11Device> device = nullptr;
    ComPtr<ID3D11DeviceContext> deviceContext = nullptr;
    UINT deviceFlags = withDebugDevice ? D3D11_CREATE_DEVICE_DEBUG : 0;
    D3D_FEATURE_LEVEL outFeatureLevel;
    // We don't want SW renderer. Ideally we specify
    // D3D_DRIVER_TYPE_HARDWARE on creation but when we specify an Adapter
    // we need to specify the D3D_DRIVER_TYPE_UNKNOWN enum, otherwise the
    // call fails. This is okay as we made sure to select the best possible
    // HW adapter. We can create Device without a DXGIAdapter but we want
    // one to exist, so we can use it for LUID matching with the SYCL device
    // for interop.
    HRESULT Result =
        D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                          deviceFlags, &minFeatureLevel, 1u, D3D11_SDK_VERSION,
                          &device, &outFeatureLevel, &deviceContext);
    if (SUCCEEDED(Result)) {
      assert(outFeatureLevel == minFeatureLevel); // sanity check
      return true;
    }
  }
  return false;
}

} // namespace detail

std::string getD3DDeviceName(const DXGI_ADAPTER_DESC1 &adapterDesc) {
  // Convert the description string to a narrow string.
  // This conversion is a little imperfect due to consideration for locale etc.
  std::string name{};
  std::wstring wstrDescription = adapterDesc.Description;
  std::transform(std::begin(wstrDescription), std::end(wstrDescription),
                 std::back_inserter(name),
                 [](wchar_t c) { return static_cast<char>(c); });
  return name;
}

/// @brief  This is a helper function to find an appropriate hardware adapter.
/// It does not create the final D3D device but rather tests creating it with
/// the chosen hardware adapter, so it can be used safely for SYCL interop.
/// This function has no thread-safety guarantees in its current state.
/// Ideally we will not be searching for the highest performing adapter but for
/// the one that specifically matches the SYCL device for interopability. For
/// that reason we will need to introduce an LUID device query extension first.
template <dx_version dxVer>
ComPtr<IDXGIAdapter1> getDXGIHardwareAdapter(IDXGIFactory1 *pFactory,
                                             std::string_view syclDeviceName) {
  assert(pFactory);
  ComPtr<IDXGIAdapter1> adapter;

  // It is not necessary to tie the DXGI debug layer with the Direct3D 11 or 12
  // device debug layer, but for the purposes of our tests and this abstracted
  // functionality, it makes things easier to maintain so you just need to flip
  // one switch and get access to the full debugging utilities of DirectX.
  const bool withDebugDevice = detail::isDXGIDebugLayerEnabled(pFactory);

  bool foundAdapter = false;
  std::string adapterName;
  if (ComPtr<IDXGIFactory6> factory6;
      SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6)))) {
    for (UINT adapterIndex = 0; SUCCEEDED(factory6->EnumAdapterByGpuPreference(
             adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
             IID_PPV_ARGS(&adapter)));
         ++adapterIndex) {
      if (!adapter) {
        continue;
      }

      DXGI_ADAPTER_DESC1 desc;
      ZeroMemory(&desc, sizeof(desc));
      if (FAILED(adapter->GetDesc1(&desc))) {
        continue;
      }

      // We don't want the Microsoft Basic Render Driver (software) adapter.
      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
        continue;
      }

      adapterName = getD3DDeviceName(desc);
#ifdef VERBOSE_PRINT
      std::cout << "Considering D3D device: " << adapterName << std::endl;
#endif
      // Try matching SYCL device name with D3D device name
      // TODO: This should be replaced by LUID matching
      if ((adapterName.find(syclDeviceName) == std::string::npos) &&
          (syclDeviceName.find(adapterName) == std::string::npos)) {
        continue;
      }

      // Test if we can successfully create a device with this adapter.
      if (!detail::canCreateDxDevice<dxVer>(adapter.Get(), withDebugDevice)) {
        continue;
      }

      foundAdapter = true;
      break;
    }
  }

  if (!foundAdapter) {
    std::cerr << "Could not find the requested DirectX hardware adapter for "
                 "SYCL device: "
              << syclDeviceName << std::endl;
  } else {
    std::cout << "Initialized D3D adapter: " << adapterName << std::endl;
  }
  return adapter;
}

} // namespace dx_helpers


#include <sycl/range.hpp>  // for sycl::range
#include <sycl/image.hpp>  // for sycl::image_channel_type, sycl::image_channel_order

namespace sycl_dx_img_utils {

// clang-format off
// Some commonly used channel types
constexpr sycl::image_channel_type sycl_unorm8 = sycl::image_channel_type::unorm_int8;
constexpr sycl::image_channel_type sycl_sint8  = sycl::image_channel_type::signed_int8;
constexpr sycl::image_channel_type sycl_half   = sycl::image_channel_type::fp16;
constexpr sycl::image_channel_type sycl_sint16 = sycl::image_channel_type::signed_int16;
constexpr sycl::image_channel_type sycl_sint32 = sycl::image_channel_type::signed_int32;
constexpr sycl::image_channel_type sycl_uint32 = sycl::image_channel_type::unsigned_int32;
constexpr sycl::image_channel_type sycl_float  = sycl::image_channel_type::fp32;

constexpr sycl::image_channel_order sycl_r    = sycl::image_channel_order::r;
constexpr sycl::image_channel_order sycl_rg   = sycl::image_channel_order::rg;
constexpr sycl::image_channel_order sycl_rgb  = sycl::image_channel_order::rgb;
constexpr sycl::image_channel_order sycl_rgba = sycl::image_channel_order::rgba;

template <typename T> 
inline constexpr uint32_t to_u32(T val) { return static_cast<uint32_t>(val); }
// clang-format on

template <uint32_t Dims = 1>
class Dims3D {
private:
  static_assert(Dims >= 1 && Dims <= 3,
                "Dimensions must be only 1, 2, or 3.");

public:
  static constexpr int dimensions = Dims;
  using sycl_range_t = sycl::range<Dims>;

  uint32_t wdth{1};
  uint32_t hght{1};
  uint32_t dpth{1};

  Dims3D() = default;
  Dims3D(const Dims3D<Dims> &rhs) = default;
  Dims3D(Dims3D<Dims> &&rhs) = default;

  // The following constructor is only available when Dims==1
  template <int N = Dims>
  Dims3D(typename std::enable_if_t<(N == 1), uint32_t> wdth) :
      wdth{wdth}, hght{1}, dpth{1} {}

  // The following constructor is only available when Dims==2
  template <int N = Dims>
  Dims3D(typename std::enable_if_t<(N == 2), uint32_t> wdth, uint32_t hght) :
      wdth{wdth}, hght{hght}, dpth{1} {}

  // The following constructor is only available when Dims==3
  template <int N = Dims>
  Dims3D(typename std::enable_if_t<(N == 3), uint32_t> wdth, uint32_t hght,
         uint32_t dpth) : wdth{wdth}, hght{hght}, dpth{dpth} {}

  // clang-format off
  // Compute total number of elements.
  size_t size() const {
    if      constexpr (Dims == 1) return static_cast<size_t>(wdth);
    else if constexpr (Dims == 2) return static_cast<size_t>(wdth) * hght;
    return static_cast<size_t>(wdth) * hght * dpth;
  } 
  size_t num_elems() const { return size(); }

  // Convert to sycl::range<Dims>
  // If flip is true, the order of dimensions is reversed.
  template <bool flip = false>
  sycl_range_t to_sycl_range() const {
    if constexpr (Dims == 1) return sycl_range_t{wdth};
    else if constexpr (flip) {
      if constexpr (Dims == 2) return sycl_range_t{hght, wdth};
      else                     return sycl_range_t{dpth, hght, wdth};
    }
    else {
      if constexpr (Dims == 2) return sycl_range_t{wdth, hght};
      else                     return sycl_range_t{wdth, hght, dpth};
    }
  }
  // Convert to sycl::range<Dims> with flipped dimensions.
  sycl_range_t to_flip_range() const { return to_sycl_range<true>(); }
  // clang-format on

  operator sycl_range_t() const { return to_sycl_range(); }
};

} // namespace sycl_dx_img_utils

#endif // DX_INTEROP_COMMON_HPP
