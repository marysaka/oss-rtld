use core::arch::global_asm;
use core::fmt::Write;
use core::panic::PanicInfo;

use super::syscall::{break_, BreakReason};

#[panic_handler]
fn panic(panic_info: &PanicInfo<'_>) -> ! {
    write!(&mut super::KernelWritter, "PANIC: {}", panic_info).ok();

    unsafe {
        break_(BreakReason::Panic, 0, 0);
    }

    loop {}
}

// TODO: Arch selection
global_asm!(include_str!("aarch64/crt0.s"));

#[no_mangle]
unsafe extern "C" fn __rtld_clean_bss(start_bss: *mut u8, end_bss: *mut u8) {
    core::ptr::write_bytes(
        start_bss,
        0,
        end_bss as *const _ as usize - start_bss as *const _ as usize,
    );
}
