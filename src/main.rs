#![feature(asm, asm_sym, naked_functions, global_asm)]
#![no_std]
#![no_main]
#![allow(dead_code)]

use core::ffi::c_void;
use core::fmt::Write;

use crate::{
    nx::syscall::{MemoryInfo, MemoryState},
    rtld::{
        LookupFunction, Module, ModuleObjectList, ModuleRuntimeIter, GLOBAL_LOAD_LIST,
        LOOKUP_FUNCTIONS, MANUAL_LOAD_LIST, RO_DEBUG_FLAG,
    },
};

mod nx;
mod rtld;

extern "C" {
    static __argdata__: c_void;
}

pub static mut EXCEPTION_HANDLER_READY: bool = false;

#[allow(non_snake_case)]
pub fn main(module_base: *mut u8, thread_handle: u32) {
    unsafe {
        rtld::initialize(module_base);

        let linker_module_base = module_base as u64;

        let mut next_query_address: u64 = 0;

        loop {
            let mut memory_info: MemoryInfo = MemoryInfo::default();
            let mut page_info = 0;

            let result = nx::syscall::query_memory(
                &mut memory_info,
                &mut page_info,
                next_query_address as usize,
            );

            debug_assert!(result == 0, "query_memory returned: {}", result);

            if memory_info.permission.read()
                && memory_info.permission.execute()
                && memory_info.state == MemoryState::Code
                && memory_info.address != linker_module_base
            {
                let module_base = memory_info.address as *mut u8;
                let module_offset = (module_base as *const u32).add(1).read() as usize;

                let module: &Module = &*(module_base).add(module_offset).cast();

                debug_assert!(module.is_valid());

                module.clear_bss();

                let module_runtime = module.get_module_runtime();

                module_runtime.initialize(module_base, module);
                module_runtime.relocate();

                rtld::GLOBAL_LOAD_LIST.link(module_runtime);
            }

            let (next_query_address_tmp, overflowed) =
                memory_info.address.overflowing_add(memory_info.size);

            next_query_address = next_query_address_tmp;

            if overflowed {
                break;
            }
        }

        if !GLOBAL_LOAD_LIST.is_empty() {
            // First bind all hardcoded symbols
            for module_runtime in ModuleRuntimeIter::new(&mut GLOBAL_LOAD_LIST.link) {
                if let Some(symbol) =
                    module_runtime.get_external_symbol_by_name("_ZN2nn2ro6detail15g_pAutoLoadListE")
                {
                    module_runtime.set_symbol_value(
                        &symbol,
                        &GLOBAL_LOAD_LIST as *const ModuleObjectList as *const _,
                    );
                }

                if let Some(symbol) = module_runtime
                    .get_external_symbol_by_name("_ZN2nn2ro6detail17g_pManualLoadListE")
                {
                    module_runtime.set_symbol_value(
                        &symbol,
                        &MANUAL_LOAD_LIST as *const ModuleObjectList as *const _,
                    );
                }

                if let Some(symbol) =
                    module_runtime.get_external_symbol_by_name("_ZN2nn2ro6detail14g_pRoDebugFlagE")
                {
                    module_runtime
                        .set_symbol_value(&symbol, &RO_DEBUG_FLAG as *const bool as *const _);
                }

                if let Some(symbol) = module_runtime.get_external_symbol_by_name(
                    "_ZN2nn2ro6detail34g_pLookupGlobalAutoFunctionPointerE",
                ) {
                    module_runtime
                        .set_symbol_value(&symbol, LOOKUP_FUNCTIONS[0].unwrap() as *const _);
                }

                if let Some(symbol) = module_runtime.get_external_symbol_by_name(
                    "_ZN2nn2ro6detail36g_pLookupGlobalManualFunctionPointerE",
                ) {
                    module_runtime.set_symbol_value(
                        &symbol,
                        &LOOKUP_FUNCTIONS[1] as *const Option<LookupFunction> as *const _,
                    );
                }
            }

            for module_runtime in ModuleRuntimeIter::new(&mut GLOBAL_LOAD_LIST.link) {
                module_runtime.resolve_symbols(true);
            }
        }

        write!(&mut nx::KernelWritter, "{:p}", &__argdata__).ok();

        type CustomStartFunc = extern fn(usize, *const c_void, unsafe fn(), unsafe fn(), unsafe fn());
        type Sdk10StartFunc = extern fn(usize, *const c_void, unsafe fn(), unsafe fn(), unsafe fn());
        type Sdk03StartFunc = extern fn(usize, *const c_void, unsafe fn(), unsafe fn());

        if let Some(init_function) = rtld::get_exported_function::<CustomStartFunc>("__rtld_custom_start") {
            // Custom initialization
            init_function(thread_handle as usize, &__argdata__, notify_exception_handler_ready, rtld::call_initializers, rtld::call_finilizers);
        }
        else if let Some(init_function) = rtld::get_exported_function::<Sdk10StartFunc>("_ZN2nn4init5StartEmmPFvvES2_S2_") {
            // ~10.x SDK initialization
            init_function(thread_handle as usize, &__argdata__, notify_exception_handler_ready, rtld::call_initializers, rtld::call_finilizers);
        } else if let Some(init_function) = rtld::get_exported_function::<Sdk03StartFunc>("_ZN2nn4init5StartEmmPFvvES2_") {
            // ~3.x SDK initialization
            init_function(thread_handle as usize, &__argdata__, notify_exception_handler_ready, rtld::call_initializers);
        } else {
            // In other cases, we assume SDK from before 3.x (brace yourself, this is hell)

            let __nnDetailInitLibc0 = &rtld::get_exported_function::<extern fn()>("__nnDetailInitLibc0").unwrap();
            let nnosInitialize = rtld::get_exported_function::<extern fn(usize, *const c_void)>("nnosInitialize").unwrap();
            let __nnDetailInitLibc1 = rtld::get_exported_function::<extern fn()>("__nnDetailInitLibc1").unwrap();
            let nndiagStartup = rtld::get_exported_function::<extern fn()>("nndiagStartup").unwrap();

            let nninitStartup = rtld::get_exported_function::<extern fn()>("nninitStartup").unwrap();
            let __nnDetailInitLibc2 = rtld::get_exported_function::<extern fn()>("__nnDetailInitLibc1").unwrap();
            let nnMain = rtld::get_exported_function::<extern fn()>("nnMain").unwrap();
            let nnosQuickExit = rtld::get_exported_function::<extern fn()>("nnosQuickExit").unwrap();

            // Weak symbols
            let nninitInitializeSdkModuleOption = rtld::get_exported_function::<extern fn()>("nninitInitializeSdkModule");
            let nninitInitializeAbortObserverOption = rtld::get_exported_function::<extern fn()>("nninitInitializeAbortObserver");
            let nninitFinalizeSdkModuleOption = rtld::get_exported_function::<extern fn()>("nninitFinalizeSdkModule");

            __nnDetailInitLibc0();
            nnosInitialize(thread_handle as usize, &__argdata__);
            notify_exception_handler_ready();
            __nnDetailInitLibc1();
            nndiagStartup();

            if let Some(nninitInitializeSdkModule) = nninitInitializeSdkModuleOption {
                nninitInitializeSdkModule();
            }

            if let Some(nninitInitializeAbortObserver) = nninitInitializeAbortObserverOption {
                nninitInitializeAbortObserver();
            }

            nninitStartup();
            __nnDetailInitLibc2();

            rtld::call_initializers();
            nnMain();

            if let Some(nninitFinalizeSdkModule) = nninitFinalizeSdkModuleOption {
                nninitFinalizeSdkModule();
            }

            nnosQuickExit();

            nx::syscall::exit_process();
        }
    }
}

unsafe fn notify_exception_handler_ready() {
    write!(&mut nx::KernelWritter, "notify_exception_handler_ready").ok();
    EXCEPTION_HANDLER_READY = true;
}
