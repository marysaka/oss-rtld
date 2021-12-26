use core::fmt::Write;
use core::panic::PanicInfo;

use crate::rtld::Module;

use super::syscall::{break_, return_from_exception, BreakReason};

#[panic_handler]
fn panic(panic_info: &PanicInfo<'_>) -> ! {
    write!(&mut super::KernelWritter, "PANIC: {}", panic_info).ok();

    unsafe {
        break_(BreakReason::Panic, 0, 0);
    }

    loop {}
}

extern "C" {
    static mut __bss_start__: u8;
    static mut __bss_end__: u8;
    static __dynamic_start__: u8;
    static __nx_mod0: Module;
}

#[link_section = ".text.crt0"]
#[naked]
#[no_mangle]
unsafe extern "C" fn __module_start() -> ! {
    asm!(
        "
        b {}
        .word __nx_mod0 - __module_start
        ",
        sym shim_entrypoint,
        options(noreturn),
    )
}

#[naked]
unsafe extern "C" fn shim_entrypoint() -> ! {
    asm!(
        "
        // Check if we entered because of an exception
        cmp x0, #0
        bne {}
        // Preserve the thread handle
        mov w19, w1
        bl {}
        b .
        ",
        sym entry_exception_shim,
        sym entry_normal_shim,
        options(noreturn),
    )
}

#[naked]
unsafe extern "C" fn entry_normal_shim() -> ! {
    asm!(
        "
        // Clean .bss
        adrp x0, {0}
        add x0, x0, #:lo12:{0}
        adrp x1, {1}
        add x1, x1, #:lo12:{1}
        bl {2}

        // Relocate current module
        adrp x0, {3}
        add x0, x0, #:lo12:{3}
        adrp x1, {4}
        add x1, x1, #:lo12:{4}
        bl {5}

        // Restore the thread handle
        adrp x0, {3}
        add x0, x0, #:lo12:{3}
        mov w1, w19
        // Now jump to main
        bl {6}
        b .
        ",
        sym __bss_start__,
        sym __bss_end__,
        sym clean_bss,
        sym __module_start,
        sym __dynamic_start__,
        sym crate::rtld::relocate_self,
        sym crate::main,
        options(noreturn),
    )
}

unsafe extern "C" fn clean_bss(start_bss: *mut u8, end_bss: *mut u8) {
    core::ptr::write_bytes(
        start_bss,
        0,
        end_bss as *const _ as usize - start_bss as *const _ as usize,
    );
}

// TODO: Support user exception handler here.
#[naked]
pub unsafe extern "C" fn entry_exception_shim() -> ! {
    asm!(
        "
        mov w0, 0xF801
        bl {}
        b .
        ",
        sym return_from_exception,
        options(noreturn),
    )
}