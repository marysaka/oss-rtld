# inspired by libtransistor-base makefile

# start llvm programs

# On MacOS, brew refuses to install clang5/llvm5 in a global place. As a result,
# they have to muck around with changing the path, which sucks.
# Let's make their lives easier by asking brew where LLVM_CONFIG is.
ifeq ($(shell uname -s),Darwin)
    ifeq ($(shell brew --prefix llvm),)
        $(error need llvm installed via brew)
    else
        LLVM_CONFIG := $(shell brew --prefix llvm)/bin/llvm-config
    endif
else
    LLVM_CONFIG := llvm-config$(LLVM_POSTFIX)
endif

LLVM_BINDIR := $(shell $(LLVM_CONFIG) --bindir)
ifeq ($(LLVM_BINDIR),)
  $(error llvm-config needs to be installed)
endif

LD := $(LLVM_BINDIR)/ld.lld
CC := $(LLVM_BINDIR)/clang
CXX := $(LLVM_BINDIR)/clang++
AS := $(LLVM_BINDIR)/llvm-mc
AR := $(LLVM_BINDIR)/llvm-ar
RANLIB := $(LLVM_BINDIR)/llvm-ranlib
# end llvm programs

SOURCE_ROOT = .
SRC_DIR = $(SOURCE_ROOT)/source
BUILD_DIR := $(SOURCE_ROOT)/build
LIB_DIR = $(BUILD_DIR)/lib/
TARGET_TRIPLET = aarch64-none-switch
LINK_SCRIPT = link.T

# For compiler-rt, we need some system header
SYS_INCLUDES := -isystem $(realpath $(SOURCE_ROOT))/include/ -isystem $(realpath $(SOURCE_ROOT))/misc/system/include
CC_FLAGS := -g -fPIC -fexceptions -fuse-ld=lld -fno-stack-protector -mtune=cortex-a57 -target aarch64-none-linux-gnu -nostdlib -nostdlibinc $(SYS_INCLUDES) -Wno-unused-command-line-argument -Wall -Wextra -O2 -ffunction-sections -fdata-sections
CXX_FLAGS := $(CC_FLAGS) -std=c++17 -stdlib=libc++ -nodefaultlibs -nostdinc++ -fno-rtti -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables
AR_FLAGS := rcs
AS_FLAGS := -arch=aarch64 -triple $(TARGET_TRIPLET)

LD_FLAGS := -T $(SOURCE_ROOT)/misc/rtld.ld \
            -shared \
            --discard-all \
            --strip-all \
            --version-script=$(SOURCE_ROOT)/exported.txt \
            -init=__rtld_init \
            -fini=__rtld_fini \
            --gc-sections \
            -z text \
            --build-id=sha1
# 
# for compatiblity
CFLAGS := $(CC_FLAGS)
CXXFLAGS := $(CXX_FLAGS)

# see https://github.com/MegatonHammer/linkle
LINKLE = linkle

# export
export LD
export CC
export CXX
export AS
export AR
export LD_FOR_TARGET = $(LD)
export CC_FOR_TARGET = $(CC)
export AS_FOR_TARGET = $(AS) -arch=aarch64 -mattr=+neon
export AR_FOR_TARGET = $(AR)
export RANLIB_FOR_TARGET = $(RANLIB)
export CFLAGS_FOR_TARGET = $(CC_FLAGS) -Wno-unused-command-line-argument -Wno-error-implicit-function-declaration

NAME = oss-rtld

all: $(NAME).nso
	@echo $(OBJECTS)

# start compiler-rt definitions
LIB_COMPILER_RT_BUILTINS := $(BUILD_DIR)/compiler-rt/lib/libclang_rt.builtins-aarch64.a
include mk/compiler-rt.mk
# end compiler-rt definitions

CFILES   := $(addprefix $(SRC_DIR)/, $(foreach dir,$(SRC_DIR),$(notdir $(wildcard $(dir)/*.c))))
CPPFILES := $(addprefix $(SRC_DIR)/, $(foreach dir,$(SRC_DIR),$(notdir $(wildcard $(dir)/*.cpp))))
ASMFILES := $(addprefix $(SRC_DIR)/, $(foreach dir,$(SRC_DIR),$(notdir $(wildcard $(dir)/*.s))))

OBJECTS = $(LIB_COMPILER_RT_BUILTINS) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) $(ASMFILES:.s=.o)

%.o: %.s
	$(AS) $(AS_FLAGS) -filetype=obj -o $@ $<

$(NAME).elf: $(OBJECTS)
	$(LD) $(LD_FLAGS) -o $@ $+

%.nso: %.elf
	$(LINKLE) nso $< $@


clean: clean_compiler-rt
	rm -rf $(OBJECTS) $(NAME).elf $(NAME).nso