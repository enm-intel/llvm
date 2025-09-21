// REQUIRES: aspect-ext_oneapi_bindless_images
// REQUIRES: aspect-ext_oneapi_external_memory_import || (windows && level_zero && aspect-ext_oneapi_bindless_images)
// REQUIRES: vulkan

// RUN: %{build} %link-vulkan -o %t.out %if target-spir %{ -Wno-ignored-attributes -DENABLE_LINEAR_TILING -DTEST_L0_SUPPORTED_VK_FORMAT %}
// RUN: %{run} env NEOReadDebugKeys=1 UseBindlessMode=1 UseExternalAllocatorForSshAndDsh=1 %t.out

// Uncomment to print additional test information
// #define VERBOSE_PRINT

#ifdef _WIN32
#define NOMINMAX
#endif

#include "../../CommonUtils/vulkan_common.hpp"
#include "../helpers/common.hpp"
#include "utils.hpp"

#include <sycl/sycl.hpp>
#include <sycl/ext/oneapi/bindless_images.hpp>

namespace syclexp = sycl::ext::oneapi::experimental;

using namespace sycl_vulkan_img_utils;

//-==========================================================================-//
struct handles_t {
  syclexp::sampled_image_handle imgInput;
  syclexp::image_mem_handle imgMem;
  syclexp::external_mem inputExternalMem;
  syclexp::external_semaphore sycl_wait_external_semaphore;
};
//-==========================================================================-//

//-==========================================================================-//
template <typename DType, sycl::image_channel_type CType> struct OutputType {
  using type = DType;
};

template <> struct OutputType<uint8_t, sycl_unorm8> {
  using type = float;
};
//-==========================================================================-//

//----------------------------------------------------------------------------//
template <typename InteropHandleT, typename InteropSemHandleT>
handles_t create_test_handles(
    sycl::context &ctxt, sycl::device &dev,
    const syclexp::bindless_image_sampler &samp, InteropHandleT interopHandle,
    [[maybe_unused]] InteropSemHandleT sycl_wait_semaphore_handle,
    syclexp::image_descriptor desc, const size_t imgSize) {
  // Extension: external memory descriptor
#ifdef _WIN32
  syclexp::external_mem_descriptor<syclexp::resource_win32_handle>
      inputExtMemDesc{interopHandle,
                      syclexp::external_mem_handle_type::win32_nt_handle,
                      imgSize};
#else
  syclexp::external_mem_descriptor<syclexp::resource_fd> inputExtMemDesc{
      interopHandle, syclexp::external_mem_handle_type::opaque_fd, imgSize};
#endif

  // Extension: external memory imported from file descriptor
  syclexp::external_mem inputExternalMem =
      syclexp::import_external_memory(inputExtMemDesc, dev, ctxt);

  // Extension: mapped memory handle from external memory
  syclexp::image_mem_handle inputMappedMemHandle =
      syclexp::map_external_image_memory(inputExternalMem, desc, dev, ctxt);

  // Extension: create the image and return the handle
  syclexp::sampled_image_handle imgInput =
      syclexp::create_image(inputMappedMemHandle, samp, desc, dev, ctxt);

#ifdef TEST_SEMAPHORE_IMPORT
  // Extension: import semaphores
#ifdef _WIN32
  syclexp::external_semaphore_descriptor<syclexp::resource_win32_handle>
      sycl_wait_external_semaphore_desc{
          sycl_wait_semaphore_handle,
          syclexp::external_semaphore_handle_type::win32_nt_handle};
#else
  syclexp::external_semaphore_descriptor<syclexp::resource_fd>
      sycl_wait_external_semaphore_desc{
          sycl_wait_semaphore_handle,
          syclexp::external_semaphore_handle_type::opaque_fd};
#endif

  syclexp::external_semaphore sycl_wait_external_semaphore =
      syclexp::import_external_semaphore(sycl_wait_external_semaphore_desc, dev,
                                         ctxt);
#else  // #ifdef TEST_SEMAPHORE_IMPORT
  syclexp::external_semaphore sycl_wait_external_semaphore{};
#endif // #ifdef TEST_SEMAPHORE_IMPORT

  return {imgInput, inputMappedMemHandle, inputExternalMem,
          sycl_wait_external_semaphore};
}
//----------------------------------------------------------------------------//
template <typename InteropHandleT, typename InteropSemHandleT, uint32_t NDims,
          typename DType, uint32_t NChannels, sycl::image_channel_type CType,
          typename KernelName>
bool run_sycl(sycl::queue syclQueue, Dims3D<NDims> dims, Dims3D<NDims> grpSize,
              InteropHandleT inputInteropMemHandle,
              InteropSemHandleT sycl_wait_semaphore_handle) {
  auto dev = syclQueue.get_device();
  auto ctxt = syclQueue.get_context();

  // Image descriptor - mapped to Vulkan image layout
  syclexp::image_descriptor desc(dims.to_sycl_range(), NChannels, CType);

  syclexp::bindless_image_sampler samp(
      sycl::addressing_mode::repeat,
      sycl::coordinate_normalization_mode::normalized,
      sycl::filtering_mode::linear);

  const size_t numElems = dims.size();
  const size_t img_size = numElems * sizeof(DType) * NChannels;

  const size_t wdth = dims.wdth;
  const size_t hght = dims.hght;
  const size_t dpth = dims.dpth;

  sycl::range<NDims> syclDim = dims.to_flip_range();
  sycl::range<NDims> syclGrp = grpSize.to_flip_range();

  using OutType = typename OutputType<DType, CType>::type;
  using VecType = sycl::vec<OutType, NChannels>;

  auto handles =
      create_test_handles(ctxt, dev, samp, inputInteropMemHandle,
                          sycl_wait_semaphore_handle, desc, img_size);

#ifdef TEST_SEMAPHORE_IMPORT
  // Extension: wait for imported semaphore
  syclQueue.ext_oneapi_wait_external_semaphore(
      handles.sycl_wait_external_semaphore);
#endif

  std::vector<VecType> out(numElems);
  printString("Submitting SYCL kernel");
#ifdef VERBOSE_PRINT
  std::cout << "\timage size: " << wdth << "(w)";
  if constexpr (NDims >= 2) std::cout << " x " << hght << "(h)";
  if constexpr (NDims == 3) std::cout << " x " << dpth << "(d)";
  std::cout << " -> group size: " << grpSize.wdth << "(w)";
  if constexpr (NDims >= 2) std::cout << " x " << grpSize.hght << "(h)";
  if constexpr (NDims == 3) std::cout << " x " << grpSize.dpth << "(d)";
  std::cout << "\n";
  std::cout << "\t# elements: " << numElems << "\n";
  std::cout << "\t# channels: " << NChannels << "\n";
  std::cout << "\telem size : " << sizeof(OutType) << " --> pixel size: "
            << sizeof(VecType) << "\n";
  std::cout << "\t# bytes   : " << numElems * sizeof(VecType) << "\n";
  if (sizeof(DType) != sizeof(OutType))
    std::cout << "\t***** NOTE: Output data type different from image data type *****\n";
#endif
  using samp_t = std::conditional_t<NChannels == 1, OutType, VecType>;
  try {
    sycl::buffer<VecType, NDims> buf((VecType *)out.data(), syclDim);
    syclQueue.submit([&](sycl::handler &cgh) {
      sycl::stream str(786432, 48, cgh);
      auto outAcc = buf.template get_access<sycl::access_mode::write>(
          cgh, syclDim);
      cgh.parallel_for<KernelName>(
          sycl::nd_range<NDims>{syclDim, syclGrp},
          [=](sycl::nd_item<NDims> it) {
            if constexpr (NDims == 3) {
              size_t z = it.get_global_id(0);
              size_t y = it.get_global_id(1);
              size_t x = it.get_global_id(2);

              // Normalize coordinates -- +0.5 to look towards centre of pixel
              sycl::float3 samp_pos{float(x + 0.5f) / (float)wdth,
                                    float(y + 0.5f) / (float)hght,
                                    float(z + 0.5f) / (float)dpth};

              // Extension: sample image data from Vulkan imported handle
              VecType pix = syclexp::sample_image<samp_t>(handles.imgInput,
                                                          samp_pos);
              outAcc[sycl::id{z, y, x}] = pix / static_cast<OutType>(2);
            } else if constexpr (NDims == 2) {
              size_t y  = it.get_global_id(0);
              size_t x  = it.get_global_id(1);
              // size_t sy = it.get_global_range(0);
              // size_t sx = it.get_global_range(1);
              // const char *py = (y < 10 ? "  " : (y < 100 ? " " : ""));
              // const char *px = (x < 10 ? "  " : (x < 100 ? " " : ""));
              // sycl::id id{y, x};

              // Normalize coordinates -- +0.5 to look towards centre of pixel
              sycl::float2 samp_pos{float(x + 0.5f) / (float)wdth,
                                    float(y + 0.5f) / (float)hght};

              // Extension: sample image data from handle (Vulkan imported)
              VecType pix = syclexp::sample_image<samp_t>(
                  handles.imgInput, samp_pos);

              // pix /= static_cast<OutType>(2);

              // size_t idx = outAcc.getIndex(id);
              // size_t gli = it.get_global_linear_id();
              // // size_t lli = it.get_local_linear_id();
              // OutType val = static_cast<OutType>(-1);
              // // constexpr bool readOkay = (NChannels < 4 || sizeof(DType) < 4);
              // constexpr bool readOkay = true;
              // if      constexpr (NChannels == 1) val = pix;
              // else if constexpr (readOkay)       val = pix[0];
              // const char *pgi  = (gli < 10 ? "  " : (gli < 100 ? " " : ""));
              // const char *pidx = (idx < 10 ? "  " : (idx < 100 ? " " : ""));
              // if (x % 4 == 0)
              //   str << "(" << py << y << "/" << sy << "," << px << x << "/" << sx << ")->"
              //       << pidx << idx << ":" << pgi << gli << "->" << val << sycl::endl;
              // if constexpr (NChannels == 4 && (CType == sycl_float || CType == sycl_uint32)) {
              //   size_t idx = outAcc.getIndex(id);
              //   if (idx < 8) {
              //     outAcc[id] = pix;
              //   }
              // }
              // else {
              //   outAcc[id] = pix;
              // }
              outAcc[sycl::id{y, x}] = pix;
            } else {
              size_t x = it.get_global_id(0);

              // Normalize coordinates -- +0.5 to look towards centre of pixel
              float fx = float(x + 0.5f) / (float)wdth;

              // Extension: sample image data from handle (Vulkan imported)
              VecType pix = syclexp::sample_image<samp_t>(handles.imgInput, fx);

              outAcc[x] = pix / static_cast<OutType>(2);
            }
          });
    });
    printString("SYCL kernel submitted; waiting for completion");
    // printString("   row    col    ->idx:gli-> pixel");
    syclQueue.wait_and_throw();

    printString("Cleaning up");
#ifdef TEST_SEMAPHORE_IMPORT
    syclexp::release_external_semaphore(handles.sycl_wait_external_semaphore,
                                        dev, ctxt);
#endif
    syclexp::destroy_image_handle(handles.imgInput, dev, ctxt);
    syclexp::unmap_external_image_memory(
        handles.imgMem, syclexp::image_type::standard, dev, ctxt);
    syclexp::release_external_memory(handles.inputExternalMem, dev, ctxt);
  } catch (sycl::exception e) {
    std::cerr << "\tKernel submission failed! " << e.what() << std::endl;
    exit(-1);
  } catch (...) {
    std::cerr << "\tKernel submission failed!" << std::endl;
    exit(-1);
  }

  printString("Validating");
  bool validated = true;
  auto getExpectedValue = [&](uint64_t i) -> OutType {
    if constexpr (std::is_integral_v<DType> ||
                  std::is_same_v<DType, sycl::half>)
      i %= static_cast<uint64_t>(std::numeric_limits<DType>::max()) + 1;
    if (CType == sycl_unorm8)
      return static_cast<OutType>(static_cast<float>(i) / 510.0f);
    // return static_cast<OutType>(i / 2.0f);
    return static_cast<OutType>(i);
  };
#ifdef VERBOSE_PRINT
  bool prevMismatch = false;
  VecType prevVal = out[0];
  VecType prevExp = prevVal;

  std::cout << "   ";
  for (uint32_t x = 0; x < wdth; ++x) {
    uint32_t x_m = x % 64;
    if (x_m < 3 || x_m >= 61) std::cout << " " << std::setw(6) << x;
  }
#endif

  for (size_t i = 0; i < numElems; i++) {
    uint32_t x = static_cast<uint32_t>(i % wdth);
    uint32_t y = static_cast<uint32_t>(i / wdth);
    uint32_t x_m = x % 64;
    uint32_t y_m = y % 32;
    bool mismatch = false;
    VecType value = out[i];
    VecType expect =
        bindless_helpers::init_vector<OutType, NChannels>(getExpectedValue(i));
#ifdef VERBOSE_PRINT
    if (y_m < 3 || y_m >= 29) {
      if (x == 0) {
        std::cout << "\n";
        if (y_m == 0) {
          std::cout << "   +";
          if (sizeof(DType) * NChannels > 8) {
            for (uint32_t x = 0; x < (wdth / 64); ++x) {
              std::cout << "---------------------------------------------+";
            }
          }
          else {
            for (uint32_t x = 0; x < (wdth / 128); ++x) {
              std::cout << "-------------------------------------------------------------------------------------------+";
            }
          }
          std::cout << "\n";
        }
        std::cout << std::setw(3) << y << "|" << std::setw(6) << value[0];
      } else {
        if (x_m < 3 || x_m >= 61) {
          if (value[0] - prevVal[0] > 1) std::cout << "|";
          else std::cout << " ";
          std::cout << std::setw(6) << value[0];
          if (x == wdth - 1) std::cout << "|";
        }
        else if (x_m == 3) std::cout << " ...";
      }
    }
    else if (y_m < 6 && x == 0) std::cout << "\n . |   .";
#endif  
    if (!bindless_helpers::equal_vec<OutType, NChannels>(value, expect)) {
      mismatch = true;
      validated = false;
    }

    if (mismatch) {
#ifdef VERBOSE_PRINT
      // if (!prevMismatch) {
      //   uint32_t x = static_cast<uint32_t>(i % wdth);
      //   uint32_t y = static_cast<uint32_t>(i / wdth);
      //   std::cout << "Result mismatch [(x,y) = (" << std::setw(3) << x << ", "
      //             << std::setw(3) << y << ") : idx = " << std::setw(5) << i
      //             << "]! Expected: " << expect << ", Actual: " << value
      //             << "\n";
      // }
#else
      break;
#endif
    }
// #ifdef VERBOSE_PRINT
//     else if (prevMismatch) {
//       size_t prv_i = (i - 1);
//       uint32_t x = static_cast<uint32_t>(prv_i % wdth);
//       uint32_t y = static_cast<uint32_t>(prv_i / wdth);
//       std::cout << "... All results in between also mismatched.\n";
//       std::cout << "Result mismatch [(x,y) = (" << std::setw(3) << x << ", "
//                 << std::setw(3) << y << ") : idx = " << std::setw(5) << prv_i
//                 << "]! Expected: " << prevExp << ", Actual: " << prevVal
//                 << "\n";
//     }
    prevExp = expect;
    prevVal = value;
    prevMismatch = mismatch;
// #endif
  }
  if (validated) {
#ifdef VERBOSE_PRINT
    std::cout << "\tTest passed: NDims " << NDims << " NChannels " << NChannels
              << " image_channel_type "
              << bindless_helpers::channelTypeToString(CType) << "\n";
#endif
  }

  return validated;
}
//----------------------------------------------------------------------------//
template <uint32_t NDims, typename DType, uint32_t NChannels,
          sycl::image_channel_type CType, sycl::image_channel_order COrder,
          typename KernelName>
bool run_test(Dims3D<NDims> dims, Dims3D<NDims> grpSize) {
  using OutType = typename OutputType<DType, CType>::type;
  using VecType = sycl::vec<OutType, NChannels>;

#ifdef VERBOSE_PRINT
    std::cout << "----------------------------------------\n";
    std::cout << "Running test:\n\tdimensions  : " << NDims
              << "\n\tchannels    : " << NChannels
              << "\n\tchannel type: "
              << bindless_helpers::channelTypeToString(CType) << "\n";
#endif

  if constexpr (sycl::is_device_copyable_v<VecType>) {
    printString("Buffer type is device copyable.");
  }
  else {
    printString("Buffer type is NOT device copyable --> SKIPPING TEST.");
    return true;
  }

  uint32_t w = dims.wdth;
  uint32_t h = dims.hght;
  uint32_t d = dims.dpth;
  VkExtent3D vkExtent = {w, h, d};

  sycl::queue syclQueue;

  // Skip `sycl::half` tests if fp16 is unsupported.
  if constexpr (std::is_same_v<DType, sycl::half>) {
    if (!syclQueue.get_device().has(sycl::aspect::fp16)) {
      return true;
    }
  }

  // Verify SYCL device support for allocating/creating an image from the
  // descriptor being tested.
  // This test always maps to an `image_mem_handle` (opaque_handle).
  syclexp::image_descriptor desc{dims, NChannels, CType};
  if (!bindless_helpers::memoryAllocationSupported(
          desc, syclexp::image_memory_handle_type::opaque_handle, syclQueue)) {
    // The device does not support allocating/creating the image with the given
    // properties. Skip the test.
    std::cout << "Memory allocation unsupported. Skipping test.\n";
    return true;
  }

  size_t numElems = dims.num_elems();
  constexpr VkImageType imgTypes[] = {VK_IMAGE_TYPE_1D, VK_IMAGE_TYPE_2D,
                                      VK_IMAGE_TYPE_3D};
  constexpr VkImageType imgType = imgTypes[NDims - 1];

  VkFormat format = vkutil::to_vulkan_format(COrder, CType);
  const size_t imgBytes = numElems * NChannels * sizeof(DType);

  printString("Creating input image");
// #ifdef VERBOSE_PRINT
//   std::cout << "\timage size: " << w << "(w)";
//   if constexpr (NDims >= 2) std::cout << " x " << h << "(h)";
//   if constexpr (NDims == 3) std::cout << " x " << d << "(d)";
//   std::cout << "\n";
//   std::cout << "\t# elements: " << numElems << "\n";
//   std::cout << "\t# channels: " << NChannels << "\n";
//   std::cout << "\telem size : " << sizeof(DType) << "\n";
//   std::cout << "\t# bytes   : " << imgBytes << "\n";
// #endif

  // Create input image memory
  constexpr VkImageUsageFlags imgFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                         VK_IMAGE_USAGE_TRANSFER_DST_BIT;
#ifdef ENABLE_LINEAR_TILING
  constexpr bool linearTiling = true;
#else
  constexpr bool linearTiling = false;
#endif
  VkImage srcImg = vkutil::createImage(imgType, format, vkExtent, imgFlags,
                                      1U /*mipLevels*/, linearTiling);
  VkMemoryRequirements memReqs;
  auto srcMemIdx = vkutil::getImageMemoryTypeIndex(
      srcImg, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memReqs);
  auto srcMem = vkutil::allocateDeviceMemory(imgBytes, srcMemIdx, srcImg);
  VK_CHECK_CALL(vkBindImageMemory(vk_device, srcImg, srcMem, 0 /*offset*/));

  printString("Creating staging buffers");
  // Create input staging memory
  constexpr VkBufferUsageFlags bufFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                          VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  constexpr VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  auto stagingBuf = vkutil::createBuffer(imgBytes, bufFlags);
  auto stagingIdx = vkutil::getBufferMemoryTypeIndex(stagingBuf, flags);
  auto stagingMem = vkutil::allocateDeviceMemory(imgBytes, stagingIdx,
                                                 nullptr /*image*/,
                                                 false /*exportable*/);
  VK_CHECK_CALL(vkBindBufferMemory(vk_device, stagingBuf,
                                   stagingMem, 0 /*offset*/));

  printString("Populating staging buffer");
  // Populate staging memory
  DType *stagingData = nullptr;
  VK_CHECK_CALL(vkMapMemory(vk_device, stagingMem, 0 /*offset*/, imgBytes,
                            0 /*flags*/, (void **)&stagingData));
  auto getInputValue = [&](uint64_t i) -> DType {
    // if (CType == sycl_unorm8)
    //   return static_cast<DType>(255);
    if constexpr (std::is_integral_v<DType> ||
                  std::is_same_v<DType, sycl::half>)
      i %= static_cast<uint64_t>(std::numeric_limits<DType>::max()) + 1;
    return static_cast<DType>(i);
  };
#ifdef VERBOSE_PRINT
  std::cout << "   ";
  for (uint32_t x = 0; x < w; ++x) {
    uint32_t x_m = x % 64;
    if (x_m < 3 || x_m >= 61) std::cout << " " << std::setw(6) << x;
  }
#endif
  { DType *p = stagingData;
    for (size_t i = 0; i < numElems; ++i) {
      uint32_t x = static_cast<uint32_t>(i % w);
      uint32_t y = static_cast<uint32_t>(i / w);
      uint32_t x_m = x % 64;
      uint32_t y_m = y % 32;
      DType v = getInputValue(i);
#ifdef VERBOSE_PRINT
    if (y_m < 3 || y_m >= 29) {
      if (x == 0) {
        std::cout << "\n";
        if (y_m == 0) {
          std::cout << "   +";
          if (sizeof(DType) * NChannels > 8) {
            for (uint32_t x = 0; x < (w / 64); ++x) {
              std::cout << "---------------------------------------------+";
            }
          }
          else {
            for (uint32_t x = 0; x < (w / 128); ++x) {
              std::cout << "-------------------------------------------------------------------------------------------+";
            }
          }
          std::cout << "\n";
        }
        std::cout << std::setw(3) << y << "|" << std::setw(6) << v;
      }
      else {
        if (x_m < 3 || x_m >= 61) {
          if (x_m == 0 && (x % 128 == 0 || sizeof(DType) * NChannels > 8))
            std::cout << "|";
          else std::cout << " ";
          std::cout << std::setw(6) << v;
        }
        else if (x_m == 3) std::cout << " ...";
        if (x == w - 1) std::cout << "|";
      }
    }
    else if (y_m < 6 && x == 0) std::cout << "\n . |   .";
#endif  
// #ifdef VERBOSE_PRINT
//       uint32_t y = i / w;
//       uint32_t x = i % w;
//       if (x % 8 == 0)
//         std::cout << "[" << std::setw(5) << i << "->("
//                   << std::setw(3) << x << ", " << std::setw(3)
//                   << y << ")]=" << std::setw(6) << v << " | ";
//       if (x == w - 1) std::cout << "\n";
// #endif
      for (int j = 0; j < NChannels; ++j) *p++ = v;
    }
  }
  // vkUnmapMemory(vk_device, stagingMem);

  printString("Submitting image layout transition");
  // Transition image layouts
  {
    VkImageMemoryBarrier barrierInput =
        vkutil::createImageMemoryBarrier(srcImg, 1 /*mipLevels*/);

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK_CALL(vkBeginCommandBuffer(vk_computeCmdBuffer, &cbbi));
    vkCmdPipelineBarrier(vk_computeCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrierInput);
    VK_CHECK_CALL(vkEndCommandBuffer(vk_computeCmdBuffer));

    VkSubmitInfo submission = {};
    submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submission.commandBufferCount = 1;
    submission.pCommandBuffers = &vk_computeCmdBuffer;

    VK_CHECK_CALL(vkQueueSubmit(vk_compute_queue, 1 /*submitCount*/,
                                &submission, VK_NULL_HANDLE /*fence*/));
    VK_CHECK_CALL(vkQueueWaitIdle(vk_compute_queue));
  }

#ifdef TEST_SEMAPHORE_IMPORT
  // Create semaphore to later import in SYCL
  printString("Creating semaphores");
  VkSemaphore syclWaitSemaphore;
  {
    VkExportSemaphoreCreateInfo esci = {};
    esci.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
#ifdef _WIN32
    esci.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    esci.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

    VkSemaphoreCreateInfo sci = {};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sci.pNext = &esci;
    VK_CHECK_CALL(
        vkCreateSemaphore(vk_device, &sci, nullptr, &syclWaitSemaphore));
  }
#endif // #ifdef TEST_SEMAPHORE_IMPORT

  printString("Copying staging memory to images");
  // Copy staging to main image memory
  {
    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkBufferImageCopy copyRegion = {};
    copyRegion.imageExtent = vkExtent;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.layerCount = 1;

    VK_CHECK_CALL(vkBeginCommandBuffer(vk_transferCmdBuffers[0], &cbbi));
    vkCmdCopyBufferToImage(vk_transferCmdBuffers[0], stagingBuf,
                           srcImg, VK_IMAGE_LAYOUT_GENERAL,
                           1 /*regionCount*/, &copyRegion);
    VK_CHECK_CALL(vkEndCommandBuffer(vk_transferCmdBuffers[0]));

    std::vector<VkPipelineStageFlags> stages{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};

    VkSubmitInfo submission = {};
    submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submission.commandBufferCount = 1;
    submission.pCommandBuffers = &vk_transferCmdBuffers[0];

#ifdef TEST_SEMAPHORE_IMPORT
    submission.signalSemaphoreCount = 1;
    submission.pSignalSemaphores = &syclWaitSemaphore;
#endif
    submission.pWaitDstStageMask = stages.data();

    VK_CHECK_CALL(vkQueueSubmit(vk_transfer_queue, 1 /*submitCount*/,
                                &submission, VK_NULL_HANDLE /*fence*/));
// Do not wait when using semaphores as they can handle the kernel execution
// order.
#ifndef TEST_SEMAPHORE_IMPORT
    VK_CHECK_CALL(vkQueueWaitIdle(vk_transfer_queue));
#endif
  }

  printString("Getting memory file descriptors");
  // Pass memory to SYCL for modification

#ifdef _WIN32
  auto input_mem_handle = vkutil::getMemoryWin32Handle(srcMem);
#else
  auto input_mem_handle = vkutil::getMemoryOpaqueFD(srcMem);
#endif

  printString("Getting semaphore interop handles");

#ifdef TEST_SEMAPHORE_IMPORT
  // Pass semaphores to SYCL for synchronization
#ifdef _WIN32
  auto sycl_wait_semaphore_handle =
      vkutil::getSemaphoreWin32Handle(syclWaitSemaphore);
#else
  auto sycl_wait_semaphore_handle =
      vkutil::getSemaphoreOpaqueFD(syclWaitSemaphore);
#endif
#else  // #ifdef TEST_SEMAPHORE_IMPORT
  void *sycl_wait_semaphore_handle = nullptr;
#endif // #ifdef TEST_SEMAPHORE_IMPORT

  printString("Calling into SYCL with interop memory handle");

  bool validated =
      run_sycl<decltype(input_mem_handle), decltype(sycl_wait_semaphore_handle),
               NDims, DType, NChannels, CType, KernelName>(
          syclQueue, dims, grpSize, input_mem_handle,
          sycl_wait_semaphore_handle);

  // Cleanup
  vkUnmapMemory(vk_device, stagingMem);
  vkDestroyBuffer(vk_device, stagingBuf, nullptr);
  vkDestroyImage(vk_device, srcImg, nullptr);
  vkFreeMemory(vk_device, stagingMem, nullptr);
  vkFreeMemory(vk_device, srcMem, nullptr);
#ifdef TEST_SEMAPHORE_IMPORT
  vkDestroySemaphore(vk_device, syclWaitSemaphore, nullptr);
#endif

  return validated;
}
//----------------------------------------------------------------------------//
bool run_tests() {
  bool valid = true;
  // clang-format off
#ifdef TEST_L0_SUPPORTED_VK_FORMAT
  valid &= run_test<1, float, 1, sycl_float, sycl_r, class fp32_1d_c1>
               ({1024}, {4});
  valid &= run_test<1, sycl::half, 2, sycl_half, sycl_rg, class fp16_1d_c2>
               ({1024}, {4});
  valid &= run_test<1, sycl::half, 4, sycl_half, sycl_rgba, class fp16_1d_c4>
               ({1024}, {4});
  valid &= run_test<1, uint8_t, 4, sycl_unorm8, sycl_rgba, class unorm_int8_1d_c4>
               ({1024}, {4});

  valid &= run_test<2, float, 1, sycl_float, sycl_r, class fp32_2d_c1>
               ({1024, 1024}, {16, 16});
  valid &= run_test<2, sycl::half, 2, sycl_half, sycl_rg, class fp16_2d_c2>
               ({1920, 1080}, {16, 8});
  valid &= run_test<2, sycl::half, 3, sycl_half, sycl_rgb, class fp16_2d_c3>
               ({2048, 2048}, {16, 16});
  valid &= run_test<2, uint8_t, 3, sycl_unorm8, sycl_rgb, class unorm_int8_2d_c3>
               ({2048, 2048}, {16, 16});
  valid &= run_test<2, sycl::half, 4, sycl_half, sycl_rgba, class fp16_2d_c4>
               ({2048, 2048}, {16, 16});
  valid &= run_test<2, uint8_t, 4, sycl_unorm8, sycl_rgba, class unorm_int8_2d_c4>
               ({2048, 2048}, {16, 16});

  valid &= run_test<3, float, 1, sycl_float, sycl_r, class fp32_3d_c1>
               ({1024, 1024, 16}, {16, 16, 1});
  valid &= run_test<3, sycl::half, 2, sycl_half, sycl_rg, class fp16_3d_c2>
               ({1920, 1080, 8}, {16, 8, 2});
  valid &= run_test<3, sycl::half, 4, sycl_half, sycl_rgba, class fp16_3d_c4>
               ({2048, 2048, 4}, {16, 16, 1});
  valid &= run_test<3, uint8_t, 4, sycl_unorm8, sycl_rgba, class unorm_int8_3d_c4>
               ({2048, 2048, 2}, {16, 16, 1});
#else
  // Debug tests - smaller sizes for quicker execution
  // valid &= run_test<1, uint8_t, 1, sycl_unorm8, sycl_r, class unorm8_1d_c1>
  //              ({1024}, {4});

  // valid &= run_test<1, int16_t, 1, sycl_sint16, sycl_rgba, class int16_1d_c1>
  //              ({1024}, {4});

  // valid &= run_test<1, float, 1, sycl_float, sycl_r, class fp32_1d_c1>
  //              ({1024}, {4});

  // valid &= run_test<1, float, 4, sycl_float, sycl_rgba, class fp32_1d_c4>
  //              ({1024}, {4});

  // valid &= run_test<2, uint8_t, 4, sycl_unorm8, sycl_rgba, class unorm8_2d_c4>
  //              ({32, 16}, {4, 2});

  // valid &= run_test<2, int16_t, 4, sycl_sint16, sycl_rgba, class int16_2d_c4>
  //              ({16, 16}, {2, 2});

  // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2>
  //              ({16, 16}, {2, 2});

  // valid &= run_test<2, float, 1, sycl_float, sycl_r, class float_2d_c1>
  //              ({16, 16}, {2, 2});

  // valid &= run_test<2, float, 2, sycl_float, sycl_rg, class float_2d_c2>
  //              ({16, 16}, {2, 2});

  // ******** Debug test cases: Commented test cases fail ********
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_08_16_22>
  //              ({8, 16}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_08_16_44>
  //              ({8, 16}, {4, 4});

  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_08_24_22>
  //              ({8, 24}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_08_24_44>
  //              ({8, 24}, {4, 4});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_10_16_22>
  // //              ({10, 16}, {2, 2});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_12_16_22>
  // //              ({12, 16}, {2, 2});

  // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_16_16_22>
  //              ({16, 16}, {2, 2});
  // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_16_16_44>
  //              ({16, 16}, {4, 4});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_16_22>
  // //              ({16, 16}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_16_44>
  // //              ({16, 16}, {4, 4});

  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_18_22>
  //              ({16, 18}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_18_42>
  //              ({16, 18}, {4, 2});

  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_20_22>
  //              ({16, 20}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_20_24>
  //              ({16, 20}, {2, 4});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_20_42>
  //              ({16, 20}, {4, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_20_44>
  //              ({16, 20}, {4, 4});

  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_24_22>
  //              ({16, 24}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_24_24>
  //              ({16, 24}, {2, 4});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_24_42>
  //              ({16, 24}, {4, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_24_44>
  //              ({16, 24}, {4, 4});

  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_32_22>
  //              ({16, 32}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_32_44>
  //              ({16, 32}, {4, 4});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_20_16_22>
  // //              ({20, 16}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_20_16_44>
  // //              ({20, 16}, {4, 4});

  // valid &= run_test<2, int16_t, 2, sycl_sint16, sycl_rg, class int16_2d_c2_20_20_22>
  //              ({20, 20}, {2, 2});
  // valid &= run_test<2, int16_t, 2, sycl_sint16, sycl_rg, class int16_2d_c2_20_20_44>
  //              ({20, 20}, {4, 4});
  // // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_20_20_22>
  // //              ({20, 20}, {2, 2});
  // // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_20_20_22>
  // //              ({20, 20}, {2, 2});
  // // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_20_20_44>
  // //              ({20, 20}, {4, 4});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_20_20_22>
  // //              ({20, 20}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_20_20_44>
  // //              ({20, 20}, {4, 4});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_40_22>
  // //              ({16, 40}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_40_44>
  // //              ({16, 40}, {4, 4});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_48_22>
  // //              ({16, 48}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_48_24>
  // //              ({16, 48}, {2, 4});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_16_48_48>
  // //              ({16, 48}, {4, 8});

  // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_24_24_22>
  //              ({24, 24}, {2, 2});
  // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_24_24_44>
  //              ({24, 24}, {4, 4});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_24_24_22>
  //              ({24, 24}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_24_24_44>
  //              ({24, 24}, {4, 4});

  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_24_32_22>
  //              ({24, 32}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_24_32_44>
  //              ({24, 32}, {4, 4});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_24_32_88>
  //              ({24, 32}, {8, 8});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_24_40_22>
  // //              ({24, 40}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_24_40_44>
  // //              ({24, 40}, {4, 4});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_24_40_88>
  // //              ({24, 40}, {8, 8});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_24_48_22>
  // //              ({24, 48}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_24_48_44>
  // //              ({24, 48}, {4, 4});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_24_48_88>
  // //              ({24, 48}, {8, 8});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_32_24_22>
  // //              ({32, 24}, {2, 2});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_32_16_22>
  // //              ({32, 16}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_32_16_44>
  // //              ({32, 16}, {4, 4});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_32_16_88>
  // //              ({32, 16}, {8, 8});

  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_32_32_22>
  //              ({32, 32}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_32_32_44>
  //              ({32, 32}, {4, 4});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_32_32_88>
  //              ({32, 32}, {8, 8});

  // // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_32_64_22>
  // //              ({32, 64}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_32_64_22>
  // //              ({32, 64}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_32_64_44>
  // //              ({32, 64}, {4, 4});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_32_64_88>
  // //              ({32, 64}, {8, 8});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_32_96_22>
  // //              ({32, 96}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_32_96_44>
  // //              ({32, 96}, {4, 4});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_32_96_88>
  // //              ({32, 96}, {8, 8});

  // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_48_24_22>
  //              ({48, 24}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_48_24_22>
  // //              ({48, 24}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_48_24_44>
  // //              ({48, 24}, {4, 4});

  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_48_32_22>
  //              ({48, 32}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_48_32_44>
  //              ({48, 32}, {4, 4});

  // // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_48_48_22>
  // //              ({48, 48}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_48_48_22>
  // //              ({48, 48}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_48_48_44>
  // //              ({48, 48}, {4, 4});

  // // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_48_64_22>
  // //              ({48, 64}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_48_64_22>
  // //              ({48, 64}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_48_64_44>
  // //              ({48, 64}, {4, 4});

  // // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_48_96_22>
  // //              ({48, 96}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_48_96_22>
  // //              ({48, 96}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_48_96_44>
  // //              ({48, 96}, {4, 4});

  // // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_48_128_22>
  // //              ({48, 128}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_48_128_22>
  // //              ({48, 128}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_48_128_44>
  // //              ({48, 128}, {4, 4});

  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_32_22>
  //              ({64, 32}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_32_44>
  //              ({64, 32}, {4, 4});

  // // valid &= run_test<2, int16_t, 2, sycl_sint16, sycl_rg, class int16_2d_c2_64_48_22>
  // //              ({64, 48}, {2, 2});
  // // valid &= run_test<2, int16_t, 4, sycl_sint16, sycl_rgba, class int16_2d_c4_64_48_22>
  // //              ({64, 48}, {2, 2});
  // // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_64_48_22>
  // //              ({64, 48}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_48_22>
  // //              ({64, 48}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_48_44>
  // //              ({64, 48}, {4, 4});

  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_64_22>
  //              ({64, 64}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_64_44>
  //              ({64, 64}, {4, 4});

  // // valid &= run_test<2, int16_t, 2, sycl_sint16, sycl_rg, class int16_2d_c2_64_80_22>
  // //              ({64, 80}, {2, 2});
  // // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_64_80_22>
  // //              ({64, 80}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_80_22>
  // //              ({64, 80}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_80_44>
  // //              ({64, 80}, {4, 4});

  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_96_22>
  //              ({64, 96}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_96_44>
  //              ({64, 96}, {4, 4});

  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_128_22>
  //              ({64, 128}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_128_44>
  //              ({64, 128}, {4, 4});

  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_160_22>
  //              ({64, 160}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_160_44>
  //              ({64, 160}, {4, 4});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_160_44>
  //              ({64, 160}, {8, 8});

  // // valid &= run_test<2, int16_t, 2, sycl_sint16, sycl_rg, class int16_2d_c2_64_176_44>
  // //              ({64, 176}, {4, 4});
  // // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_64_176_44>
  // //              ({64, 176}, {4, 4});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_176_22>
  // //              ({64, 176}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_176_44>
  // //              ({64, 176}, {4, 4});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_176_44>
  // //              ({64, 176}, {8, 8});

  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_192_22>
  //              ({64, 192}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_192_44>
  //              ({64, 192}, {4, 4});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_192_44>
  //              ({64, 192}, {8, 8});

  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_256_22>
  //              ({64, 256}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_256_44>
  //              ({64, 256}, {4, 4});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_64_256_44>
  //              ({64, 256}, {8, 8});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_72_256_22>
  // //              ({72, 256}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_72_256_44>
  // //              ({72, 256}, {4, 4});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_72_256_44>
  // //              ({72, 256}, {8, 8});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_80_80_22>
  // //              ({80, 80}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_80_80_44>
  // //              ({80, 80}, {4, 4});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_80_96_22>
  // //              ({80, 96}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_80_96_44>
  // //              ({80, 96}, {4, 4});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_80_128_22>
  // //              ({80, 128}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_80_128_44>
  // //              ({80, 128}, {4, 4});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_96_96_22>
  // //              ({96, 96}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_96_96_44>
  // //              ({96, 96}, {4, 4});

  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_96_128_22>
  // //              ({96, 128}, {2, 2});
  // // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_96_128_44>
  // //              ({96, 128}, {4, 4});

  // valid &= run_test<2, uint32_t, 1, sycl_uint32, sycl_r, class uint32_2d_c1_128_128_22>
  //              ({128, 128}, {2, 2});
  // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_128_128_22>
  //              ({128, 128}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_128_128_22>
  //              ({128, 128}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_128_128_44>
  //              ({128, 128}, {4, 4});

  // valid &= run_test<2, uint32_t, 1, sycl_uint32, sycl_r, class uint32_2d_c1_128_256_22>
  //              ({128, 256}, {2, 2});
  // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_128_256_22>
  //              ({128, 256}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_128_256_22>
  //              ({128, 256}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_128_256_44>
  //              ({128, 256}, {4, 4});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_128_256_44>
  //              ({128, 256}, {8, 8});

  // valid &= run_test<2, uint32_t, 1, sycl_uint32, sycl_r, class uint32_2d_c1_256_256_22>
  //              ({256, 256}, {2, 2});
  // valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_256_256_22>
  //              ({256, 256}, {2, 2});
  // valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_256_256_22>
  //              ({256, 256}, {2, 2});

  valid &= run_test<2, uint32_t, 1, sycl_uint32, sycl_r, class uint32_2d_c1_512_256_22>
               ({512, 256}, {2, 2});
  valid &= run_test<2, uint32_t, 2, sycl_uint32, sycl_rg, class uint32_2d_c2_512_256_22>
               ({512, 256}, {2, 2});
  valid &= run_test<2, uint32_t, 4, sycl_uint32, sycl_rgba, class uint32_2d_c4_512_256_22>
               ({512, 256}, {2, 2});
  // ******** End of debug tests ********

  // valid &= run_test<2, float, 4, sycl_float, sycl_rgba, class float_2d>
  //              ({16, 16}, {2, 2});

  // valid &= run_test<2, float, 2, sycl_float, sycl_rg, class float_2d_large>
  //              ({1024, 1024}, {4, 2});

  // valid &= run_test<3, char, 2, sycl_sint8, sycl_rg, class int8_3d>
  //              ({256, 16, 2}, {2, 2, 2});

  // valid &= run_test<2, uint32_t, 1, sycl_uint32, sycl_r, class uint32_2d>
  //              ({64, 32}, {4, 2});

  // valid &= run_test<3, uint32_t, 4, sycl_uint32, sycl_rgba, class uint_3d_large>
  //              ({1024, 256, 16}, {2, 2, 4});

  // valid &= run_test<2, int32_t, 1, sycl_sint32, sycl_r, class int32_2d>
  //              ({64, 32}, {4, 2});

  // valid &= run_test<3, int32_t, 2, sycl_sint32, sycl_rg, class int32_3d>
  //              ({64, 32, 64}, {4, 2, 4});

  // valid &= run_test<3, int16_t, 1, sycl_sint16, sycl_r, class int16_3d>
  //              ({64, 32, 64}, {4, 2, 4});
#endif
  // clang-format on
  return valid;
}
//----------------------------------------------------------------------------//
int main() {

  if (vkutil::setupInstance() != VK_SUCCESS) {
    std::cerr << "Instance setup failed!\n";
    return EXIT_FAILURE;
  }

  sycl::device dev;

  if (vkutil::setupDevice(dev) != VK_SUCCESS) {
    std::cerr << "Device setup failed!\n";
    return EXIT_FAILURE;
  }

  if (vkutil::setupCommandBuffers() != VK_SUCCESS) {
    std::cerr << "Compute pipeline setup failed!\n";
    return EXIT_FAILURE;
  }

  bool result_ok = run_tests();

  if (vkutil::cleanup() != VK_SUCCESS) {
    std::cerr << "Cleanup failed!\n";
    return EXIT_FAILURE;
  }

  if (result_ok) {
    std::cout << "All tests passed!\n";
    return EXIT_SUCCESS;
  }

  std::cerr << "Tests failed\n";
  return EXIT_FAILURE;
}
//----------------------------------------------------------------------------//
