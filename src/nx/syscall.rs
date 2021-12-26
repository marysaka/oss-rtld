use core::arch::asm;
use modular_bitfield::prelude::*;

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
#[repr(u32)]
#[allow(dead_code)]
pub enum BreakReason {
    Panic = 0,
    Assert = 1,
    User = 2,
    PreLoadDll = 3,
    PostLoadDll = 4,
    PreUnloadDll = 5,
    PostUnloadDll = 6,
    CppException = 7,
    NotificationOnlyFlag = 0x80000000,
}

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
#[repr(u32)]
#[allow(dead_code)]
pub enum MemoryState {
    Free = 0,
    Io = 1,
    Static = 2,
    Code = 3,
    CodeData = 4,
    Normal = 5,
    Shared = 6,
    Alias = 7,
    AliasCode = 8,
    AliasCodeData = 9,
    Ipc = 10,
    Stack = 11,
    ThreadLocal = 12,
    Transfered = 13,
    SharedTransfered = 14,
    SharedCode = 15,
    Inaccessible = 16,
    NonSecureIpc = 17,
    NonDeviceIpc = 18,
    Kernel = 19,
    GeneratedCode = 20,
    CodeOut = 21,
    Coverage = 22,
}

impl Default for MemoryState {
    fn default() -> Self {
        MemoryState::Free
    }
}

#[bitfield]
#[derive(Debug, Default, Clone, Copy)]
#[repr(u32)]
#[allow(dead_code)]
pub struct MemoryPermission {
    pub read: bool,
    pub write: bool,
    pub execute: bool,
    reserved: B29,
}

#[bitfield]
#[derive(Debug, Default, Clone, Copy)]
#[repr(u32)]
#[allow(dead_code)]
pub struct MemoryAttribute {
    pub locked: bool,
    pub ipc_locked: bool,
    pub device_shared: bool,
    pub uncached: bool,
    reserved: B28,
}

#[derive(Debug, Default, Clone, Copy)]
#[repr(C)]
pub struct MemoryInfo {
    pub address: u64,
    pub size: u64,
    pub state: MemoryState,
    pub attribute: MemoryAttribute,
    pub permission: MemoryPermission,
    pub device_ref_count: u32,
    pub ipc_ref_count: u32,
    pub padding: u32,
}

pub unsafe fn query_memory(
    memory_info: *mut MemoryInfo,
    page_info: *mut u32,
    address: usize,
) -> u32 {
    let mut result;
    let mut page_info_raw;

    asm!(
        "svc 0x6",
        in("x0") memory_info,
        in("x2") address,
        lateout("w0") result,
        out("w1") page_info_raw,
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
        in("w0") break_reason as u32,
        in("x1") address,
        in("x2") size,
    )
}

pub unsafe fn output_debug_string(debug_str: *const u8, debug_str_len: usize) {
    asm!(
        "svc 0x27",
        in("x0") debug_str,
        in("x1") debug_str_len
    )
}

pub unsafe fn return_from_exception(result_code: u32) {
    asm!(
        "svc 0x28",
        in("x0") result_code
    )
}
