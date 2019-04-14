OUTPUT_FORMAT(elf64-littleaarch64)
OUTPUT_ARCH(aarch64)
ENTRY(__module_start)

PHDRS
{
  text PT_LOAD FLAGS(5);
  rodata PT_LOAD FLAGS(4);
  data PT_LOAD FLAGS(6);
  bss PT_LOAD FLAGS(6);
  dynamic PT_DYNAMIC;
}

SECTIONS
{
  . = 0;

  .text : {
    HIDDEN(__text_start = .);
    KEEP(*(.text.crt0))
    *(.text .text.*)
    *(.plt .plt.*)
    HIDDEN(__text_end = .);
  } :text

  /* Read-only sections */

  . = ALIGN(0x1000);

  .rodata : {
    KEEP (*(.rodata.application_name))
    *(.rodata .rodata.*)
  } :rodata
  .mod0 : {
    KEEP(*(.data.mod0))
  } :rodata
  .hash : { *(.hash) } :rodata
  .dynsym : { *(.dynsym .dynsym.*) } :rodata
  .dynstr : { *(.dynstr .dynstr.*) } :rodata
  .rela.dyn : { *(.rela.dyn) } :rodata
  .rela.plt : { *(.rela.plt) } :rodata

  .hash : { *(.hash) } :rodata
  .gnu.hash : { *(.gnu.hash) } :rodata
  .gnu.version : { *(.gnu.version) } :rodata
  .gnu.version_d : { *(.gnu.version_d) } :rodata
  .gnu.version_r : { *(.gnu.version_r) } :rodata
  .note.gnu.build-id : { *(.note.gnu.build-id) } :rodata

  /* Read-write sections */

   . = ALIGN(0x1000);

  .data : {
    *(.data .data.*)
    *(.got .got.*)
    *(.got.plt .got.plt.*)
  } :data

  .dynamic : {
    HIDDEN(__dynamic_start = .);
    *(.dynamic)
  } :data :dynamic

  /* BSS section */

  . = ALIGN(0x1000);

  .bss : {
    HIDDEN(__bss_start__ = .);
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(8);
    HIDDEN(__bss_end__ = .);
  } :bss

  . = ALIGN(0x1000);

  __argdata__ = .;

  /DISCARD/ : {
    *(.eh_frame)
    *(.debug_loc)
    *(.debug_str)
  } :NONE
}
