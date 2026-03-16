#pragma once
#include <random>
#include <sycl/detail/core.hpp>
#include <sycl/ext/oneapi/bindless_images.hpp>
#include <sycl/image.hpp>  // for sycl::image_channel_type, sycl::image_channel_order
#include <sycl/range.hpp>  // for sycl::range

using sycl_channel_t = sycl::image_channel_type;
using sycl_color     = sycl::image_channel_order;

// clang-format off
// Some commonly used channel types
constexpr sycl_channel_t sycl_snorm8   = sycl_channel_t::snorm_int8;
constexpr sycl_channel_t sycl_snorm16  = sycl_channel_t::snorm_int16;
constexpr sycl_channel_t sycl_unorm8   = sycl_channel_t::unorm_int8;
constexpr sycl_channel_t sycl_unorm16  = sycl_channel_t::unorm_int16;
constexpr sycl_channel_t sycl_unorm565 = sycl_channel_t::unorm_short_565;
constexpr sycl_channel_t sycl_unorm555 = sycl_channel_t::unorm_short_555;
constexpr sycl_channel_t sycl_un101010 = sycl_channel_t::unorm_int_101010;
constexpr sycl_channel_t sycl_sint8    = sycl_channel_t::signed_int8;
constexpr sycl_channel_t sycl_sint16   = sycl_channel_t::signed_int16;
constexpr sycl_channel_t sycl_sint32   = sycl_channel_t::signed_int32;
constexpr sycl_channel_t sycl_uint8    = sycl_channel_t::unsigned_int8;
constexpr sycl_channel_t sycl_uint16   = sycl_channel_t::unsigned_int16;
constexpr sycl_channel_t sycl_uint32   = sycl_channel_t::unsigned_int32;
constexpr sycl_channel_t sycl_half     = sycl_channel_t::fp16;
constexpr sycl_channel_t sycl_float    = sycl_channel_t::fp32;

constexpr sycl_color sycl_a    = sycl_color::a;
constexpr sycl_color sycl_r    = sycl_color::r;
constexpr sycl_color sycl_rx   = sycl_color::rx;
constexpr sycl_color sycl_rg   = sycl_color::rg;
constexpr sycl_color sycl_rgx  = sycl_color::rgx;
constexpr sycl_color sycl_ra   = sycl_color::ra;
constexpr sycl_color sycl_rgb  = sycl_color::rgb;
constexpr sycl_color sycl_rgbx = sycl_color::rgbx;
constexpr sycl_color sycl_rgba = sycl_color::rgba;
constexpr sycl_color sycl_argb = sycl_color::argb;
constexpr sycl_color sycl_bgra = sycl_color::bgra;
constexpr sycl_color sycl_abgr = sycl_color::abgr;

template <typename T> inline constexpr uint32_t to_s32 (T val) { return static_cast< int32_t>(val); }
template <typename T> inline constexpr uint32_t to_u32 (T val) { return static_cast<uint32_t>(val); }
template <typename T> inline constexpr uint64_t to_u64 (T val) { return static_cast<uint64_t>(val); }
template <typename T> inline constexpr size_t   to_size(T val) { return static_cast<  size_t>(val); }
template <typename T> inline constexpr float    to_flt (T val) { return static_cast<   float>(val); }
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

  // Construct from Vulkan VkExtent3D
  // Dims3D(const VkExtent3D &ext) : wdth{ext.width}, hght{ext.height},
  //                                 dpth{ext.depth} {}

  // clang-format off
  // Compute total number of elements.
  size_t size() const {
    if      constexpr (Dims == 1) return to_size(wdth);
    else if constexpr (Dims == 2) return to_size(wdth) * hght;
    return to_size(wdth) * hght * dpth;
  } 
  size_t num_elems() const { return size(); }

  // Convert to Vulkan VkExtent3D
  // operator VkExtent3D() const { return {wdth, hght, dpth}; }

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

template <typename DType, int NChannels>
std::ostream &operator<<(std::ostream &os,
                         const sycl::vec<DType, NChannels> &vec) {
  static_assert(NChannels >= 1);
  std::string str{std::to_string(vec[0])};
  for (int i = 1; i < NChannels; ++i) {
    str += "," + std::to_string(vec[i]);
  }
  return os << str;
}

namespace bindless_helpers {

namespace syclexp = sycl::ext::oneapi::experimental;

bool memoryAllocationSupported(syclexp::image_descriptor &imgDesc,
                               syclexp::image_memory_handle_type memHandleType,
                               sycl::queue &syclQueue) {
// #ifdef VERBOSE_PRINT
//   std::cerr << "  Checking if memory allocation is supported for: " << static_cast<unsigned int>(memHandleType) << "\n";
// #endif
  auto supportedMemTypes =
      syclexp::get_image_memory_support(imgDesc, syclQueue);
// #ifdef VERBOSE_PRINT
//   std::cerr << "  Supported memory handle types: " << supportedMemTypes.size()
//             << " types\n";
//   for (const auto &type : supportedMemTypes) {
//     std::cerr << "    - " << static_cast<unsigned int>(type) << "\n";
//   }
// #endif
  return std::find(supportedMemTypes.begin(), supportedMemTypes.end(),
                   memHandleType) != supportedMemTypes.end();
}

template <int NDims>
static void printTestName(std::string name, sycl::range<NDims> globalSize,
                          sycl::range<NDims> localSize) {
#if defined(VERBOSE_LV1) || defined(VERBOSE_LV2) || defined(VERBOSE_LV3)
  std::cout << name << "\n";
  std::cout << "Global Size: ";

  for (int i = 0; i < NDims; i++) {
    std::cout << globalSize[i] << " ";
  }

  std::cout << " Local Size: ";

  for (int i = 0; i < NDims; i++) {
    std::cout << localSize[i] << " ";
  }

  std::cout << "\n";
#endif
}

const char *channelTypeToString(sycl::image_channel_type type) {
  switch (type) {
  case sycl::image_channel_type::snorm_int8:
    return "sycl::image_channel_type::snorm_int8";
  case sycl::image_channel_type::snorm_int16:
    return "sycl::image_channel_type::snorm_int16";
  case sycl::image_channel_type::unorm_int8:
    return "sycl::image_channel_type::unorm_int8";
  case sycl::image_channel_type::unorm_int16:
    return "sycl::image_channel_type::unorm_int16";
  case sycl::image_channel_type::unorm_short_565:
    return "sycl::image_channel_type::unorm_short_565";
  case sycl::image_channel_type::unorm_short_555:
    return "sycl::image_channel_type::unorm_short_555";
  case sycl::image_channel_type::unorm_int_101010:
    return "sycl::image_channel_type::unorm_int_101010";
  case sycl::image_channel_type::signed_int8:
    return "sycl::image_channel_type::signed_int8";
  case sycl::image_channel_type::signed_int16:
    return "sycl::image_channel_type::signed_int16";
  case sycl::image_channel_type::signed_int32:
    return "sycl::image_channel_type::signed_int32";
  case sycl::image_channel_type::unsigned_int8:
    return "sycl::image_channel_type::unsigned_int8";
  case sycl::image_channel_type::unsigned_int16:
    return "sycl::image_channel_type::unsigned_int16";
  case sycl::image_channel_type::unsigned_int32:
    return "sycl::image_channel_type::unsigned_int32";
  case sycl::image_channel_type::fp16:
    return "sycl::image_channel_type::fp16";
  case sycl::image_channel_type::fp32:
    return "sycl::image_channel_type::fp32";
  default:
    std::cerr << "Unsupported image_channel_type in channelTypeToString\n";
    exit(-1);
  }
}

template <typename DType, int NChannel>
constexpr sycl::vec<DType, NChannel> init_vector(DType val) {
  if constexpr (NChannel == 1) {
    return sycl::vec<DType, NChannel>{val};
  } else if constexpr (NChannel == 2) {
    return sycl::vec<DType, NChannel>{val, val};
  } else if constexpr (NChannel == 3) {
    return sycl::vec<DType, NChannel>{val, val, val};
  } else if constexpr (NChannel == 4) {
    return sycl::vec<DType, NChannel>{val, val, val, val};
  } else {
    std::cerr << "Unsupported number of channels " << NChannel << "\n";
    exit(-1);
  }
}

template <typename DType, int NChannels>
bool equal_vec(sycl::vec<DType, NChannels> v1, sycl::vec<DType, NChannels> v2) {
  for (int i = 0; i < NChannels; ++i) {
    if (v1[i] != v2[i]) {
      return false;
    }
  }
  return true;
}

template <typename T>
static void fill_rand(std::vector<T> &v,
                      int seed = std::default_random_engine::default_seed) {
  assert(!v.empty());
  using DType = sycl::detail::get_elem_type_t<T>;
  constexpr int NChannels = sycl::detail::get_vec_size<T>::size;
  std::default_random_engine generator;
  generator.seed(seed);
  auto distribution = [&]() {
    if constexpr (std::is_same_v<DType, sycl::half>) {
      return std::uniform_real_distribution<float>(0.0, 100.0);
    } else if constexpr (std::is_floating_point_v<DType>) {
      return std::uniform_real_distribution<DType>(0.0, 100.0);
    } else if constexpr (sizeof(DType) == 1) {
      return std::uniform_int_distribution<unsigned short>(0, 100);
    } else {
      return std::uniform_int_distribution<DType>(0, 100);
    }
  }();
  for (int i = 0; i < v.size(); ++i) {
    T temp;
    if constexpr (NChannels == 1) {
      temp = static_cast<DType>(distribution(generator));
    } else {
      for (int j = 0; j < NChannels; ++j)
        temp[j] = static_cast<DType>(distribution(generator));
    }

    v[i] = temp;
  }
}

template <typename DType, int NChannels>
static void add_host(const std::vector<sycl::vec<DType, NChannels>> &in_0,
                     const std::vector<sycl::vec<DType, NChannels>> &in_1,
                     std::vector<sycl::vec<DType, NChannels>> &out) {
  for (int i = 0; i < out.size(); ++i) {
    for (int j = 0; j < NChannels; ++j) {
      out[i][j] = in_0[i][j] + in_1[i][j];
    }
  }
}

template <typename DType, int NChannels,
          typename = std::enable_if_t<NChannels == 1>>
static DType add_kernel(const DType in_0, const DType in_1) {
  return in_0 + in_1;
}

template <typename DType, int NChannels,
          typename = std::enable_if_t<(NChannels > 1)>>
static sycl::vec<DType, NChannels>
add_kernel(const sycl::vec<DType, NChannels> &in_0,
           const sycl::vec<DType, NChannels> &in_1) {
  sycl::vec<DType, NChannels> out;
  for (int i = 0; i < NChannels; ++i) {
    out[i] = in_0[i] + in_1[i];
  }
  return out;
}

template <int NDims>
static constexpr sycl::range<NDims> reverse_dims(sycl::range<NDims> input) {
  if constexpr (NDims == 3) {
    return sycl::range<NDims>(input[2], input[1], input[0]);
  } else if constexpr (NDims == 2) {
    return sycl::range<NDims>(input[1], input[0]);
  } else { // NDims == 1
    return input;
  }
}

template <int NDims> struct ImageArrayDims {
  template <int Dims = NDims, typename = std::enable_if_t<Dims == 3>>
  ImageArrayDims(sycl::range<3> dims) : array_count(dims[2]) {
    array_dims[0] = dims[0];
    array_dims[1] = dims[1];
  }

  template <int Dims = NDims, typename = std::enable_if_t<Dims == 2>>
  ImageArrayDims(sycl::range<2> dims) : array_count(dims[1]) {
    array_dims[0] = dims[0];
  }

  sycl::range<NDims - 1> array_dims;
  unsigned int array_count;
};

template <int NDims> static sycl::range<NDims> getGlobalSize(size_t index) {

  const std::vector<sycl::range<1>> globalSizes1D = {{32}, {16}, {20},
                                                     {9},  {14}, {2}};
  const std::vector<sycl::range<2>> globalSizes2D = {{32, 16}, {8, 32}, {20, 5},
                                                     {3, 9},   {14, 7}, {2, 2}};
  const std::vector<sycl::range<3>> globalSizes3D = {
      {16, 8, 4}, {2, 6, 12}, {10, 15, 5}, {9, 6, 3}, {15, 7, 3}, {2, 2, 2}};

  const size_t globalIndex = index % 6;

  if constexpr (NDims == 1) {
    return {globalSizes1D[globalIndex]};
  }

  if constexpr (NDims == 2) {
    return {globalSizes2D[globalIndex]};
  }

  if constexpr (NDims == 3) {
    return {globalSizes3D[globalIndex]};
  }
}

template <int NDims> static sycl::range<NDims> getLocalSize(size_t index) {

  const std::vector<sycl::range<1>> localSizes1D = {{2}, {16}, {5},
                                                    {3}, {7},  {1}};
  const std::vector<sycl::range<2>> localSizes2D = {{16, 4}, {2, 32}, {5, 5},
                                                    {3, 3},  {7, 7},  {1, 1}};
  const std::vector<sycl::range<3>> localSizes3D = {
      {8, 4, 2}, {1, 3, 12}, {5, 5, 5}, {3, 3, 3}, {5, 7, 3}, {1, 1, 1}};

  const size_t localIndex = index % 6;

  if constexpr (NDims == 1) {
    return localSizes1D[localIndex];
  }

  if constexpr (NDims == 2) {
    return localSizes2D[localIndex];
  }

  if constexpr (NDims == 3) {
    return localSizes3D[localIndex];
  }
}

}; // namespace bindless_helpers
