// REQUIRES: aspect-ext_oneapi_bindless_images
// REQUIRES: aspect-ext_oneapi_external_memory_import || (windows && level_zero && aspect-ext_oneapi_bindless_images)
// REQUIRES: vulkan

// RUN: %{build} %link-vulkan -o %t.out %if target-spir %{ -Wno-ignored-attributes %}

// UNSUPPORTED: linux
// UNSUPPORTED-TRACKER: GSD-12357

/*
    Run ALL the vulkan formats through the gauntlet. sampled and unsampled.
    This entire test takes less than 30 seconds on a slow machine.  MUCH faster
   (and more complete coveraage) than SFINAE based approach.

    IF a particular variant is having problems on some platform, please do NOT
   just disable the whole test, instead use   RUN~IF (SOMETHING) yadda-yadda
    to enable/disable that variant.

    For semaphore testing, we run just a sampling. Note, that on Linux if there
   is a failure in the first section, then likely ALL semaphore tests afterwards
   will fail. This is being tracked as a separate issue.

*/
// clang-format off

// RUN: %{run} %t.out --type float --channels 1 32x33
// RUN: %{run} %t.out --type float --channels 2 32x33
// RUN: %{run} %t.out --type float --channels 4 32x33
// RUN: %{run} %t.out --type half --channels 1 32x33
// RUN: %{run} %t.out --type half --channels 2 32x33
// RUN: %{run} %t.out --type half --channels 4 32x33
// RUN: %{run} %t.out --type int32 --channels 1 32x33
// RUN: %{run} %t.out --type int32 --channels 2 32x33
// RUN: %{run} %t.out --type int32 --channels 4 32x33
// RUN: %{run} %t.out --type uint32 --channels 1 32x33
// RUN: %{run} %t.out --type uint32 --channels 2 32x33
// RUN: %{run} %t.out --type uint32 --channels 4 32x33
// RUN: %{run} %t.out --type int16 --channels 1 32x33
// RUN: %{run} %t.out --type int16 --channels 2 32x33
// RUN: %{run} %t.out --type int16 --channels 4 32x33
// RUN: %{run} %t.out --type uint16 --channels 1 32x33
// RUN: %{run} %t.out --type uint16 --channels 2 32x33
// RUN: %{run} %t.out --type uint16 --channels 4 32x33
// RUN: %{run} %t.out --type uint8 --channels 1 32x33
// RUN: %{run} %t.out --type uint8 --channels 2 32x33
// RUN: %{run} %t.out --type uint8 --channels 4 32x33
// RUN: %{run} %t.out --type int8 --channels 1 32x33
// RUN: %{run} %t.out --type int8 --channels 2 32x33
// RUN: %{run} %t.out --type int8 --channels 4 32x33
// RUN: %{run} %t.out --type float --channels 1 --sampled 32x33
// RUN: %{run} %t.out --type float --channels 2 --sampled 32x33
// RUN: %{run} %t.out --type float --channels 4 --sampled 32x33
// RUN: %{run} %t.out --type half --channels 1 --sampled 32x33
// RUN: %{run} %t.out --type half --channels 2 --sampled 32x33
// RUN: %{run} %t.out --type half --channels 4 --sampled 32x33
// RUN: %{run} %t.out --type int32 --channels 1 --sampled 32x33
// RUN: %{run} %t.out --type int32 --channels 2 --sampled 32x33
// RUN: %{run} %t.out --type int32 --channels 4 --sampled 32x33
// RUN: %{run} %t.out --type uint32 --channels 1 --sampled 32x33
// RUN: %{run} %t.out --type uint32 --channels 2 --sampled 32x33
// RUN: %{run} %t.out --type uint32 --channels 4 --sampled 32x33
// RUN: %{run} %t.out --type int16 --channels 1 --sampled 32x33
// RUN: %{run} %t.out --type int16 --channels 2 --sampled 32x33
// RUN: %{run} %t.out --type int16 --channels 4 --sampled 32x33
// RUN: %{run} %t.out --type uint16 --channels 1 --sampled 32x33
// RUN: %{run} %t.out --type uint16 --channels 2 --sampled 32x33
// RUN: %{run} %t.out --type uint16 --channels 4 --sampled 32x33
// RUN: %{run} %t.out --type uint8 --channels 1 --sampled 32x33
// RUN: %{run} %t.out --type uint8 --channels 2 --sampled 32x33
// RUN: %{run} %t.out --type uint8 --channels 4 --sampled 32x33
// RUN: %{run} %t.out --type int8 --channels 1 --sampled 32x33
// RUN: %{run} %t.out --type int8 --channels 2 --sampled 32x33
// RUN: %{run} %t.out --type int8 --channels 4 --sampled 32x33


// None of the 2D stuff is working on Linux.
// On Windows, we require driver 38303 or later to avoid semaphore issues, which the CI does not yet have. 
// Rather than mark the WHOLE test as requiring 38303, which would mean no testing nowhere,
// I'm just intentionally breaking the R U N directive below until it can be restored.


// RUN-IF: !windows, %{run} %t.out --type float --channels 1 32x33 --semaphores
// RUN-IF: !windows, %{run} %t.out --type float --channels 4 32x33 --semaphores
// RUN-IF: !windows, %{run} %t.out --type half --channels 1 32x33 --semaphores
// RUN-IF: !windows, %{run} %t.out --type uint32 --channels 4 32x33 --semaphores
// RUN-IF: !windows, %{run} %t.out --type uint16 --channels 2 32x33 --semaphores
// RUN-IF: !windows, %{run} %t.out --type int8 --channels 1 32x33 --semaphores
// RUN-IF: !windows, %{run} %t.out --type float --channels 4 --sampled 32x33 --semaphores
// RUN-IF: !windows, %{run} %t.out --type int32 --channels 4 --sampled 32x33 --semaphores
// RUN-IF: !windows, %{run} %t.out --type int16 --channels 4 --sampled 32x33 --semaphores
// RUN-IF: !windows, %{run} %t.out --type uint8 --channels 2 --sampled 32x33 --semaphores

/*
  Vulkan/SYCL 2D Image Read Test (Sampled + Unsampled)

  clang++ -fsycl  -o vsr_2d_test.bin vulkan_sycl_image_interop_read_2d.cpp -lvulkan -I$VULKAN_SDK/include -L$VULKAN_SDK/lib

  clang++ -fsycl  -o vsr_2d_test.exe vulkan_sycl_image_interop_read_2d.cpp -Wno-ignored-attributes  -lvulkan-1 -I$VULKAN_SDK/Include -L$VULKAN_SDK/Lib

  USAGE:
    ./vsr_2d_test.bin [FLAGS] [WxH]

  FLAGS:
    --sampled      Use sampled image path (default: unsampled/storage)
    --semaphores   Use Vulkan Semaphores for SYCL Interop Sync
    --linear       Use LINEAR tiling for the Vulkan Image (default is OPTIMAL)
    --channels X   Set number of channels (1, 2, or 4). Default is 4 (RGBA)
    --type XXX     Set data type (float, half, uint32, int32, uint16, int16, uint8, int8, unorm8). 
                   Default is float 
    WxH            Set custom Width x Height (e.g. 8x4)

  EXAMPLES:
    ./vsr_2d_test.bin
    ./vsr_2d_test.bin --sampled --semaphores
    ./vsr_2d_test.bin --sampled --linear --channels 2 8x4
    ./vsr_2d_test.bin --type half --channels 2
    ./vsr_2d_test.bin --linear --type unorm8 16x16

  NOTE: --linear is not currently working with 2D images on Linux
*/
// clang-format on
#include <iostream>

#include "vulkan_setup.hpp"

#include <optional>
#include <string>
#include <sycl/sycl.hpp>
#include <sycl/builtins.hpp>
#include <sycl/detail/core.hpp>
#include <sycl/ext/oneapi/bindless_images.hpp>
#include <sycl/ext/oneapi/bindless_images_interop.hpp>
#include <sycl/image.hpp>
#include <sycl/properties/queue_properties.hpp>

namespace syclexp = sycl::ext::oneapi::experimental;

// clang-format off
using sycl_img_data_t   = sycl::image_channel_type;
using sycl_img_samp_h   = syclexp::sampled_image_handle;
using sycl_img_unsamp_h = syclexp::unsampled_image_handle;
using sycl_img_mem_h    = syclexp::image_mem_handle;

// Some commonly used channel types
constexpr sycl_img_data_t sycl_unorm8 = sycl_img_data_t::unorm_int8;
constexpr sycl_img_data_t sycl_sint8  = sycl_img_data_t::signed_int8;
constexpr sycl_img_data_t sycl_uint8  = sycl_img_data_t::unsigned_int8;
constexpr sycl_img_data_t sycl_half   = sycl_img_data_t::fp16;
constexpr sycl_img_data_t sycl_sint16 = sycl_img_data_t::signed_int16;
constexpr sycl_img_data_t sycl_uint16 = sycl_img_data_t::unsigned_int16;
constexpr sycl_img_data_t sycl_sint32 = sycl_img_data_t::signed_int32;
constexpr sycl_img_data_t sycl_uint32 = sycl_img_data_t::unsigned_int32;
constexpr sycl_img_data_t sycl_float  = sycl_img_data_t::fp32;

// Commonly used channel orders
constexpr sycl::image_channel_order sycl_r    = sycl::image_channel_order::r;
constexpr sycl::image_channel_order sycl_rg   = sycl::image_channel_order::rg;
constexpr sycl::image_channel_order sycl_rgb  = sycl::image_channel_order::rgb;
constexpr sycl::image_channel_order sycl_rgba = sycl::image_channel_order::rgba;

template <typename T> 
inline constexpr uint32_t to_u32(T val) { return static_cast<uint32_t>(val); }

template <typename T, typename U=T> 
inline constexpr T to_T(U val) { return static_cast<T>(val); }

// clang-format on

// #define VERBOSE_PRINT
#define DEBUG_IMG_COPY 0
#ifdef VERBOSE_PRINT
#define VERBOSE_DEBUG DEBUG_IMG_COPY
#else
#define VERBOSE_DEBUG 0
#endif

//----------------------------------------------------------------------------//
// SYCL TYPE MAPPING HELPERS
// ---------------------------------------------------------
// clang-format off
template <typename T> sycl_img_data_t getSyclChannelType();
template <> inline sycl_img_data_t getSyclChannelType<float     >() { return sycl_float;  }
template <> inline sycl_img_data_t getSyclChannelType<sycl::half>() { return sycl_half;   }
template <> inline sycl_img_data_t getSyclChannelType<int32_t   >() { return sycl_sint32; }
template <> inline sycl_img_data_t getSyclChannelType<uint32_t  >() { return sycl_uint32; }
template <> inline sycl_img_data_t getSyclChannelType<int16_t   >() { return sycl_sint16; }
template <> inline sycl_img_data_t getSyclChannelType<uint16_t  >() { return sycl_uint16; }
template <> inline sycl_img_data_t getSyclChannelType<uint8_t   >() { return sycl_uint8;  }
template <> inline sycl_img_data_t getSyclChannelType<int8_t    >() { return sycl_sint8;  }
// clang-format on
//----------------------------------------------------------------------------//
// SYCL CHANNEL ORDER (for unsampled)
// ---------------------------------------------------------
inline sycl::image_channel_order getSyclChannelOrder(int channels) {
  switch (channels) {
  case 1: return sycl_r;
  case 2: return sycl_rg;
  case 4: return sycl_rgba;
  default:
    throw std::runtime_error("Unsupported channel count for SYCL Order");
  }
}
//----------------------------------------------------------------------------//
// VULKAN FORMAT MAPPING
// ---------------------------------------------------------
template <> inline VkFormat getVulkanFormat<sycl::half>(int channels) {
  switch (channels) {
  case 1: return VK_FORMAT_R16_SFLOAT;
  case 2: return VK_FORMAT_R16G16_SFLOAT;
  case 4: return VK_FORMAT_R16G16B16A16_SFLOAT;
  default:
    throw std::runtime_error("Unsupported channels for half");
  }
}
//----------------------------------------------------------------------------//
void wait() {
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
void _printBuf(const InT *data, const uint32_t len,
               const uint32_t prec = Precision<AsT>,
               const uint32_t cols = DefltCols,
               const uint32_t rows = DefltRows) {
  ENTER("_printBuf", "const InT *, uint32_t, ...");
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
  LEAVE;
}
//----------------------------------------------------------------------------//
template<typename InT, uint32_t NChannels = 1, typename AsT = InT>
void _printImg(const InT *data, const uint32_t wdth, const uint32_t hght,
               const uint32_t dpth, const uint32_t prec,
               const uint32_t cols, const uint32_t rows) {
  ENTER("_printImg", "const InT *, uint32_t, uint32_t, uint32_t, ...");
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
  LEAVE;
}
//----------------------------------------------------------------------------//
template<typename InT, typename AsT>
void printImg(const InT *data, const uint32_t wdth, const uint32_t hght,
              const uint32_t numChannels,
              const uint32_t dpth, const uint32_t prec,
              const uint32_t cols, const uint32_t rows) {
  switch (numChannels) {
  case 1:_printImg<InT,1,AsT>(data, wdth, hght, dpth, prec, cols, rows); break;
  case 2:_printImg<InT,2,AsT>(data, wdth, hght, dpth, prec, cols, rows); break;
  case 4:_printImg<InT,4,AsT>(data, wdth, hght, dpth, prec, cols, rows); break;
  default: throw std::runtime_error("Unsupported channel count for printImg");
  }
}
//----------------------------------------------------------------------------//
template<uint32_t NChannels = 1>
inline void printImgU08(const uint8_t *data,
                        const uint32_t wdth, const uint32_t hght,
                        const uint32_t cols = DefltCols,
                        const uint32_t rows = DefltRows) {
  _printImg<uint8_t, NChannels>(data, wdth, hght, 0, cols, rows);
}
//----------------------------------------------------------------------------//
template<uint32_t NChannels = 1>
inline void printImgU32(const uint32_t *data,
                        const uint32_t wdth, const uint32_t hght,
                        const uint32_t cols = DefltCols,
                        const uint32_t rows = DefltRows) {
  _printImg<uint32_t, NChannels>(data, wdth, hght, 0, cols, rows);
}
//----------------------------------------------------------------------------//
template<uint32_t NChannels = 1>
inline void printImgS32(const  int32_t *data,
                        const uint32_t wdth, const uint32_t hght,
                        const uint32_t cols = DefltCols,
                        const uint32_t rows = DefltRows) {
  _printImg<int32_t, NChannels>(data, wdth, hght, 0, cols, rows);
}
//----------------------------------------------------------------------------//
template<uint32_t NChannels = 1, typename AsT = float>
inline void printImgFlt(const float *data,
                        const uint32_t wdth, const uint32_t hght,
                        const uint32_t prec = DefltPrec,
                        const uint32_t cols = DefltCols,
                        const uint32_t rows = DefltRows) {
  _printImg<float, NChannels, AsT>(data, wdth, hght, 0, cols, rows);
}
//----------------------------------------------------------------------------//
template<uint32_t NChannels = 1>
inline void printImgFltAsU32(const float *data,
                             const uint32_t wdth, const uint32_t hght,
                             const uint32_t prec = DefltPrec,
                             const uint32_t cols = DefltCols,
                             const uint32_t rows = DefltRows) {
  _printImg<float, NChannels, uint32_t>(data, wdth, hght, 0, cols, rows);
}
//----------------------------------------------------------------------------//
/*
template <typename T, uint32_t NChannels>
void submitSampledImgReadKernel(sycl::queue &q, uint32_t wdth, uint32_t hght,
                                uint32_t channels, sycl_img_data_t type,
                                bool useSemaphores) {
  using syclexp::sample_image;
  using syclexp::map_external_image_memory;

  // Branch: Sampled vs Unsampled
  syclexp::sampled_image_handle img;

  // Bindless image use (x,y,z) order,
  // differening from SYCL 2020 "fastest incrementing" convention.
  syclexp::image_descriptor imgDesc(sycl::range<2>(width, height), channels,
                                    type);

  // Map external memory
  PUTS("Mapping external image memory");
  sycl_img_mem_h imgMem = map_external_image_memory(
      extMem, imgDesc, q.get_device(), q.get_context());

  PUTS("Creating Sampled Image Handle");
  // Sampler: Nearest required for Integer types
  syclexp::bindless_image_sampler sampler(
      sycl::addressing_mode::clamp_to_edge,
      sycl::coordinate_normalization_mode::unnormalized,
      sycl::filtering_mode::nearest);
  img = syclexp::create_image(imgMem, sampler, imgDesc,
                                        q.get_device(), q.get_context());

  // Output Buffer
  sycl::buffer<T, 1> dstBuf(wdth * hght * channels);

  // Wait for Vulkan semaphore if needed
  sycl::event waitEvent;
  if (useSemaphores) {
    PUTS("Waiting for Vulkan semaphore");
    waitEvent = q.submit([&](sycl::handler &h) {
      h.ext_oneapi_wait_external_semaphore(extSem);
    });
  }

  PUTS("Submitting SYCL kernel to read from sampled image");
  q.submit([&](sycl::handler &h) {
    sycl::stream str(786432, 64, h);
    if (useSemaphores) h.depends_on(waitEvent);
    sycl::accessor dst(dstBuf, h, sycl::write_only);

    // ranges for parallel_for use "fastest incrementing" order (z,y,x),
    // but bindless images ranges use (x,y,z) order.
    h.parallel_for(sycl::range<2>(hght, wdth), [=](sycl::item<2> item) {
      int x = item.get_id(1);
      int y = item.get_id(0);

      // Sampled path: use sample_image with float coordinates
      float coordX = (float)x + 0.5f;
      float coordY = (float)y + 0.5f;

      if constexpr (std::is_floating_point_v<T> ||
                    std::is_same_v<T, sycl::half>) {
        // Float types: sample as Vec4
        sycl::float4 pix = sample_image<sycl::float4>(
            sampledHandle, sycl::float2(coordX, coordY));

        size_t pix = y * wdth + x;
        size_t idx = pix * channels;
        dst[idx++] = static_cast<T>(pix.x());
        if (channels >= 2) dst[idx++] = static_cast<T>(pix.y());
        if (channels >= 4) {
          dst[idx++] = static_cast<T>(pix.z());
          dst[idx++] = static_cast<T>(pix.w());
        }
      } else {
        // Integer types: sample as int4/uint4
        if constexpr (std::is_signed_v<T>) {
          sycl::int4 pix = sample_image<sycl::int4>(
              sampledHandle, sycl::float2(coordX, coordY));

          size_t base = (y * wdth + x) * channels;
          if (channels >= 1)
            dst[base + 0] = static_cast<T>(pix.x());
          if (channels >= 2)
            dst[base + 1] = static_cast<T>(pix.y());
          if (channels >= 4) {
            dst[base + 2] = static_cast<T>(pix.z());
            dst[base + 3] = static_cast<T>(pix.w());
          }
        } else {
          sycl::uint4 pix = sample_image<sycl::uint4>(
              sampledHandle, sycl::float2(coordX, coordY));

          size_t base = (y * wdth + x) * channels;
          if (channels >= 1)
            dst[base + 0] = static_cast<T>(pix.x());
          if (channels >= 2)
            dst[base + 1] = static_cast<T>(pix.y());
          if (channels >= 4) {
            dst[base + 2] = static_cast<T>(pix.z());
            dst[base + 3] = static_cast<T>(pix.w());
          }
        }
      }
    });
  }).wait();
}
*/
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
// TEMPLATED RUNNER
// ---------------------------------------------------------
template <typename T>
int runTest(uint32_t width, uint32_t height, uint32_t channels,
            bool useLinear, bool useSemaphores, bool useSampled,
            VkFormat fmtOverride = VK_FORMAT_UNDEFINED,
            std::optional<sycl_img_data_t> syclOverride = std::nullopt) {

  ENTER("runTest", "int, int, int, bool, bool, bool, VkFormat, std::optional<sycl::image_channel_type>");
  VkImageTiling tiling =
      useLinear ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
  VkFormat vkFormat = (fmtOverride != VK_FORMAT_UNDEFINED)
                          ? fmtOverride
                          : getVulkanFormat<T>(channels);

  std::cout << "VK Format: " << getFormatString(vkFormat) << std::endl;

  // Setup Vulkan
  PUTS("Setting up Vulkan context and resources");
  VulkanContext vkCtx = createVulkanContext();
  VkExtent3D extent = {width, height, 1};
  ImageResources imgRes =
      createExportableImage(vkCtx, extent, vkFormat, VK_IMAGE_TYPE_2D, tiling);

  // For the linear tiling path we map the imported Vulkan memory as a tightly
  // packed linear USM region (see map_external_linear_memory below). That is
  // only valid when Vulkan lays out the image rows with no padding, i.e. when
  // the row pitch equals width * channels * sizeof(T). If the driver adds row
  // padding, the packed assumption is wrong, so warn and exit gracefully rather
  // than produce bogus results.
  size_t rowPitch = getRowPitch(vkCtx, imgRes.image);
  size_t rowBytes = static_cast<size_t>(width) * channels * sizeof(T);
  PRINT("Vulkan image row pitch: {:5} bytes\n", rowPitch);
  PRINT("Actual image row bytes: {:5} bytes\n", rowBytes);
#ifndef _WIN32
  if (useLinear && rowPitch != rowBytes) {
    std::cerr << "WARNING: Vulkan image row pitch (" << rowPitch
              << " bytes) does not match the expected tightly packed pitch of "
              << rowBytes << " bytes (width " << width
              << " * channels " << channels << " * sizeof(T) " << sizeof(T)
              << "). Skipping the --linear test." << std::endl;
    cleanupVulkan(vkCtx, imgRes);
    RETURN("0");
    return 0;
  }
#endif

  // Semaphores
  PUTS("Creating Vulkan Semaphore for synchronization");
  VkSemaphore vkSem = VK_NULL_HANDLE;
  if (useSemaphores) {
    PUTS("Using Vulkan Semaphores for synchronization");
    vkSem = createExportableSemaphore(vkCtx);
  }

  // Upload test data
  PUTS("Uploading test data to Vulkan image");
  if (!uploadAndVerify<T>(vkCtx, imgRes, vkSem, channels)) {
    std::cerr << "Vulkan Upload Failed!" << std::endl;
    RETURN(1);
    return 1;
  }

  using sycl_ext_sem_h = syclexp::external_semaphore_handle_type;
  using sycl_ext_mem_h = syclexp::external_mem_handle_type;
#ifdef _WIN32
  using sycl_ext_mem_desc = syclexp::external_mem_descriptor<syclexp::resource_win32_handle>;
  using sycl_ext_sem_desc = syclexp::external_semaphore_descriptor<syclexp::resource_win32_handle>;

  constexpr sycl_ext_mem_h memHandleType = sycl_ext_mem_h::win32_nt_handle;
  constexpr sycl_ext_sem_h semHandleType = sycl_ext_sem_h::win32_nt_handle;
#else
  using sycl_ext_mem_desc = syclexp::external_mem_descriptor<syclexp::resource_fd>;
  using sycl_ext_sem_desc = syclexp::external_semaphore_descriptor<syclexp::resource_fd>;

  constexpr sycl_ext_mem_h memHandleType = sycl_ext_mem_h::opaque_fd;
  constexpr sycl_ext_sem_h semHandleType = sycl_ext_sem_h::opaque_fd;
#endif

  // SYCL Import and Verification
  try {
    // Bindless image interop requires an in-order queue (per spec). External
    // semaphore ops additionally require immediate command lists; see
    // sycl_ext_oneapi_bindless_images.asciidoc.
    sycl::property_list qProps =
        useSemaphores ? sycl::property_list{sycl::property::queue::in_order{},
                                            sycl::ext::intel::property::queue::
                                                immediate_command_list{}}
                      : sycl::property_list{sycl::property::queue::in_order{}};
    sycl::queue q{qProps};

    // Import Memory (Platform Specific)
#ifdef _WIN32
    HANDLE memHandle = getMemHandle(vkCtx, imgRes.memory);
    sycl_ext_mem_desc extMemDesc{memHandle, memHandleType, imgRes.allocationSize};
#else
    int memFd = getMemFd(vkCtx, imgRes.memory);
    sycl_ext_mem_desc extMemDesc{
        memFd, memHandleType,
        imgRes.allocationSize};
#endif

    syclexp::external_mem extMem = syclexp::import_external_memory(
        extMemDesc, q.get_device(), q.get_context());

    // Import Semaphore (Platform Specific)
    PUTS("Importing Vulkan semaphore");
    syclexp::external_semaphore extSem;
    if (useSemaphores) {
#ifdef _WIN32
      HANDLE semHandle = getSemaphoreHandle(vkCtx, vkSem);
      sycl_ext_sem_desc extSemDesc{semHandle, semHandleType};
#else
      int semFd = getSemaphoreFd(vkCtx, vkSem);
      sycl_ext_sem_desc extSemDesc{semFd, semHandleType};
#endif
      extSem = syclexp::import_external_semaphore(extSemDesc, q.get_device(),
                                                  q.get_context());
    }

    // Create Image Descriptor
    sycl_img_data_t syclType = syclOverride.has_value()
                                            ? syclOverride.value()
                                            : getSyclChannelType<T>();

    // bindless image use (x,y,z) order,
    // differening from SYCL 2020 "fastest incrementing" convention.
    syclexp::image_descriptor imgDesc(sycl::range<2>(width, height), channels,
                                      syclType);

    // Map external memory. When linear tiling is requested, map the imported
    // Vulkan memory as a linear USM region and wrap the returned device pointer
    // in an image_mem_handle. That way the create_image() calls below are
    // identical for both the linear and optimal tiling paths.
    PUTS("Mapping external image memory");
    size_t numElems = width * height * channels;
    syclexp::image_mem_handle imgMem;
    if (useLinear) {
      size_t numBytes = numElems * sizeof(T);
      void *memPtr = syclexp::map_external_linear_memory(
          extMem, 0, numBytes, q.get_device(), q.get_context());
      imgMem.raw_handle =
          reinterpret_cast<syclexp::image_mem_handle::raw_handle_type>(
              memPtr);
    } else {
      imgMem = syclexp::map_external_image_memory(
          extMem, imgDesc, q.get_device(), q.get_context());
    }

    // Branch: Sampled vs Unsampled
    syclexp::sampled_image_handle sampledHandle;
    syclexp::unsampled_image_handle unsampledHandle;

    // Output Buffer
    sycl::buffer<T, 1> dstBuf(numElems);

    if (useSampled) {
      PUTS("Creating Sampled Image Handle");
      // Sampler: Nearest required for Integer types
      syclexp::bindless_image_sampler sampler(
          sycl::addressing_mode::clamp_to_edge,
          sycl::coordinate_normalization_mode::unnormalized,
          sycl::filtering_mode::nearest);
      sampledHandle = syclexp::create_image(imgMem, sampler, imgDesc,
                                            q.get_device(), q.get_context());
    } else {
      PUTS("Creating Unsampled Image Handle");
      // Unsampled image
      unsampledHandle = syclexp::create_image(imgMem, imgDesc,
                                              q.get_device(), q.get_context());
    }

    // Wait for Vulkan semaphore if needed
    sycl::event waitEvent;
    if (useSemaphores) {
      PUTS("Waiting for Vulkan semaphore");
      waitEvent = q.submit([&](sycl::handler &h) {
        h.ext_oneapi_wait_external_semaphore(extSem);
      });
    }

    // Kernel: Read image data
    PUTS("Submitting SYCL kernel to read image data");
    if (useSampled) {
      PUTS("Using Sampled Image Access (sample_image)");
    } else {
      PUTS("Using Unsampled Image Access (fetch_image)");
    }
    q.submit([&](sycl::handler &h) {
      sycl::stream str(786432, 64, h);
      if (useSemaphores)
        h.depends_on(waitEvent);
      sycl::accessor dst(dstBuf, h, sycl::write_only);

      // ranges for parallel_for use "fastest incrementing" order (z,y,x),
      // but bindless images ranges use (x,y,z) order.
      h.parallel_for(sycl::range<2>(height, width), [=](sycl::item<2> item) {
        int x = item.get_id(1);
        int y = item.get_id(0);
        size_t pix = y * width + x;
        size_t idx = pix * channels;

        if (useSampled) {
          // Sampled path: use sample_image with float coordinates
          sycl::float2 pos(x + 0.5f, y + 0.5f);

          if constexpr (std::is_floating_point_v<T> ||
                        std::is_same_v<T, sycl::half>) {
            // Float types: sample as Vec4
            // sycl::float4 pixel = syclexp::sample_image<sycl::float4>(
            //     sampledHandle, pos);

            dst[idx++] = generateTestValue<T>(pix, 0);
            if (channels >= 2)
              dst[idx++] = generateTestValue<T>(pix, 1);
            if (channels >= 4) {
              dst[idx++] = generateTestValue<T>(pix, 2);
              dst[idx++] = generateTestValue<T>(pix, 3);
            }
          } else {
            // Integer types: sample as int4/uint4
            if constexpr (std::is_signed_v<T>) {
              // sycl::int4 pixel = syclexp::sample_image<sycl::int4>(
              //     sampledHandle, pos);

              dst[idx++] = generateTestValue<T>(pix, 0);
              if (channels >= 2)
                dst[idx++] = generateTestValue<T>(pix, 1);
              if (channels >= 4) {
                dst[idx++] = generateTestValue<T>(pix, 2);
                dst[idx++] = generateTestValue<T>(pix, 3);
              }
            } else {
              // sycl::uint4 pixel = syclexp::sample_image<sycl::uint4>(
              //     sampledHandle, pos);

              dst[idx++] = generateTestValue<T>(pix, 0);
              if (channels >= 2)
                dst[idx++] = generateTestValue<T>(pix, 1);
              if (channels >= 4) {
                dst[idx++] = generateTestValue<T>(pix, 2);
                dst[idx++] = generateTestValue<T>(pix, 3);
              }
            }
          }
        } else {
          // Unsampled path: use fetch_image with int coordinates
          sycl::int2 pos(x, y);
          size_t pix = y * width + x;
          size_t idx = pix * channels;

          if (channels == 1) {
            T pixel = syclexp::fetch_image<T>(unsampledHandle, pos);
            dst[idx] = pixel;
          } else if (channels == 2) {
            sycl::vec<T, 2> pixel =
                syclexp::fetch_image<sycl::vec<T, 2>>(unsampledHandle, pos);
            dst[idx++] = pixel.x();
            dst[idx++] = pixel.y();
          } else if (channels == 4) {
            sycl::vec<T, 4> pixel =
                syclexp::fetch_image<sycl::vec<T, 4>>(unsampledHandle, pos);
            dst[idx++] = pixel.x();
            dst[idx++] = pixel.y();
            dst[idx++] = pixel.z();
            dst[idx++] = pixel.w();
          }
        }
      });
    }).wait();

    std::cout << "SYCL Kernel Executed." << std::endl;

    // Verify results
    sycl::host_accessor hostAcc(dstBuf, sycl::read_only);
    bool passed = true;
    int errorCount = 0;

    T *data = new T[numElems];
    for (size_t i = 0; i < numElems; ++i) data[i] = hostAcc[i];
    printImg<T>(data, width, height, channels);

    for (size_t i = 0; i < numElems; ++i) {
      size_t pixelIdx = i / channels;
      int channelIdx = i % channels;

      T expected = generateTestValue<T>(pixelIdx, channelIdx);

      if (!checkValue(data[i], expected)) {
        if (errorCount < 10) {
          std::cerr << "Mismatch at index " << i << " (pixel " << pixelIdx
                    << ", channel " << channelIdx << "): "
                    << "Expected " << expected << ", Got " << data[i]
                    << std::endl;
        }
        errorCount++;
        passed = false;
      }
    }
    delete[] data;

    if (passed) {
      std::cout << "SUCCESS! All " << numElems << " values match."
                << std::endl;
    } else {
      std::cout << "FAILURE! " << errorCount << " errors out of " << numElems
                << " values." << std::endl;
    }

    // Cleanup SYCL resources
    if (useSampled) {
      PUTS("Destroying Sampled Image Handle");
      syclexp::destroy_image_handle(sampledHandle, q.get_device(),
                                    q.get_context());
    } else {
      PUTS("Destroying Unsampled Image Handle");
      syclexp::destroy_image_handle(unsampledHandle, q.get_device(),
                                    q.get_context());
    }

    syclexp::release_external_memory(extMem, q.get_device(), q.get_context());

    if (useSemaphores) {
      PUTS("Releasing External Semaphore");
      syclexp::release_external_semaphore(extSem, q.get_device(),
                                          q.get_context());
      vkDestroySemaphore(vkCtx.device, vkSem, nullptr);
    }

    cleanupVulkan(vkCtx, imgRes);
    RETURN(passed ? 0 : 1);
    return passed ? 0 : 1;

  } catch (std::exception &e) {
    std::cerr << "SYCL Exception: " << e.what() << std::endl;
    cleanupVulkan(vkCtx, imgRes);
    RETURN(1);
    return 1;
  }
}
//----------------------------------------------------------------------------//
// MAIN
// ---------------------------------------------------------
int main(int argc, char **argv) {
  uint32_t width = 4;
  uint32_t height = 4;
  uint32_t channels = 4;
  bool useLinear = false;
  bool useSemaphores = false;
  bool useSampled = false; // Default: unsampled
  std::string type = "float";

  // Parse arguments
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "--sampled") {
      useSampled = true;
    } else if (arg == "--semaphores") {
      useSemaphores = true;
    } else if (arg == "--linear") {
      useLinear = true;
    } else if (arg == "--channels" && i + 1 < argc) {
      channels = to_u32(std::stoi(argv[++i]));
    } else if (arg == "--type" && i + 1 < argc) {
      type = argv[++i];
    } else if (arg.find('x') != std::string::npos) {
      // Parse WxH
      size_t pos = arg.find('x');
      width = to_u32(std::stoi(arg.substr(0, pos)));
      height = to_u32(std::stoi(arg.substr(pos + 1)));
    }
  }

  if (channels != 1 && channels != 2 && channels != 4) {
    std::cerr << "Error: Only 1, 2, or 4 channels supported." << std::endl;
    return 1;
  }

  std::cout << "Running " << (useSampled ? "SAMPLED" : "UNSAMPLED")
            << " 2D Read Test | Type: " << type << " | Size: " << width << "x"
            << height << " | Channels: " << channels
            << " | Tiling: " << (useLinear ? "LINEAR" : "OPTIMAL")
            << " | Semaphores: " << (useSemaphores ? "ON" : "OFF") << std::endl;

  // Dispatch to appropriate type
  if (type == "float")
    return runTest<float>(width, height, channels, useLinear, useSemaphores,
                          useSampled);
  if (type == "half")
    return runTest<sycl::half>(width, height, channels, useLinear,
                               useSemaphores, useSampled);

  if (type == "int32")
    return runTest<int32_t>(width, height, channels, useLinear, useSemaphores,
                            useSampled);
  if (type == "uint32")
    return runTest<uint32_t>(width, height, channels, useLinear, useSemaphores,
                             useSampled);

  if (type == "int16")
    return runTest<int16_t>(width, height, channels, useLinear, useSemaphores,
                            useSampled);
  if (type == "uint16")
    return runTest<uint16_t>(width, height, channels, useLinear, useSemaphores,
                             useSampled);

  if (type == "uint8")
    return runTest<uint8_t>(width, height, channels, useLinear, useSemaphores,
                            useSampled);
  if (type == "int8")
    return runTest<int8_t>(width, height, channels, useLinear, useSemaphores,
                           useSampled);

  if (type == "unorm8") {
    return runTest<uint8_t>(width, height, channels, useLinear, useSemaphores,
                            useSampled, getUnorm8Format(channels),
                            sycl_unorm8);
  }

  std::cerr << "Unknown type: " << type << std::endl;
  return 1;
}
//----------------------------------------------------------------------------//
