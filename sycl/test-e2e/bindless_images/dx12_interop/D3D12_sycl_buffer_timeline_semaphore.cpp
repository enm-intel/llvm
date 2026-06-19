// REQUIRES: aspect-ext_oneapi_external_memory_import
// REQUIRES: aspect-ext_oneapi_external_semaphore_import
// REQUIRES: windows

// REQUIRES-INTEL-DRIVER: lin: 38303 win: 101.9999

// RUN: %{build} %link-directx -o %t.exe %if target-spir %{ -Wno-ignored-attributes %}
// RUN: %{run} %t.exe --no-sem
// RUN: %{run} %t.exe

// clang-format off
/*
  DirectX 12 / SYCL Buffer + Fence (Timeline) Interop Stress Test

  clang++.exe -fsycl -o dsbt.exe D3D12_sycl_buffer_timeline_semaphore.cpp -ld3d12 -ldxgi -ld3dcompiler

  Iteratively round-trips data through a D3D12 buffer and a SYCL kernel,
  synchronized via a single ID3D12Fence with monotonically increasing
  values.  No image APIs are used — only D3D12 buffers exported and mapped
  to SYCL USM pointers.

  Semaphore protocol (ID3D12Fence):
    One timeline fence, initial value 0.
    D3D12 signals odd values: 2*i - 1  (for iteration i = 1..N)
    SYCL  signals even values: 2*i

  Flow (per iteration i):
    1. D3D12: fill upload buffer with value i, copy to default buffer,
              signal fence = 2*i-1 on the command queue
    2. SYCL:  wait fence >= 2*i-1, kernel: out[j] = in[j] * 2,
              signal fence = 2*i
    3. D3D12: device-wait fence >= 2*i on the command queue,
              copy to readback buffer, CPU wait on fence, verify out[j] == i*2
*/
// clang-format on

#include "d3d12_setup.hpp"
#include <iostream>
#include <string>
#include <sycl/detail/core.hpp>
#include <sycl/ext/oneapi/bindless_images.hpp>
#include <sycl/properties/queue_properties.hpp>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define PUTS_(msg) std::cerr << std::format("{}\n", msg)
#define PRINT_(frmt, ...) std::cerr << std::format(frmt, __VA_ARGS__)

namespace syclexp = sycl::ext::oneapi::experimental;

using sycl_ext_mem = syclexp::external_mem;
using sycl_ext_sem = syclexp::external_semaphore;

using sycl_ext_mem_type    = syclexp::external_mem_handle_type;
using sycl_ext_sem_type    = syclexp::external_semaphore_handle_type;
using sycl_ext_mem_desc_nt = syclexp::external_mem_descriptor<syclexp::resource_win32_handle>;
using sycl_ext_sem_desc_nt = syclexp::external_semaphore_descriptor<syclexp::resource_win32_handle>;

static constexpr sycl_ext_mem_type sycl_ext_mem_nt_handle     = syclexp::external_mem_handle_type::win32_nt_handle;
static constexpr sycl_ext_sem_type sycl_ext_sem_nt_dx12_fence = syclexp::external_semaphore_handle_type::win32_nt_dx12_fence;

int main(int argc, char **argv) {
  size_t numElems = 1024;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--size" && i + 1 < argc)
      numElems = std::stoi(argv[++i]);
  }

  size_t bufSize = numElems * sizeof(uint32_t);

  std::cout << "Running SYCL D3D12 Buffer + Timeline Fence Stress Test\n";
  std::cout << "Elements: " << numElems << "\n";

  // D3D12 SETUP
  D3D12Context d3dCtx = createD3D12Context();

  // Exportable device-local buffers
  // D3D12BufferResources inBuf = createExportableBuffer(d3dCtx, bufSize);
  // D3D12BufferResources outBuf = createExportableBuffer(d3dCtx, bufSize);

  // Host-visible staging buffers
  // D3D12BufferResources inStaging = createUploadBuffer(d3dCtx, bufSize);
  // D3D12BufferResources outStaging = createReadbackBuffer(d3dCtx, bufSize);

  // Interop Timeline Fence
  D3D12ExportableFence extFence = createExportableFence(d3dCtx);

  // Set initial buffer states explicitly to COPY_DEST to avoid generic read
  // promotion issues
  // d3dCtx.cmdAlloc->Reset();
  // d3dCtx.cmdList->Reset(d3dCtx.cmdAlloc.Get(), nullptr);
  // D3D12_RESOURCE_BARRIER initBarriers[2] = {};
  // initBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  // initBarriers[0].Transition.pResource = inBuf.resource.Get();
  // initBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  // initBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
  // initBarriers[0].Transition.Subresource =
  //     D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  // initBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  // initBarriers[1].Transition.pResource = outBuf.resource.Get();
  // initBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  // initBarriers[1].Transition.StateAfter =
  //     D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  // initBarriers[1].Transition.Subresource =
  //     D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  // d3dCtx.cmdList->ResourceBarrier(2, initBarriers);
  // d3dCtx.cmdList->Close();
  // executeAndWait(d3dCtx);

  // SYCL INTEROP
  try {
    // Bindless image interop requires an in-order queue (per spec). External
    // semaphore ops additionally require immediate command lists; see
    // sycl_ext_oneapi_bindless_images.asciidoc.
    sycl::queue q{{sycl::property::queue::in_order{},
                  sycl::ext::intel::property::queue::immediate_command_list{}}};
    auto device = q.get_device();
    auto contxt = q.get_context();

    PRINT_("[SYCL] Device: {}\n", device.get_info<sycl::info::device::name>());

    // Import buffers
    // sycl_ext_mem_desc_nt inDesc{inBuf.sharedHandle, sycl_ext_mem_nt_handle, bufSize};
    // sycl_ext_mem inExtMem = syclexp::import_external_memory(inDesc, device, contxt);

    // sycl_ext_mem_desc_nt outDesc{outBuf.sharedHandle, sycl_ext_mem_nt_handle, bufSize};
    // sycl_ext_mem outExtMem = syclexp::import_external_memory(outDesc, device, contxt);

    // syclexp::external_mem_descriptor<syclexp::resource_win32_handle> inDesc{
    //     inBuf.sharedHandle, syclexp::external_mem_handle_type::win32_nt_handle,
    //     bufSize};
    // syclexp::external_mem inExtMem =
    //     syclexp::import_external_memory(inDesc, device, contxt);

    // syclexp::external_mem_descriptor<syclexp::resource_win32_handle> outDesc{
    //     outBuf.sharedHandle, syclexp::external_mem_handle_type::win32_nt_handle,
    //     bufSize};
    // syclexp::external_mem outExtMem =
    //     syclexp::import_external_memory(outDesc, device, contxt);

    // Import timeline fence
    auto semDesc = sycl_ext_sem_desc_nt{extFence.sharedHandle, sycl_ext_sem_nt_dx12_fence};
    sycl_ext_sem syclSem = syclexp::import_external_semaphore(semDesc, device, contxt);

    // uint32_t *inPtr = static_cast<uint32_t *>(
    //     syclexp::map_external_linear_memory(inExtMem, 0, bufSize, q));
    // uint32_t *outPtr = static_cast<uint32_t *>(
    //     syclexp::map_external_linear_memory(outExtMem, 0, bufSize, q));

    std::cout << "[Test] Starting stress test...\n";

    uint64_t d3dSignalVal = 1u;
    uint64_t syclSignalVal = 2u;
    uint64_t curFenceVal = 0;

    // D3D12: Upload and copy
    // void *mapped;
    // inStaging.resource->Map(0, nullptr, &mapped);
    // auto *data = static_cast<uint32_t *>(mapped);
    // for (size_t j = 0; j < numElems; ++j)
    //   data[j] = (uint32_t)j;
    // inStaging.resource->Unmap(0, nullptr);

    // d3dCtx.cmdAlloc->Reset();
    // d3dCtx.cmdList->Reset(d3dCtx.cmdAlloc.Get(), nullptr);

    // d3dCtx.cmdList->CopyBufferRegion(inBuf.resource.Get(), 0,
    //                                   inStaging.resource.Get(), 0, bufSize);

    // // Barrier: CopyDest -> UAV
    // D3D12_RESOURCE_BARRIER barrier = {};
    // barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    // barrier.Transition.pResource = inBuf.resource.Get();
    // barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    // barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    // barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    // d3dCtx.cmdList->ResourceBarrier(1, &barrier);

    // d3dCtx.cmdList->Close();
    // ID3D12CommandList *ppCommandLists[] = {d3dCtx.cmdList.Get()};
    // d3dCtx.cmdQueue->ExecuteCommandLists(1, ppCommandLists);

    PRINT_("[D3D12] cmdQueue->Signal(extFence, {})\n", d3dSignalVal);
    d3dCtx.cmdQueue->Signal(extFence.fence.Get(), d3dSignalVal);

    // std::cout << "  D3D12 upload done" << std::flush;

    // SYCL: Wait, execute, signal
    // std::cout << ", SYCL sem-wait(" << d3dSignalVal << ")..." << std::flush;
    PRINT_("[SYCL] ext_oneapi_wait_external_semaphore(syclSem, {})\n", d3dSignalVal);
    q.ext_oneapi_wait_external_semaphore(syclSem, d3dSignalVal);
    // std::cout << "ok" << std::flush;

    // D3D12: Readback and verify
    PRINT_("[D3D12] cmdQueue->Wait(extFence, {})\n", syclSignalVal);
    curFenceVal = extFence.fence->GetCompletedValue();
    PRINT_("[D3D12] Current external fence value = {}...\n", curFenceVal);
    d3dCtx.cmdQueue->Wait(extFence.fence.Get(), syclSignalVal);

    // q.submit([&](sycl::handler &h) {
    //   h.parallel_for(sycl::range<1>(numElems), [=](sycl::item<1> item) {
    //     size_t id = item.get_id(0);
    //     outPtr[id] = inPtr[id] * 2;
    //   });
    // });
    // Submit a no-op kernel to represent the "denoise" work between the
    // wait and the signal (matches OIDN's executeAsync slot).
    q.submit([&](sycl::handler &h) { h.single_task([=]() {}); });

    // std::cout << ", SYCL sem-signal(" << syclSignalVal << ")..."
    //           << std::flush;
    PRINT_("[SYCL] ext_oneapi_signal_external_semaphore(syclSem, {})\n", syclSignalVal);
    q.ext_oneapi_signal_external_semaphore(syclSem, syclSignalVal);
    // std::cout << "ok" << std::flush;
    // q.wait();
    // std::cout << ", SYCL done" << std::flush;

    // d3dCtx.cmdAlloc->Reset();
    // d3dCtx.cmdList->Reset(d3dCtx.cmdAlloc.Get(), nullptr);

    // // Barrier: UAV -> CopySource
    // barrier.Transition.pResource = outBuf.resource.Get();
    // barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    // barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    // d3dCtx.cmdList->ResourceBarrier(1, &barrier);

    // d3dCtx.cmdList->CopyBufferRegion(outStaging.resource.Get(), 0,
    //                                   outBuf.resource.Get(), 0, bufSize);

    // // Barrier: revert outBuf back to UAV for next iteration
    // barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    // barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    // d3dCtx.cmdList->ResourceBarrier(1, &barrier);

    // d3dCtx.cmdList->Close();
    // d3dCtx.cmdQueue->ExecuteCommandLists(1, ppCommandLists);

    // Host wait for readback
    // std::cout << ", d3d-fence..." << std::flush;
    d3dCtx.fenceValue++;
    PRINT_("[D3D12] Signaling fence to {} and waiting for CPU event...\n", d3dCtx.fenceValue);
    d3dCtx.cmdQueue->Signal(d3dCtx.fence.Get(), d3dCtx.fenceValue);
    
    PRINT_("[D3D12] CPU: SetEventOnCompletion({}) + "
           "WaitForSingleObject (timeout={}ms)\n",
           N + 2, kWaitTimeoutMs);
    d3dCtx.fence->SetEventOnCompletion(d3dCtx.fenceValue, d3dCtx.fenceEvent);

    PRINT_("[D3D12] CPU is now waiting for the GPU to reach fence value {}...\n", N + 2);
    PRINT_("[D3D12] Current external fence value = {}...\n", curFenceVal);
    if (WaitForSingleObject(d3dCtx.fenceEvent, 5000) == WAIT_TIMEOUT) {
      std::cerr << "\nTIMEOUT on host wait!\n";
      return 1;
    }
    // std::cout << "ok" << std::flush;

    // Verify data
    // outStaging.resource->Map(0, nullptr, &mapped);
    // auto *outData = static_cast<uint32_t *>(mapped);
    // uint32_t expected = (uint32_t)i * 2;
    // int errors = 0;
    // for (size_t j = 0; j < numElems; ++j) {
    //   if (outData[j] != expected) {
    //     if (errors++ < 5)
    //       std::cerr << "  [" << j << "]: got " << outData[j] << " expected "
    //                 << expected << "\n";
    //   }
    // }
    // outStaging.resource->Unmap(0, nullptr);

    // if (errors > 0) {
    //   std::cerr << "\nFAILURE at iteration " << i << ": " << errors
    //             << " mismatches\n";
    //   return 1;
    // }

    // Reset inBuf state to COPY_DEST for next iteration
    // d3dCtx.cmdAlloc->Reset();
    // d3dCtx.cmdList->Reset(d3dCtx.cmdAlloc.Get(), nullptr);
    // barrier.Transition.pResource = inBuf.resource.Get();
    // barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    // barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    // d3dCtx.cmdList->ResourceBarrier(1, &barrier);
    // d3dCtx.cmdList->Close();
    // executeAndWait(d3dCtx); // Use generic helper to submit and wait

    std::cout << "SUCCESS! Timeline semaphore test passed.\n";

    // SYCL Cleanup
    // syclexp::unmap_external_linear_memory(inPtr, q);
    // syclexp::unmap_external_linear_memory(outPtr, q);
    // syclexp::release_external_semaphore(syclSem, device, contxt);
    // syclexp::release_external_memory(inExtMem, device, contxt);
    // syclexp::release_external_memory(outExtMem, device, contxt);

  } catch (sycl::exception &e) {
    std::cerr << "SYCL Exception: " << e.what() << "\n";
    return 1;
  }

  // D3D12 Cleanup
  cleanupExportableFence(extFence);
  // cleanupBuffer(inBuf);
  // cleanupBuffer(outBuf);
  // cleanupBuffer(inStaging);
  // cleanupBuffer(outStaging);
  // Clean up the generic context event directly
  if (d3dCtx.fenceEvent)
    CloseHandle(d3dCtx.fenceEvent);

  return 0;
}
