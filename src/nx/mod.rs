use core::arch::global_asm;
use core::fmt::Write;

pub mod rt;
pub mod syscall;

#[derive(Debug, Clone, Copy)]
pub struct KernelWritter;

impl Write for KernelWritter {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        unsafe {
            let bytes = s.as_bytes();

            syscall::output_debug_string(bytes.as_ptr(), bytes.len())
        }

        Ok(())
    }
}

// MISC: Setup module
// TODO: Move to full Rust here.
global_asm!(include_str!("header.s"));

// MISC: Set application name
#[repr(packed)]
#[allow(unused_variables)]
pub struct ModuleName<const LEN: usize> {
    pub version: u32,
    pub name_length: u32,
    pub name: [u8; LEN],
}

impl<const LEN: usize> ModuleName<LEN> {
    pub const fn new(bytes: &[u8; LEN]) -> Self {
        Self {
            version: 0,
            name_length: LEN as u32 - 1,
            name: *bytes,
        }
    }
}

const __MODULE_NAME_RAW: &[u8; 9] = b"oss-rtld\0";
const __MODULE_NAME_LEN: usize = __MODULE_NAME_RAW.len();

#[used]
#[link_section = ".application_name"]
pub static __MODULE_NAME: ModuleName<__MODULE_NAME_LEN> = ModuleName::new(__MODULE_NAME_RAW);
