// REQUIRES: aspect-ext_oneapi_external_semaphore_import
// REQUIRES: windows
// REQUIRES: level_zero

// RUN: %{build} %link-directx -o %t.exe %if target-spir %{ -Wno-ignored-attributes %}
// RUN: %{run} %t.exe

// clang-format off
/*
  Minimal reproducer for a driver hang when a D3D12 GPU-queue Wait is
  unblocked by a SYCL ext_oneapi_signal_external_semaphore submitted
  entirely asynchronously (no q.wait() between the SYCL signal submission
  and the D3D12 GPU-side cmdQueue->Wait).

  This replicates the exact semaphore protocol used by OIDN's
  signalSemaphoreAsync / waitSemaphoreAsync with D3D12 fences in
  ChameleonRT's DXR backend (render_dxr.cpp):

    GPU queue:  [render work] --> Signal(fence, N)
                              --> Wait(fence, N+1)     <- must be unblocked by SYCL
                              --> Signal(fence, N+2)
    SYCL queue: wait_external_semaphore(sem, N)
                --> trivial kernel
                --> signal_external_semaphore(sem, N+1)  <- must unblock GPU Wait

  The CPU enqueues all D3D12 GPU-side commands first, then submits the
  SYCL work, and immediately waits for fence value N+2 with a 5-second
  timeout.  If the SYCL async signal does not reach the D3D12 fence the
  CPU wait times out and the test reports a hang.

  The existing D3D12_sycl_buffer_timeline_semaphore.cpp test calls
  q.wait() before d3dCtx.cmdQueue->Wait, which hides this bug because by
  that point the SYCL signal has already been delivered.  This test omits
  that CPU synchronization point to expose the failure.

  Build:
    clang++.exe -fsycl -o hang_repro.exe D3D12_sycl_fence_signal_hangs.cpp ^
        -ld3d12 -ldxgi
*/
// clang-format on

#include "d3d12_setup.hpp"
#include <iostream>
#include <sycl/detail/core.hpp>
#include <sycl/ext/oneapi/bindless_images.hpp>
#include <sycl/properties/queue_properties.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace syclexp = sycl::ext::oneapi::experimental;

// Timeout in milliseconds for the final CPU wait. 500 milliseconds is more than
// enough for a trivial kernel; a hang will blow past this.
static constexpr DWORD kWaitTimeoutMs = 500;

int main() {
    std::cout << "D3D12/SYCL async fence-signal hang reproducer\n";
    std::cout << "----------------------------------------------\n";

    // ---------------------------------------------------------------
    // D3D12 setup
    // ---------------------------------------------------------------
    D3D12Context d3dCtx = createD3D12Context();

    // The exportable fence is the shared synchronization object.
    // Must be created with D3D12_FENCE_FLAG_SHARED so a Win32 NT handle
    // can be exported and imported by SYCL.
    D3D12ExportableFence extFence = createExportableFence(d3dCtx);

    // A separate plain fence is used for the CPU-side readback wait so
    // that the two fence objects stay independent and we don't accidentally
    // advance the shared fence from the CPU.
    ComPtr<ID3D12Fence> cpuFence;
    HANDLE cpuFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    uint64_t cpuFenceValue = 0;
    ThrowIfFailed(
        d3dCtx.device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                   IID_PPV_ARGS(&cpuFence)),
        "Failed to create CPU fence");

    // ---------------------------------------------------------------
    // SYCL setup -- must use an immediate command list queue for
    // external semaphore ops.
    // ---------------------------------------------------------------
    sycl::queue q{sycl::property_list{
        sycl::property::queue::in_order{},
        sycl::ext::intel::property::queue::immediate_command_list{}}};

    auto syclDevice  = q.get_device();
    auto syclContext = q.get_context();

    std::cout << "[SYCL] Device: "
              << syclDevice.get_info<sycl::info::device::name>() << "\n";

    // Import the shared D3D12 fence into SYCL.
    auto semDesc =
        syclexp::external_semaphore_descriptor<syclexp::resource_win32_handle>{
            extFence.sharedHandle,
            syclexp::external_semaphore_handle_type::win32_nt_dx12_fence};
    std::cout << "[SYCL] Importing external semaphore from D3D12 fence handle\n";
    syclexp::external_semaphore syclSem =
        syclexp::import_external_semaphore(semDesc, syclDevice, syclContext);

    std::cout << "[SYCL] Imported external semaphore from D3D12 fence handle: "
              << extFence.sharedHandle << "\n";

    // ---------------------------------------------------------------
    // The protocol (matches ChameleonRT render_dxr.cpp):
    //
    //   N   = 1   (D3D12 GPU signals after "render" work)
    //   N+1 = 2   (SYCL signals after "denoise" work)
    //   N+2 = 3   (D3D12 GPU signals after "tonemap" work; CPU waits here)
    //
    // All D3D12 GPU-side commands are enqueued first.  Then the SYCL
    // work is submitted asynchronously -- NO q.wait() before the D3D12
    // GPU-side Wait(N+1).
    // Step 1: D3D12 GPU side -- enqueue Signal(N), Wait(N+1), Signal(N+2)
    std::cout << "[D3D12] cmdQueue->Signal(extFence, " << N << ")\n"
    ThrowIfFailed(
        d3dCtx.cmdQueue->Signal(extFence.fence.Get(), N),
        "cmdQueue->Signal(N) failed");

    // Wait(N+1): GPU-side stall until SYCL delivers the signal.
    // This returns immediately on the CPU; it only blocks the GPU queue.
    std::cout << "[D3D12] cmdQueue->Wait(extFence, " << (N + 1) << ")  "
              << "[GPU-side wait, CPU continues immediately]\n"
              << std::flush;
    ThrowIfFailed(
        d3dCtx.cmdQueue->Wait(extFence.fence.Get(), N + 1),
        "cmdQueue->Wait(N+1) failed");

    // Signal(N+2): simulates post-denoise tonemap/readback work.
    // Will never execute if Wait(N+1) is never satisfied.
    std::cout << "[D3D12] cmdQueue->Signal(extFence, " << (N + 2) << ")\n"
              << std::flush;
    ThrowIfFailed(
        d3dCtx.cmdQueue->Signal(extFence.fence.Get(), N + 2),
        "cmdQueue->Signal(N+2) failed");

    // Step 2: SYCL -- submit async: wait(N) --> kernel --> signal(N+1)
    // Crucially, NO q.wait() is called here before the CPU timeout wait
    // below.  This is the exact pattern used by OIDN's async interop.
    std::cout << "[SYCL] ext_oneapi_wait_external_semaphore(syclSem, " << N
              << ")\n"
              << std::flush;
    q.ext_oneapi_wait_external_semaphore(syclSem, N);

    // Submit a no-op kernel to represent the "denoise" work between the
    // wait and the signal (matches OIDN's executeAsync slot).
    q.submit([&](sycl::handler &h) { h.single_task([=]() {}); });

    std::cout << "[SYCL] ext_oneapi_signal_external_semaphore(syclSem, "
              << (N + 1) << ")\n"
              << std::flush;
    q.ext_oneapi_signal_external_semaphore(syclSem, N + 1);

    // Step 3: CPU waits for fence value N+2.  This requires:
    //   a) SYCL signals fence to N+1  (unblocks GPU Wait(N+1))
    //   b) D3D12 GPU executes Signal(N+2)
    //   c) CPU SetEventOnCompletion fires
    //
    // If (a) never happens the GPU queue stalls at Wait(N+1) and the
    // CPU blocks forever (or until the timeout below).
    std::cout << "[D3D12] CPU: SetEventOnCompletion(" << (N + 2) << ") + "
              << "WaitForSingleObject (timeout=" << kWaitTimeoutMs << "ms)\n"
              << std::flush;

    ThrowIfFailed(
        extFence.fence->SetEventOnCompletion(N + 2, cpuFenceEvent),
        "SetEventOnCompletion(N+2) failed");

    DWORD waitResult = WaitForSingleObject(cpuFenceEvent, kWaitTimeoutMs);

    if (waitResult == WAIT_TIMEOUT) {
        std::cerr
            << "\n[FAIL] TIMEOUT -- fence never reached " << (N + 2) << ".\n"
            << "       extFence.fence->GetCompletedValue() = "
            << extFence.fence->GetCompletedValue() << "\n"
            << "       The D3D12 GPU queue is stuck at Wait(extFence, "
            << (N + 1) << ").\n"
            << "       SYCL's async ext_oneapi_signal_external_semaphore("
            << (N + 1) << ") did not\n"
            << "       advance the D3D12 fence, so the GPU-side Wait was "
               "never satisfied.\n";
        // Cleanup as much as possible before exit
        syclexp::release_external_semaphore(syclSem, syclDevice, syclContext);
        cleanupExportableFence(extFence);
        CloseHandle(cpuFenceEvent);
        return 1;
    }

    ThrowIfFailed(waitResult == WAIT_OBJECT_0 ? S_OK : E_FAIL,
                  "WaitForSingleObject returned unexpected value");

    std::cout << "[PASS] Fence reached " << (N + 2)
              << " -- SYCL async signal successfully unblocked the D3D12 GPU "
                 "queue.\n";

    // ---------------------------------------------------------------
    // Cleanup
    // ---------------------------------------------------------------
    q.wait();
    syclexp::release_external_semaphore(syclSem, syclDevice, syclContext);
    cleanupExportableFence(extFence);
    CloseHandle(cpuFenceEvent);
    return 0;
}