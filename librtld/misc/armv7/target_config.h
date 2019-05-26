#pragma once

// Mandatory.
#include <elf.h>

// ELF related.
typedef Elf32_Addr Elf_Addr;
typedef Elf32_Rel Elf_Rel;
typedef Elf32_Rela Elf_Rela;
typedef Elf32_Dyn Elf_Dyn;
typedef Elf32_Sym Elf_Sym;
typedef Elf32_Word Elf_Xword;

#define ELF_R_SYM ELF32_R_SYM
#define ELF_R_TYPE ELF32_R_TYPE
#define ELF_ST_BIND ELF32_ST_BIND
#define ELF_ST_VISIBILITY ELF32_ST_VISIBILITY

// Relocation related.
#define ARCH_RELATIVE R_ARM_RELATIVE
#define ARCH_JUMP_SLOT R_ARM_JUMP_SLOT
#define ARCH_GLOB_DAT R_ARM_GLOB_DAT
#define ARCH_IS_REL_ABSOLUTE(type) type == R_ARM_ABS32