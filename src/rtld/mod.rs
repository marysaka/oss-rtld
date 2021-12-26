use core::arch::asm;
use core::fmt::{Debug, Write};
use core::{ffi::c_void, marker::PhantomData};
use static_assertions::assert_eq_size;

use modular_bitfield::prelude::*;

// TODO: Use an enum here.
const DT_NULL: isize = 0;
const DT_NEEDED: isize = 1;
const DT_PLTRELSZ: isize = 2;
const DT_PLTGOT: isize = 3;
const DT_HASH: isize = 4;
const DT_STRTAB: isize = 5;
const DT_SYMTAB: isize = 6;
const DT_RELA: isize = 7;
const DT_RELASZ: isize = 8;
const DT_RELAENT: isize = 9;
const DT_STRSZ: isize = 10;
const DT_SYMENT: isize = 11;
const DT_INIT: isize = 12;
const DT_FINI: isize = 13;
const DT_SONAME: isize = 14;
const DT_RPATH: isize = 15;
const DT_SYMBOLIC: isize = 16;
const DT_REL: isize = 17;
const DT_RELSZ: isize = 18;
const DT_RELENT: isize = 19;
const DT_PLTREL: isize = 20;
const DT_DEBUG: isize = 21;
const DT_TEXTREL: isize = 22;
const DT_JMPREL: isize = 23;
const DT_RELACOUNT: isize = 0x6ffffff9;
const DT_RELCOUNT: isize = 0x6ffffffa;

// TODO: ARMv8
const R_AARCH64_ABS64: usize = 0x101;
const R_AARCH64_ABS32: usize = 0x102;
const R_AARCH64_GLOB_DAT: usize = 0x401;
const R_AARCH64_JUMP_SLOT: usize = 0x402;
const R_AARCH64_RELATIVE: usize = 0x403;

fn is_arch_got_relocation_type(reloc_type: usize) -> bool {
    reloc_type == R_AARCH64_ABS64
        || reloc_type == R_AARCH64_ABS32
        || reloc_type == R_AARCH64_GLOB_DAT
}

trait Relocation: Sized + Debug {
    fn get_info(&self) -> usize;
    fn get_offset(&self) -> usize;
    fn compute_value(&self, value: *const u8) -> usize;
    fn apply(&self, module_base: *const u8, value: *const u8);

    fn get_type(&self) -> usize {
        // TODO: ARMv8
        self.get_info() & u32::MAX as usize
    }

    fn get_symbol_index(&self) -> usize {
        // TODO: ARMv8
        self.get_info() >> 32
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct RelocationTableEntry {
    pub offset: usize,
    pub info: usize,
}

impl Relocation for RelocationTableEntry {
    #[inline(always)]
    fn get_info(&self) -> usize {
        self.info
    }

    #[inline(always)]
    fn get_offset(&self) -> usize {
        self.offset
    }

    #[inline(always)]
    fn compute_value(&self, value: *const u8) -> usize {
        value as usize
    }

    #[inline(always)]
    fn apply(&self, module_base: *const u8, value: *const u8) {
        unsafe {
            let ptr = module_base.add(self.offset) as *mut usize;

            *ptr += self.compute_value(value);
        }
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct RelocationTableAddendEntry {
    pub offset: usize,
    pub info: usize,
    pub addend: isize,
}

impl Relocation for RelocationTableAddendEntry {
    #[inline(always)]
    fn get_info(&self) -> usize {
        self.info
    }

    #[inline(always)]
    fn get_offset(&self) -> usize {
        self.offset
    }

    #[inline(always)]
    fn compute_value(&self, value: *const u8) -> usize {
        unsafe { value.offset(self.addend) as usize }
    }

    #[inline(always)]
    fn apply(&self, module_base: *const u8, value: *const u8) {
        unsafe {
            let ptr = module_base.add(self.offset) as *mut usize;

            *ptr = self.compute_value(value);
        }
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct DynamicTableEntry {
    pub tag: isize,
    pub value: usize,
}

#[derive(Debug)]
struct DynamicTableItererator<'a> {
    current: *const DynamicTableEntry,
    _phantom: PhantomData<&'a DynamicTableEntry>,
}

impl<'a> DynamicTableItererator<'a> {
    #[inline(always)]
    pub fn new(table: *const DynamicTableEntry) -> Self {
        DynamicTableItererator {
            current: table,
            _phantom: PhantomData::default(),
        }
    }
}

impl<'a> Iterator for DynamicTableItererator<'a> {
    type Item = &'a DynamicTableEntry;

    #[inline(always)]
    fn next(&mut self) -> Option<Self::Item> {
        let entry = unsafe { &*self.current };

        if entry.tag == DT_NULL {
            None
        } else {
            let result = Some(entry);

            unsafe {
                self.current = self.current.add(1);
            }

            result
        }
    }
}

#[derive(BitfieldSpecifier, Copy, Clone, PartialEq, Eq, Debug)]
#[bits = 4]
pub enum SymbolType {
    None = 0,
    Object = 1,
    Function = 2,
    Section = 3,
    File = 4,
    Common = 5,
    ThreadLocalStorage = 6,
}

#[bitfield(bits = 8)]
#[derive(Debug, Default, Clone, Copy)]
#[repr(C)]
#[allow(dead_code)]
pub struct SymbolInfo {
    pub symbol_type: SymbolType,
    pub bind_global: bool,
    pub bind_weak: bool,
    pub os_specific: bool,
    pub processor_specific: bool,
}

#[derive(BitfieldSpecifier, Copy, Clone, PartialEq, Eq, Debug)]
#[bits = 2]
pub enum SymbolVisibility {
    Default = 0,
    Internal = 1,
    Hidden = 2,
    Protected = 3,
}

#[bitfield(bits = 8)]
#[derive(Debug, Default, Clone, Copy)]
#[repr(C)]
pub struct SymbolOther {
    pub visibility: SymbolVisibility,
    pub reserved: B6,
}

#[repr(C)]
#[derive(Copy, Clone, Debug)]
// TODO: Arch dependent structs
pub struct SymbolTableEntry {
    pub name_offset: u32,
    pub info: SymbolInfo,
    pub other: SymbolOther,
    pub section: u16,
    pub value: usize,
    pub size: usize,
}

#[repr(C)]
#[derive(Debug)]
pub struct Module {
    pub magic: u32,
    pub dynamic_offset: u32,
    pub bss_start_offset: u32,
    pub bss_end_offset: u32,
    pub exception_info_start_offset: u32,
    pub exception_info_end_offset: u32,
    pub module_runtime_offset: u32,
}

impl Module {
    #[inline(always)]
    pub fn is_valid(&self) -> bool {
        self.magic == 0x30444F4D
    }

    #[inline(always)]
    pub unsafe fn get_dynamic_table(&self) -> &mut DynamicTableEntry {
        let module_ptr = self as *const Module as *mut u8;

        &mut *(module_ptr.add(self.dynamic_offset as usize) as *mut DynamicTableEntry)
    }

    #[inline(always)]
    pub unsafe fn get_module_runtime(&self) -> &mut ModuleRuntime {
        let module_ptr = self as *const Module as *mut u8;

        &mut *(module_ptr.add(self.module_runtime_offset as usize) as *mut ModuleRuntime)
    }

    pub unsafe fn clear_bss(&self) {
        let module_ptr = self as *const Module as *mut u8;
        let start_bss_ptr = module_ptr.add(self.bss_start_offset as usize);

        core::ptr::write_bytes(
            start_bss_ptr,
            0,
            (self.bss_end_offset - self.bss_start_offset) as usize,
        );
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct ModuleRuntimeLink {
    pub prev: *mut ModuleRuntimeLink,
    pub next: *mut ModuleRuntimeLink,
}

impl Default for ModuleRuntimeLink {
    fn default() -> Self {
        ModuleRuntimeLink {
            prev: core::ptr::null_mut(),
            next: core::ptr::null_mut(),
        }
    }
}

impl ModuleRuntimeLink {
    #[inline(always)]
    pub unsafe fn initialize(&mut self) {
        self.next = self as *mut ModuleRuntimeLink;
        self.prev = self as *mut ModuleRuntimeLink;
    }

    #[inline(always)]
    pub unsafe fn link(&mut self, other: *mut ModuleRuntimeLink) {
        let other_prev = (*other).prev;

        (*other).prev = self.prev;
        (*other_prev).next = self as *mut ModuleRuntimeLink;
        (*self.next).next = other;

        self.prev = other_prev;
    }

    #[inline(always)]
    pub fn has_prev(&self) -> bool {
        self.prev == self as *const _ as *mut _
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct ModuleRuntime {
    pub link: ModuleRuntimeLink,
    // We avoid using a union here
    pub procedure_linkage_table: *const c_void,
    // We avoid using a union here
    pub relocations: *const c_void,
    pub module_base: *const u8,
    pub dynamic_table: *const DynamicTableEntry,
    pub is_rela: bool,
    pub procedure_linkage_table_size: usize,
    pub dt_init: Option<fn()>,
    pub dt_fini: Option<fn()>,
    pub hash_bucket: *const u32,
    pub hash_chain: *const u32,
    pub dynamic_str: *const u8,
    pub dynamic_symbols: *const SymbolTableEntry,
    pub dynamic_str_size: usize,
    pub global_offset_table: *mut *mut c_void,
    pub rela_dynamic_size: usize,
    pub rel_dynamic_size: usize,
    pub rel_count: usize,
    pub rela_count: usize,
    pub hash_nchain_value: usize,
    pub hash_nbucket_value: usize,
    pub got_stub_ptr: usize,

    // 6.x
    pub soname_idx: usize,
    pub nro_size: usize,
    pub cannot_revert_symbols: bool,
}

impl Default for ModuleRuntime {
    fn default() -> Self {
        ModuleRuntime {
            link: ModuleRuntimeLink::default(),
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
        }
    }
}

assert_eq_size!([u8; 0xD0], ModuleRuntime);

#[derive(Debug)]
struct HashBucketIter<'a> {
    chain: &'a [u32],
    dynamic_symbols: *const SymbolTableEntry,
    chain_value: u32,
}

impl<'a> HashBucketIter<'a> {
    #[inline(always)]
    fn compute_pjw_hash(data: &[u8]) -> u64 {
        let mut h = 0;

        for byte in data {
            h = (h << 4) + u64::from(*byte);

            let high = h & 0xF0000000;

            if high != 0 {
                h ^= high >> 24;
            }

            h &= !high;
        }
        return h;
    }

    pub fn new(module_runtime: &'a ModuleRuntime, name: &str) -> Self {
        let name_hash = Self::compute_pjw_hash(name.as_bytes()) as usize;

        let bucket = unsafe {
            core::slice::from_raw_parts(
                module_runtime.hash_bucket,
                module_runtime.hash_nbucket_value,
            )
        };
        let chain = unsafe {
            core::slice::from_raw_parts(module_runtime.hash_chain, module_runtime.hash_nchain_value)
        };
        let chain_value = bucket[name_hash % module_runtime.hash_nbucket_value];

        HashBucketIter {
            chain,
            dynamic_symbols: module_runtime.dynamic_symbols,
            chain_value,
        }
    }
}

impl<'a> Iterator for HashBucketIter<'a> {
    type Item = SymbolTableEntry;

    #[inline(always)]
    fn next(&mut self) -> Option<Self::Item> {
        if self.chain_value == 0 {
            return None;
        }

        let value = unsafe { *self.dynamic_symbols.add(self.chain_value as usize) };

        self.chain_value = self.chain[self.chain_value as usize];

        Some(value)
    }
}

impl ModuleRuntime {
    pub fn initialize(&mut self, module_base: *mut u8, module: &Module) {
        // Ensure default values
        *self = Self::default();

        // Init base of the link list
        unsafe {
            self.link.initialize();
        }

        unsafe {
            self.module_base = module_base;
            self.dynamic_table = module.get_dynamic_table();

            for entry in DynamicTableItererator::new(self.dynamic_table) {
                match entry.tag {
                    DT_PLTRELSZ => self.procedure_linkage_table_size = entry.value,
                    DT_PLTGOT => {
                        self.global_offset_table = self.module_base.add(entry.value) as *mut _
                    }
                    DT_HASH => {
                        let hash_table = self.module_base.add(entry.value) as *const u32;

                        let nbucket = *hash_table;
                        let nchain = *hash_table.add(1);

                        self.hash_nbucket_value = nbucket as usize;
                        self.hash_nchain_value = nchain as usize;

                        self.hash_bucket = hash_table.add(2);
                        self.hash_chain = hash_table.add(2).add(nbucket as usize);
                    }
                    DT_STRTAB => {
                        self.dynamic_str = self.module_base.add(entry.value as usize);
                    }
                    DT_SYMTAB => {
                        self.dynamic_symbols =
                            self.module_base.add(entry.value as usize) as *const SymbolTableEntry;
                    }
                    DT_RELA | DT_REL => {
                        self.relocations =
                            self.module_base.add(entry.value as usize) as *const c_void;
                    }
                    DT_RELASZ => self.rela_dynamic_size = entry.value,
                    DT_RELAENT => {
                        debug_assert!(
                            entry.value == core::mem::size_of::<RelocationTableAddendEntry>()
                        )
                    }
                    DT_STRSZ => self.dynamic_str_size = entry.value,
                    DT_SYMENT => {
                        debug_assert!(entry.value == core::mem::size_of::<SymbolTableEntry>())
                    }
                    DT_INIT => {
                        self.dt_init = Some(
                            *(&module_base.add(entry.value as usize) as *const _ as *const fn()),
                        )
                    }
                    DT_FINI => {
                        self.dt_fini = Some(
                            *(&module_base.add(entry.value as usize) as *const _ as *const fn()),
                        )
                    }
                    // 6.x+, ignored before
                    DT_SONAME => self.soname_idx = entry.value as usize,
                    DT_RELSZ => self.rel_dynamic_size = entry.value as usize,
                    DT_RELENT => {
                        debug_assert!(entry.value == core::mem::size_of::<RelocationTableEntry>())
                    }
                    DT_PLTREL => {
                        let value = entry.value as isize;
                        self.is_rela = value == DT_RELA;

                        debug_assert!(value == DT_REL || value == DT_RELA);
                    }
                    DT_JMPREL => {
                        self.procedure_linkage_table =
                            self.module_base.add(entry.value as usize) as *const c_void
                    }
                    DT_RELACOUNT => self.rela_count = entry.value as usize,
                    DT_RELCOUNT => self.rel_count = entry.value as usize,
                    DT_NEEDED | DT_RPATH | DT_SYMBOLIC | DT_DEBUG | DT_TEXTREL => {}
                    _unknown_dt => {
                        //write!(&mut crate::nx::KernelWritter, "Unknown DT: {}", unknown_dt).ok();
                    }
                }
            }
        }
    }

    #[inline(always)]
    fn relocate_entries<T>(module_base: *const u8, relocations: *const T, relocations_count: usize)
    where
        T: Relocation,
    {
        let relocations = unsafe { core::slice::from_raw_parts(relocations, relocations_count) };

        for relocation in relocations {
            let relocation_type = relocation.get_type();

            if relocation_type == R_AARCH64_RELATIVE {
                relocation.apply(module_base, module_base);
            }
        }
    }

    pub fn relocate(&mut self) {
        if self.rela_count != 0 {
            Self::relocate_entries(
                self.module_base,
                self.relocations as *const RelocationTableAddendEntry,
                self.rela_count,
            );
        }

        if self.rel_count != 0 {
            Self::relocate_entries(
                self.module_base,
                self.relocations as *const RelocationTableEntry,
                self.rel_count,
            );
        }
    }

    #[inline(always)]
    unsafe fn get_string_table(&self) -> &[u8] {
        core::slice::from_raw_parts(self.dynamic_str, self.dynamic_str_size)
    }

    #[inline(always)]
    fn try_get_symbol_str(&self, symbol: &SymbolTableEntry) -> Option<&str> {
        let string_table = unsafe { self.get_string_table() };

        let name_offset = symbol.name_offset as usize;
        let mut str_size = 0;

        loop {
            // If we are getting out of range of the table, return!
            if name_offset + str_size > string_table.len() {
                return None;
            }

            if string_table[name_offset + str_size] == 0 {
                break;
            }

            str_size += 1;
        }

        if let Ok(result) = core::str::from_utf8(&string_table[name_offset..name_offset + str_size])
        {
            Some(result)
        } else {
            None
        }
    }

    pub fn get_external_symbol_by_name(&self, name: &str) -> Option<SymbolTableEntry> {
        if let Some(symbol) = self.get_symbol_by_name(name) {
            if symbol.info.bind_global() || symbol.info.bind_weak() {
                return Some(symbol);
            }
        }

        return None;
    }

    pub fn get_symbol_by_name(&self, name: &str) -> Option<SymbolTableEntry> {
        // TODO: Enum for this
        const SHN_UNDEF: u16 = 0;
        const SHN_COMMON: u16 = 0xfff2;

        for symbol in HashBucketIter::new(self, name) {
            if symbol.section != SHN_UNDEF && symbol.section != SHN_COMMON {
                if let Some(other_name) = self.try_get_symbol_str(&symbol) {
                    if other_name == name {
                        return Some(symbol);
                    }
                } else {
                    write!(
                        &mut crate::nx::KernelWritter,
                        "Warning invalid symbol was found: {:?}",
                        symbol
                    )
                    .ok();
                }
            }
        }

        None
    }

    pub fn resolve_symbol(&self, symbol: &SymbolTableEntry) -> Option<usize> {
        let string_table = unsafe { self.get_string_table() };

        if symbol.other.visibility() == SymbolVisibility::Default {
            for lookup_function in unsafe { LOOKUP_FUNCTIONS } {
                if let Some(lookup_function) = lookup_function {
                    let target_address = lookup_function(
                        self as *const _ as *mut Self,
                        string_table[symbol.name_offset as usize..].as_ptr(),
                    );

                    if target_address != 0 {
                        return Some(target_address);
                    }
                }
            }
        } else {
            // In other cases, assume protected/internal/... to the module.
            // FIXME: We could use the string table to ensure we don't get out of bound when parsing the cstr.
            if let Some(name) =
                unsafe { parse_cstr(string_table[symbol.name_offset as usize..].as_ptr()) }
            {
                if let Some(symbol) = self.get_symbol_by_name(name) {
                    return Some(unsafe { self.get_symbol_value(&symbol) as usize });
                }
            }
        }

        if symbol.info.bind_weak() {
            Some(0)
        } else {
            None
        }
    }

    #[inline(always)]
    fn resolve_got_entries<T>(&self, dynamic_relocation_offset: usize, relocations_size: usize)
    where
        T: Relocation,
    {
        let relocations = unsafe {
            core::slice::from_raw_parts(
                self.relocations as *const T,
                relocations_size / core::mem::size_of::<T>(),
            )
        };

        for relocation in &relocations[dynamic_relocation_offset..] {
            let relocation_type = relocation.get_type();

            if is_arch_got_relocation_type(relocation_type) {
                let symbol_index = relocation.get_symbol_index();
                let symbol_to_resolve = unsafe { *self.dynamic_symbols.add(symbol_index) };

                if let Some(target_address) = self.resolve_symbol(&symbol_to_resolve) {
                    relocation.apply(self.module_base, target_address as *const u8);
                } else if RO_DEBUG_FLAG {
                    write!(
                        &mut crate::nx::KernelWritter,
                        "[rtld] warning: unresolved symbol = '{:?}'",
                        self.try_get_symbol_str(&symbol_to_resolve)
                    )
                    .ok();
                }
            }
        }
    }

    #[inline(always)]
    fn resolve_plt_entries<T>(&mut self, lazy_init: bool)
    where
        T: Relocation,
    {
        if self.procedure_linkage_table_size < core::mem::size_of::<T>() {
            return;
        }

        let procedure_linkage_table = unsafe {
            core::slice::from_raw_parts(
                self.procedure_linkage_table as *const T,
                self.procedure_linkage_table_size / core::mem::size_of::<T>(),
            )
        };

        for relocation in procedure_linkage_table {
            let relocation_type = relocation.get_type();

            if relocation_type == R_AARCH64_JUMP_SLOT {
                let target = unsafe { self.module_base.add(relocation.get_offset()) as *mut usize };
                let target_ptr = unsafe { self.module_base.add(*target) as usize };

                if self.got_stub_ptr == 0 {
                    self.got_stub_ptr = target_ptr;
                } else {
                    debug_assert!(self.got_stub_ptr == target_ptr);
                }

                if lazy_init {
                    unsafe {
                        *target = target_ptr;
                    }
                } else {
                    let symbol_index = relocation.get_symbol_index();
                    let symbol_to_resolve = unsafe { *self.dynamic_symbols.add(symbol_index) };

                    if let Some(target_address) = self.resolve_symbol(&symbol_to_resolve) {
                        relocation.apply(self.module_base, target_address as *const u8);
                    } else {
                        if RO_DEBUG_FLAG {
                            write!(
                                &mut crate::nx::KernelWritter,
                                "[rtld] warning: unresolved symbol = '{:?}'",
                                self.try_get_symbol_str(&symbol_to_resolve)
                            )
                            .ok();
                        }

                        unsafe {
                            *target = target_ptr;
                        }
                    }
                }
            }
        }
    }

    pub fn resolve_symbols(&mut self, lazy_init: bool) {
        self.resolve_got_entries::<RelocationTableEntry>(self.rel_count, self.rel_dynamic_size);
        self.resolve_got_entries::<RelocationTableAddendEntry>(
            self.rela_count,
            self.rela_dynamic_size,
        );

        if self.is_rela {
            self.resolve_plt_entries::<RelocationTableAddendEntry>(lazy_init);
        } else {
            self.resolve_plt_entries::<RelocationTableEntry>(lazy_init);
        }

        if !self.global_offset_table.is_null() {
            // Setup GOT stub parameters
            unsafe {
                *self.global_offset_table.add(1) = self as *const Self as *mut _;
                // TODO: lazy loading setup
                *self.global_offset_table.add(2) = runtime_resolve as *mut _;
            }
        }
    }

    pub unsafe fn get_symbol_value(&self, symbol: &SymbolTableEntry) -> *mut *const c_void {
        self.module_base.add(symbol.value) as *mut _
    }

    #[inline(always)]
    pub unsafe fn set_symbol_value(&self, symbol: &SymbolTableEntry, value: *const c_void) {
        *self.get_symbol_value(symbol) = value;
    }
}

#[repr(C)]
#[derive(Default, Debug)]
pub struct ModuleObjectList {
    pub link: ModuleRuntimeLink,
}

impl ModuleObjectList {
    #[inline(always)]
    pub unsafe fn initialize(&mut self) {
        self.link.initialize();
    }

    #[inline(always)]
    pub unsafe fn link(&mut self, other: *mut ModuleRuntime) {
        self.link.link(&mut (*other).link);
    }

    #[inline(always)]
    pub fn is_empty(&self) -> bool {
        self.link.has_prev()
    }
}

#[derive(Debug)]
pub struct ModuleRuntimeIter<'a> {
    root: *mut ModuleRuntimeLink,
    current: *const ModuleRuntimeLink,
    _phantom: PhantomData<&'a ModuleRuntimeLink>,
}

impl<'a> ModuleRuntimeIter<'a> {
    pub fn new(root: *mut ModuleRuntimeLink) -> Self {
        ModuleRuntimeIter {
            root: root,
            current: unsafe { (*root).prev },
            _phantom: PhantomData::default(),
        }
    }
}

impl<'a> Iterator for ModuleRuntimeIter<'a> {
    type Item = &'a mut ModuleRuntime;

    #[inline(always)]
    fn next(&mut self) -> Option<Self::Item> {
        let has_next = self.current == self.root;

        if has_next {
            None
        } else {
            let result = unsafe { Some(&mut *(self.current as *mut ModuleRuntime)) };

            unsafe {
                self.current = (*self.current).prev;
            }

            result
        }
    }
}

// TODO: Move this here and remove global_asm usage
extern "C" {
    static __nx_mod0: Module;
}

#[no_mangle]
#[used]
pub static mut SELF_MODULE_RUNTIME: ModuleRuntime = ModuleRuntime {
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

pub static mut MANUAL_LOAD_LIST: ModuleObjectList = ModuleObjectList {
    link: ModuleRuntimeLink {
        next: core::ptr::null_mut(),
        prev: core::ptr::null_mut(),
    },
};

pub static mut GLOBAL_LOAD_LIST: ModuleObjectList = ModuleObjectList {
    link: ModuleRuntimeLink {
        next: core::ptr::null_mut(),
        prev: core::ptr::null_mut(),
    },
};

pub type LookupFunction = extern "C" fn(module: *mut ModuleRuntime, raw_name: *const u8) -> usize;

pub static mut LOOKUP_FUNCTIONS: [Option<LookupFunction>; 2] = [
    Some(default_lookup_auto_list),
    // Manual lookup modified by the SDK
    None,
];

pub static RO_DEBUG_FLAG: bool = true;

pub unsafe fn parse_cstr<'a>(raw_name: *const u8) -> Option<&'a str> {
    let mut str_size = 0;

    loop {
        if *raw_name.add(str_size) == 0 {
            break;
        }

        str_size += 1;
    }

    if let Ok(result) = core::str::from_utf8(core::slice::from_raw_parts(raw_name, str_size)) {
        Some(result)
    } else {
        None
    }
}

pub extern "C" fn default_lookup_auto_list(
    _module: *mut ModuleRuntime,
    raw_name: *const u8,
) -> usize {
    unsafe {
        if GLOBAL_LOAD_LIST.is_empty() {
            return 0;
        }
    }

    if let Some(name) = unsafe { parse_cstr(raw_name) } {
        for module_runtime in ModuleRuntimeIter::new(unsafe { &mut GLOBAL_LOAD_LIST.link }) {
            if let Some(symbol) = module_runtime.get_external_symbol_by_name(name) {
                return unsafe { module_runtime.get_symbol_value(&symbol) as usize };
            }
        }
    }

    0
}

#[inline(always)]
pub unsafe fn initialize(module_base: *mut u8) {
    MANUAL_LOAD_LIST.initialize();
    GLOBAL_LOAD_LIST.initialize();
    SELF_MODULE_RUNTIME.initialize(module_base, &__nx_mod0);

    GLOBAL_LOAD_LIST.link(&mut SELF_MODULE_RUNTIME);
}

pub fn relocate_self(base_address: *mut u8, dynamic: *mut DynamicTableEntry) {
    let mut rela_offset = None;
    let mut rela_entry_size = 0;
    let mut rela_count = 0;

    let mut rel_offset = None;
    let mut rel_entry_size = 0;
    let mut rel_count = 0;

    for entry in DynamicTableItererator::new(dynamic) {
        match entry.tag {
            DT_RELA => rela_offset = Some(entry.value),
            DT_RELAENT => rela_entry_size = entry.value,
            DT_REL => rel_offset = Some(entry.value),
            DT_RELENT => rel_entry_size = entry.value,
            DT_RELACOUNT => rela_count = entry.value,
            DT_RELCOUNT => rel_count = entry.value,
            _ => {}
        }
    }

    if let Some(rela_offset) = rela_offset {
        if rela_entry_size != core::mem::size_of::<RelocationTableAddendEntry>() {
            loop {}
        }

        let relocations =
            unsafe { base_address.add(rela_offset) as *const RelocationTableAddendEntry };

        ModuleRuntime::relocate_entries(base_address, relocations, rela_count);
    }

    if let Some(rel_offset) = rel_offset {
        if rel_entry_size != core::mem::size_of::<RelocationTableEntry>() {
            loop {}
        }

        let relocations = unsafe { base_address.add(rel_offset) as *const RelocationTableEntry };

        ModuleRuntime::relocate_entries(base_address, relocations, rel_count);
    }
}

pub unsafe fn call_initializers() {
    for module_runtime in ModuleRuntimeIter::new(&mut GLOBAL_LOAD_LIST.link) {
        if let Some(init_function) = module_runtime.dt_init {
            init_function();
        }
    }
}

pub unsafe fn call_finilizers() {
    for module_runtime in ModuleRuntimeIter::new(&mut GLOBAL_LOAD_LIST.link) {
        if let Some(fini_function) = module_runtime.dt_fini {
            fini_function();
        }
    }
}

// TODO: Move to ModuleRuntime after refacto
fn lazy_bind_resolve_internal<T>(module: &mut ModuleRuntime, index: usize) -> usize
where
    T: Relocation,
{
    let procedure_linkage_table = unsafe {
        core::slice::from_raw_parts(
            module.procedure_linkage_table as *const T,
            module.procedure_linkage_table_size / core::mem::size_of::<T>(),
        )
    };

    let relocation = &procedure_linkage_table[index];
    let symbol_index = relocation.get_symbol_index();
    let symbol_to_resolve = unsafe { *module.dynamic_symbols.add(symbol_index) };

    if let Some(target_address) = module.resolve_symbol(&symbol_to_resolve) {
        if target_address == 0 {
            return 0;
        }

        return relocation.compute_value(target_address as *const u8);
    } else if RO_DEBUG_FLAG {
        write!(
            &mut crate::nx::KernelWritter,
            "[rtld] warning: unresolved symbol = '{:?}'",
            module.try_get_symbol_str(&symbol_to_resolve)
        )
        .ok();
    }

    0
}

unsafe fn lazy_bind_symbol(module: *mut ModuleRuntime, index: usize) -> usize {
    let module = &mut *module;

    if module.is_rela {
        lazy_bind_resolve_internal::<RelocationTableAddendEntry>(module, index)
    } else {
        lazy_bind_resolve_internal::<RelocationTableEntry>(module, index)
    }
}

#[naked]
pub unsafe extern "C" fn runtime_resolve() -> ! {
    asm!(
        "
        /* AArch64 we get called with:
        ip0 (x16)         &PLTGOT[2]
        ip1 (x17)         temp(dl resolver entry point)
        [sp, #0]          &PLTGOT[n]
    
        What we need:
        x0 = calling module (ip0[-1])
        x1 = .rel.plt index ((ip1 - ip0 - 8) / 8)
        */
        ldr x17, [sp]
        str x29, [sp]
        stp x8, x19, [sp, #-0x10]!
        stp x6, x7, [sp, #-0x10]!
        stp x4, x5, [sp, #-0x10]!
        stp x2, x3, [sp, #-0x10]!
        stp x0, x1, [sp, #-0x10]!
        stp q6, q7, [sp, #-0x20]!
        stp q4, q5, [sp, #-0x20]!
        stp q2, q3, [sp, #-0x20]!
        stp q0, q1, [sp, #-0x20]!
        mov x29, sp
        mov x19, x17
        sub x1, x17, x16
        sub x1, x1, #8
        lsr x1, x1, #3
        ldur x0, [x16, #-8]
        bl {}
        str x0, [x19]
        mov x16, x0
        ldp q0, q1, [sp], #0x20
        ldp q2, q3, [sp], #0x20
        ldp q4, q5, [sp], #0x20
        ldp q6, q7, [sp], #0x20
        ldp x0, x1, [sp], #0x10
        ldp x2, x3, [sp], #0x10
        ldp x4, x5, [sp], #0x10
        ldp x6, x7, [sp], #0x10
        ldp x8, x19, [sp], #0x10
        ldp x29, x30, [sp], #0x10
        br x16
        b .
        ",
        sym lazy_bind_symbol,
        options(noreturn),
    )
}

pub fn get_exported_function<T>(name: &str) -> Option<T>
where
    T: Sized + Copy,
{
    for module_runtime in ModuleRuntimeIter::new(unsafe { &mut GLOBAL_LOAD_LIST.link }) {
        if let Some(symbol) = module_runtime.get_symbol_by_name(name) {
            if let Some(value) = module_runtime.resolve_symbol(&symbol) {
                if value != 0 {
                    return Some(unsafe { *(&value as *const usize as *const T) });
                }
            }
        }
    }

    None
}
