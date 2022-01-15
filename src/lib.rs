#![feature(asm_sym, naked_functions, linkage)]
#![no_std]
#![no_main]
#![allow(dead_code, clippy::identity_op)]


use core::ffi::c_void;

use crate::{
    nx::common::{MemoryInfo, MemoryState},
    rtld::{
        LookupFunction, Module, ModuleObjectList, ModuleRuntime, ModuleRuntimeIter,
        ModuleRuntimeLink, GLOBAL_LOAD_LIST, LOOKUP_FUNCTIONS, MANUAL_LOAD_LIST, RO_DEBUG_FLAG,
    },
};

mod nx;
mod rtld;

// NOTE: We differ from Nintendo here to provide the exception type to the user directly.
type CustomExceptionHandlerFunc = extern "C" fn(u32);
type SdkExceptionHandlerFunc = extern "C" fn();
type CustomStartFunc = extern "C" fn(
    usize,
    *const c_void,
    unsafe extern "C" fn(),
    unsafe extern "C" fn(),
    unsafe extern "C" fn(),
);
type Sdk10StartFunc = extern "C" fn(
    usize,
    *const c_void,
    unsafe extern "C" fn(),
    unsafe extern "C" fn(),
    unsafe extern "C" fn(),
);
type Sdk03StartFunc =
    extern "C" fn(usize, *const c_void, unsafe extern "C" fn(), unsafe extern "C" fn());

extern "C" {
    static __argdata__: c_void;

    // Custom user exception handler
    #[linkage = "extern_weak"]
    static __rtld_custom_user_exception_handler: *const c_void;

    // SDK user exception handler
    #[linkage = "extern_weak"]
    static _ZN2nn2os6detail20UserExceptionHandlerEv: *const c_void;

    // Custom start entrypoint
    #[linkage = "extern_weak"]
    static __rtld_custom_start: *const c_void;

    // [11.0.0+] SDK start entrypoint
    #[linkage = "extern_weak"]
    static _ZN2nn4init5StartEmmPFvvES2_S2_: *const c_void;

    // [3.0.0-11.0.0] SDK start entrypoint
    #[linkage = "extern_weak"]
    static _ZN2nn4init5StartEmmPFvvES2_: *const c_void;

    // [1.0.0-3.0.0] SDK start entrypoint internals (inlined in RTLD)
    fn __nnDetailInitLibc0();
    fn __nnDetailInitLibc1();
    fn __nnDetailInitLibc2();
    fn nninitStartup();
    fn nndiagStartup();
    fn nnosInitialize(thread_handle: usize, argument_ptr: *const c_void);
    fn nnMain();
    fn nnosQuickExit();

    #[linkage = "extern_weak"]
    static nninitInitializeSdkModule: *const c_void;

    #[linkage = "extern_weak"]
    static nninitInitializeAbortObserver: *const c_void;

    #[linkage = "extern_weak"]
    static nninitFinalizeSdkModule: *const c_void;
}

#[no_mangle]
#[used]
static mut SELF_MODULE_RUNTIME: ModuleRuntime = ModuleRuntime {
    link: ModuleRuntimeLink {
        next: core::ptr::null_mut(),
        prev: core::ptr::null_mut(),
    },
    procedure_linkage_table: core::ptr::null(),
    relocations: core::ptr::null(),
    module_base: core::ptr::null(),
    dynamic_table: core::ptr::null(),
    is_rela: false,
    procedure_linkage_table_size: 0,
    dt_init: None,
    dt_fini: None,
    hash_bucket: core::ptr::null(),
    hash_chain: core::ptr::null(),
    dynamic_str: core::ptr::null(),
    dynamic_symbols: core::ptr::null(),
    dynamic_str_size: 0,
    global_offset_table: core::ptr::null_mut(),
    rela_dynamic_size: 0,
    rel_dynamic_size: 0,
    rel_count: 0,
    rela_count: 0,
    hash_nchain_value: 0,
    hash_nbucket_value: 0,
    got_stub_ptr: 0,

    // 6.x
    soname_idx: 0,
    nro_size: 0,
    cannot_revert_symbols: false,
};

#[no_mangle]
pub static mut EXCEPTION_HANDLER_READY: bool = false;

#[inline(always)]
unsafe fn get_function_from_ptr<T>(ptr: *const c_void) -> Option<T>
where
    T: Sized + Copy,
{
    if ptr.is_null() {
        None
    } else {
        Some(*(&ptr as *const *const c_void as *const T))
    }
}

#[no_mangle]
unsafe fn __rtld_relocate_self(module_base: *mut u8) {
    let module = &mut *Module::get_module_by_module_base(module_base);
    let module_runtime = module.get_module_runtime();

    module_runtime.initialize(module_base, module);
    module_runtime.relocate(false, true);
}

#[no_mangle]
unsafe extern "C" fn __rtld_handle_exception(exception_type: u32) {
    if let Some(custom_user_exception_handler) =
        get_function_from_ptr::<CustomExceptionHandlerFunc>(__rtld_custom_user_exception_handler)
    {
        custom_user_exception_handler(exception_type);
    }
    if let Some(sdk_user_exception_handler) =
        get_function_from_ptr::<SdkExceptionHandlerFunc>(_ZN2nn2os6detail20UserExceptionHandlerEv)
    {
        sdk_user_exception_handler();
    }
}

// TODO: improve safety of this function and make clippy happy
#[allow(non_snake_case, clippy::not_unsafe_ptr_arg_deref)]
#[no_mangle]
pub fn main(module_base: *mut u8, thread_handle: u32) {
    unsafe {
        let module = &mut *Module::get_module_by_module_base(module_base);
        let module_runtime = (module).get_module_runtime();

        rtld::initialize(module_runtime);

        let linker_module_base = module_base as u64;

        let mut next_query_address: usize = 0;

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
                let module: &mut Module = &mut *Module::get_module_by_module_base(module_base);

                debug_assert!(module.is_valid());

                module.clear_bss();

                let module_runtime = module.get_module_runtime();

                module_runtime.initialize(module_base, module);

                rtld::GLOBAL_LOAD_LIST.link(module_runtime);
            }

            let (next_query_address_tmp, overflowed) =
                (memory_info.address as usize).overflowing_add(memory_info.size as usize);

            next_query_address = next_query_address_tmp;

            if overflowed {
                break;
            }
        }

        if !GLOBAL_LOAD_LIST.is_empty() {
            // First bind all hardcoded symbols
            for module_runtime in ModuleRuntimeIter::new(&mut GLOBAL_LOAD_LIST.link, false) {
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

            for module_runtime in ModuleRuntimeIter::new(&mut GLOBAL_LOAD_LIST.link, false) {
                module_runtime.relocate(module_runtime.module_base == module_base, false);
                module_runtime.resolve_symbols(true);
            }
        }

        let rtld_custom_start_option =
            get_function_from_ptr::<CustomStartFunc>(__rtld_custom_start);
        let sdk10_start_option =
            get_function_from_ptr::<Sdk10StartFunc>(_ZN2nn4init5StartEmmPFvvES2_S2_);
        let sdk03_start_option =
            get_function_from_ptr::<Sdk03StartFunc>(_ZN2nn4init5StartEmmPFvvES2_);

        if let Some(rtld_custom_start) = rtld_custom_start_option {
            // Custom initialization
            rtld_custom_start(
                thread_handle as usize,
                &__argdata__,
                notify_exception_handler_ready,
                rtld::call_initializers,
                rtld::call_finilizers,
            );
        } else if let Some(sdk10_start) = sdk10_start_option {
            // ~10.x SDK initialization
            sdk10_start(
                thread_handle as usize,
                &__argdata__,
                notify_exception_handler_ready,
                rtld::call_initializers,
                rtld::call_finilizers,
            );
        } else if let Some(sdk03_start) = sdk03_start_option {
            // ~3.x SDK initialization
            sdk03_start(
                thread_handle as usize,
                &__argdata__,
                notify_exception_handler_ready,
                rtld::call_initializers,
            );
        } else {
            // In other cases, we assume SDK from before 3.x (brace yourself, this is hell)
            let initialize_sdk_module_option =
                get_function_from_ptr::<extern "C" fn()>(nninitInitializeSdkModule);
            let initialize_abort_observer_option =
                get_function_from_ptr::<extern "C" fn()>(nninitInitializeAbortObserver);
            let finalize_sdk_module_option =
                get_function_from_ptr::<extern "C" fn()>(nninitFinalizeSdkModule);

            __nnDetailInitLibc0();
            nnosInitialize(thread_handle as usize, &__argdata__);
            notify_exception_handler_ready();
            __nnDetailInitLibc1();
            nndiagStartup();

            if let Some(initialize_sdk_module) = initialize_sdk_module_option {
                initialize_sdk_module();
            }

            if let Some(initialize_abort_observer) = initialize_abort_observer_option {
                initialize_abort_observer();
            }

            nninitStartup();
            __nnDetailInitLibc2();

            rtld::call_initializers();
            nnMain();

            if let Some(finalize_sdk_module) = finalize_sdk_module_option {
                finalize_sdk_module();
            }

            nnosQuickExit();

            nx::syscall::exit_process();
        }
    }
}

unsafe extern "C" fn notify_exception_handler_ready() {
    EXCEPTION_HANDLER_READY = true;
}
