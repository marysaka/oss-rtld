use crate::nx::common::*;
use core::arch::asm;

pub unsafe fn query_memory(
    memory_info: *mut MemoryInfo,
    page_info: *mut u32,
    address: usize,
) -> u32 {
    let mut result;
    let mut page_info_raw;

    asm!(
        "svc 0x6",
        in("r0") memory_info,
        in("r2") address,
        lateout("r0") result,
        out("r1") page_info_raw,
    );

    *page_info = page_info_raw;

    result
}

pub unsafe fn exit_process() -> ! {
    asm!("svc 0x7", options(noreturn))
}

pub unsafe fn break_(break_reason: BreakReason, address: usize, size: usize) {
    asm!(
        "svc 0x26",
        in("r0") break_reason as u32,
        in("r1") address,
        in("r2") size,
    )
}

pub unsafe fn output_debug_string(debug_str: *const u8, debug_str_len: usize) {
    asm!(
        "svc 0x27",
        in("r0") debug_str,
        in("r1") debug_str_len
    )
}

pub unsafe fn return_from_exception(result_code: u32) {
    asm!(
        "svc 0x28",
        in("r0") result_code
    )
}
