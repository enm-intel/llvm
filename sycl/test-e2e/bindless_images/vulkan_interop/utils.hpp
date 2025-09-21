#pragma once

#include <sycl/range.hpp>  // for sycl::range
#include <sycl/image.hpp>  // for sycl::image_channel_type, sycl::image_channel_order
#include <vulkan/vulkan.h> // for VkExtent3D

namespace sycl_vulkan_img_utils {

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

  // Construct from Vulkan VkExtent3D
  Dims3D(const VkExtent3D &ext) : wdth{ext.width}, hght{ext.height},
                                  dpth{ext.depth} {}

  // clang-format off
  // Compute total number of elements.
  size_t size() const {
    if      constexpr (Dims == 1) return static_cast<size_t>(wdth);
    else if constexpr (Dims == 2) return static_cast<size_t>(wdth) * hght;
    return static_cast<size_t>(wdth) * hght * dpth;
  } 
  size_t num_elems() const { return size(); }

  // Convert to Vulkan VkExtent3D
  operator VkExtent3D() const { return {wdth, hght, dpth}; }

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
};

} // namespace sycl_vulkan_img_utils
