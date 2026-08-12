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
#include <sycl/sycl.hpp> // For sycl::stream
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
using sycl_img_desc     = syclexp::image_descriptor;
using sycl_ext_sem      = syclexp::external_semaphore;

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

// clang-format on

#define VERBOSE_PRINT
#define DEBUG_IMG_COPY 0
#ifdef VERBOSE_PRINT
#define VERBOSE_DEBUG DEBUG_IMG_COPY
#else
#define VERBOSE_DEBUG 0
#endif

//----------------------------------------------------------------------------//
// SYCL TYPE MAPPING HELPERS
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
inline sycl::image_channel_order getSyclChannelOrder(uint32_t channels) {
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
template <> inline VkFormat getVulkanFormat<sycl::half>(uint32_t channels) {
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
template <typename T>
bool verifyResults(sycl::buffer<T, 1> &dstBuf, uint32_t wdth, uint32_t hght,
                   uint32_t channels) {
  // Verify results
  sycl::host_accessor hostAcc(dstBuf, sycl::read_only);
  bool passed = true;
  int errors = 0;

  const size_t numPixs = to_size(wdth) * to_size(hght);
  const size_t numVals = numPixs * to_size(channels);

  T *data = new T[numVals];
  for (size_t i = 0; i < numVals; ++i) data[i] = hostAcc[i];
  printImg<T>(data, wdth, hght, channels);

  for (size_t pix = 0, idx = 0; pix < numPixs; ++pix) {
    for (uint32_t c = 0; c < channels; ++c, ++idx) {
      T expect = generateTestValue<T>(pix, c);
      if (!checkValue(data[idx], expect)) {
        if (errors < 10) {
          std::cerr << "Mismatch at index " << idx << " (pixel " << pix
                    << ", channel " << c << "): "
                    << "Expected " << expect << ", Got " << data[idx]
                    << std::endl;
        }
        errors++;
        passed = false;
      }
    }
  }
  delete[] data;

  if (passed) std::cout << "SUCCESS! All " << numVals << " values match.\n";
  else        std::cout << "FAILURE! " << errors << " errors out of " << numVals
                        << " values.\n";
  return passed;
}

//----------------------------------------------------------------------------//
// Templated function to run the sampled image kernel and verify results.
template <typename T>
bool runKernelSampled(sycl::buffer<T, 1> &dstBuf, uint32_t wdth, uint32_t hght,
                      uint32_t channels, sycl::queue &q, sycl_img_mem_h imgMem,
                      sycl_img_desc imgDesc, sycl_ext_sem *extSem = nullptr) {
  assert(channels == 1 || channels == 2 || channels == 4);

  constexpr bool isFloat = std::is_floating_point_v<T> ||
                           std::is_same_v<T, sycl::half>;
  constexpr bool isSigned = std::is_signed_v<T>;

  ENTER("runKernelSampled", "sycl::buffer<T, 1> &, uint32_t, uint32_t, uint32_t, sycl::queue &, sycl_img_mem_h, sycl_img_desc, sycl_ext_sem *");
  syclexp::sampled_image_handle imgHandle;

  PUTS("Creating Sampled Image Handle");
  // Sampler: Nearest required for Integer types
  syclexp::bindless_image_sampler sampler(
      sycl::addressing_mode::clamp_to_edge,
      sycl::coordinate_normalization_mode::unnormalized,
      sycl::filtering_mode::nearest);
  imgHandle = syclexp::create_image(imgMem, sampler, imgDesc, q.get_device(),
                                    q.get_context());

  // Wait for Vulkan semaphore if needed
  sycl::event waitEvent;
  if (extSem) {
    PUTS("Waiting for Vulkan semaphore");
    waitEvent = q.submit([&](sycl::handler &h) {
      h.ext_oneapi_wait_external_semaphore(*extSem);
    });
  }

  // Kernel: Read image data
  PUTS("Submitting SYCL kernel to read SAMPLED image data");
  q.submit([&](sycl::handler &h) {
    if (extSem) h.depends_on(waitEvent);
    sycl::accessor dst(dstBuf, h, sycl::write_only);

    // ranges for parallel_for use "fastest incrementing" order (z,y,x),
    // but bindless images ranges use (x,y,z) order.
    h.parallel_for(sycl::range<2>(hght, wdth), [=](sycl::item<2> item) {
      int x = item.get_id(1);
      int y = item.get_id(0);
      size_t idx = (y * wdth + x) * channels;

      using IntT = std::conditional_t<isSigned, int32_t, uint32_t>;
      using ValT = std::conditional_t<isFloat, float, IntT>;
      using PixT = sycl::vec<ValT, 4>; // Sampled image always returns 4 channels

      // Sampled path: use sample_image with float coordinates
      sycl::float2 pos(x + 0.5f, y + 0.5f);

      PixT pixel = syclexp::sample_image<PixT>(imgHandle, pos);

      dst[idx + 0] = pixel.x();
      if (channels >= 2)
        dst[idx + 1] = pixel.y();
      if (channels >= 4) {
        dst[idx + 2] = pixel.z();
        dst[idx + 3] = pixel.w();
      }
    });
  }).wait();
  std::cout << "SYCL Kernel Executed." << std::endl;

  bool passed = verifyResults<T>(dstBuf, wdth, hght, channels);

  PUTS("Destroying Sampled Image Handle");
  syclexp::destroy_image_handle(imgHandle, q.get_device(), q.get_context());
  RETURN(passed);
  return passed;
}
//----------------------------------------------------------------------------//
// Templated function to run the unsampled image kernel and verify results.
template <typename T>
bool runKernelUnsamp(sycl::buffer<T, 1> &dstBuf, uint32_t wdth, uint32_t hght,
                     uint32_t channels, sycl::queue &q, sycl_img_mem_h imgMem,
                     sycl_img_desc imgDesc, sycl_ext_sem *extSem = nullptr) {
  assert(channels == 1 || channels == 2 || channels == 4);

  ENTER("runKernelUnsamp", "sycl::buffer<T, 1> &, uint32_t, uint32_t, uint32_t, sycl::queue &, sycl_img_mem_h, sycl_img_desc, sycl_ext_sem *");
  syclexp::unsampled_image_handle imgHandle;

  PUTS("Creating Unsampled Image Handle");
  imgHandle = syclexp::create_image(imgMem, imgDesc, q.get_device(),
                                    q.get_context());

  // Wait for Vulkan semaphore if needed
  sycl::event waitEvent;
  if (extSem) {
    PUTS("Waiting for Vulkan semaphore");
    waitEvent = q.submit([&](sycl::handler &h) {
      h.ext_oneapi_wait_external_semaphore(*extSem);
    });
  }

  // Kernel: Read image data
  PUTS("Submitting SYCL kernel to read UNSAMPLED image data");
  q.submit([&](sycl::handler &h) {
    if (extSem) h.depends_on(waitEvent);
    sycl::accessor dst(dstBuf, h, sycl::write_only);

    // ranges for parallel_for use "fastest incrementing" order (z,y,x),
    // but bindless images ranges use (x,y,z) order.
    h.parallel_for(sycl::range<2>(hght, wdth), [=](sycl::item<2> item) {
      int x = item.get_id(1);
      int y = item.get_id(0);
      size_t idx = (y * wdth + x) * channels;

      // Unsampled path: use fetch_image with int coordinates
      sycl::int2 pos(x, y);

      if (channels == 1) {
        T pixel = syclexp::fetch_image<T>(imgHandle, pos);
        dst[idx] = pixel;
      } else if (channels == 2) {
        sycl::vec<T, 2> pixel =
            syclexp::fetch_image<sycl::vec<T, 2>>(imgHandle, pos);
        dst[idx + 0] = pixel.x();
        dst[idx + 1] = pixel.y();
      } else if (channels == 4) {
        sycl::vec<T, 4> pixel =
            syclexp::fetch_image<sycl::vec<T, 4>>(imgHandle, pos);
        dst[idx + 0] = pixel.x();
        dst[idx + 1] = pixel.y();
        dst[idx + 2] = pixel.z();
        dst[idx + 3] = pixel.w();
      }
    });
  }).wait();
  std::cout << "SYCL Kernel Executed." << std::endl;

  bool passed = verifyResults<T>(dstBuf, wdth, hght, channels);

  PUTS("Destroying Unsampled Image Handle");
  syclexp::destroy_image_handle(imgHandle, q.get_device(), q.get_context());
  RETURN(passed);
  return passed;
}
//----------------------------------------------------------------------------//
// TEMPLATED RUNNER
template <typename T>
int runTest(uint32_t wdth, uint32_t hght, uint32_t channels, bool useLinear,
            bool useSemaphores, bool useSampled,
            VkFormat fmtOverride = VK_FORMAT_UNDEFINED,
            std::optional<sycl_img_data_t> syclOverride = std::nullopt) {
  assert(channels == 1 || channels == 2 || channels == 4);

  constexpr bool isFloat = std::is_floating_point_v<T> || std::is_same_v<T, sycl::half>;
  constexpr bool isSigned = std::is_signed_v<T>;

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
  VkExtent3D extent = {wdth, hght, 1};
  ImageResources imgRes =
      createExportableImage(vkCtx, extent, vkFormat, VK_IMAGE_TYPE_2D, tiling);

  { // Get the Vulkan row pitch to set the corresponding image descriptor.
    size_t rowPitch = getRowPitch(vkCtx, imgRes.image);
    size_t rowBytes = to_size(wdth) * to_size(channels) * sizeof(T);
    PRINT("Vulkan image row pitch: {:5} bytes\n", rowPitch);
    PRINT("Linear image row bytes: {:5} bytes\n", rowBytes);
  }

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
    HANDLE memNT = getMemHandle(vkCtx, imgRes.memory);
    sycl_ext_mem_desc extMemDesc{memNT, memHandleType, imgRes.allocationSize};
#else
    int memFd = getMemFd(vkCtx, imgRes.memory);
    sycl_ext_mem_desc extMemDesc{memFd, memHandleType, imgRes.allocationSize};
#endif

    syclexp::external_mem extMem = syclexp::import_external_memory(
        extMemDesc, q.get_device(), q.get_context());

    // Import Semaphore (Platform Specific)
    PUTS("Importing Vulkan semaphore");
    sycl_ext_sem extSem;
    if (useSemaphores) {
#ifdef _WIN32
      HANDLE semNT = getSemaphoreHandle(vkCtx, vkSem);
      sycl_ext_sem_desc extSemDesc{semNT, semHandleType};
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

    // When the Vulkan image was created with LINEAR tiling, its row stride is
    // not guaranteed to match SYCL's tightly-packed default. Query Vulkan for
    // the actual row pitch and forward it to the image_descriptor so adapters
    // that can honor a user-supplied pitch (e.g. L0) use the right stride.
    // vkGetImageSubresourceLayout is only defined for LINEAR tiling; leave the
    // pitch at 0 (tightly-packed) for OPTIMAL tiling.
    size_t rowPitch = useLinear ? getRowPitch(vkCtx, imgRes.image) : 0;

    // bindless image use (x,y,z) order,
    // differening from SYCL 2020 "fastest incrementing" convention.
    sycl_img_desc imgDesc(sycl::range<2>(wdth, hght), channels,
                                      syclType, syclexp::image_type::standard,
                                      /*num_levels=*/1, /*array_size=*/1,
                                      /*num_samples=*/0, rowPitch);

    // Map external memory
    size_t numElems = to_size(wdth) * to_size(hght) * to_size(channels);
    sycl_img_mem_h imgMem;
    if (useLinear) {
      PUTS("Mapping external linear memory");
      size_t numBytes = numElems * sizeof(T);
      void *memPtr = syclexp::map_external_linear_memory(
          extMem, 0, numBytes, q.get_device(), q.get_context());
      imgMem.raw_handle =
          reinterpret_cast<sycl_img_mem_h::raw_handle_type>(memPtr);
    } else {
      PUTS("Mapping external image memory");
      imgMem = syclexp::map_external_image_memory(
          extMem, imgDesc, q.get_device(), q.get_context());
    }

    // Output Buffer
    sycl::buffer<T, 1> dstBuf(numElems);

    bool passed = false;
#ifndef DISABLE_SAMPLED_IMAGE
    if (useSampled)
      passed = runKernelSampled(dstBuf, wdth, hght, channels, q, imgMem,
                                imgDesc, useSemaphores ? &extSem : nullptr);
    else
#endif // DISABLE_SAMPLED_IMAGE
      passed = runKernelUnsamp(dstBuf, wdth, hght, channels, q, imgMem, imgDesc,
                               useSemaphores ? &extSem : nullptr);

    // Cleanup SYCL resources
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
