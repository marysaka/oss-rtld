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
SRC_DIR = $(SOURCE_ROOT)/source $(SOURCE_ROOT)/source/$(ARCH)
BUILD_DIR := $(SOURCE_ROOT)/build/$(ARCH)
LINK_SCRIPT = link.T

export VPATH := $(foreach dir,$(SRC_DIR),$(CURDIR)/$(dir)) $(BUILD_DIR)

# For compiler-rt, we need some system header
SYS_INCLUDES := -isystem $(realpath $(SOURCE_ROOT))/include/ -isystem $(realpath $(SOURCE_ROOT))/misc/$(ARCH) -isystem $(realpath $(SOURCE_ROOT))/misc/system/include
CC_FLAGS := -fuse-ld=lld -fno-stack-protector -target $(TARGET_TRIPLET) $(CC_ARCH) -fPIC -nostdlib -nostdlibinc $(SYS_INCLUDES) -Wno-unused-command-line-argument -Wall -Wextra -O2 -ffunction-sections -fdata-sections
CXX_FLAGS := $(CC_FLAGS) -std=c++17 -stdlib=libc++ -nodefaultlibs -nostdinc++ -fno-rtti -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables
AR_FLAGS := rcs
AS_FLAGS := $(AS_ARCH)

# required compiler-rt definitions
LIB_COMPILER_RT_PATH := $(BUILD_DIR)/compiler-rt/build/$(ARCH)/lib
LIB_COMPILER_RT_BUILTINS := $(LIB_COMPILER_RT_PATH)/libclang_rt.builtins-$(COMPILER_RT_ARCH).a

LD_FLAGS := \
            --version-script=$(SOURCE_ROOT)/exported.txt \
            --shared \
            --gc-sections \
            -T $(SOURCE_ROOT)/misc/$(ARCH)/rtld.ld \
            -init=__rtld_init \
            -fini=__rtld_fini \
            -z text \
            --build-id=sha1 \
            -L$(LIB_COMPILER_RT_PATH) \
            -lclang_rt.builtins-$(COMPILER_RT_ARCH) \

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
export AS_FOR_TARGET = $(AS) -arch=$(ARCH)
export AR_FOR_TARGET = $(AR)
export RANLIB_FOR_TARGET = $(RANLIB)
export CFLAGS_FOR_TARGET = $(CC_FLAGS) -Wno-unused-command-line-argument -Wno-error-implicit-function-declaration
export TARGET_TRIPLET

NAME = rtld-$(ARCH)

all: $(NAME).nso

include mk/compiler-rt.mk

CFILES   := $(foreach dir,$(SRC_DIR),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SRC_DIR),$(notdir $(wildcard $(dir)/*.cpp)))
ASMFILES := $(foreach dir,$(SRC_DIR),$(notdir $(wildcard $(dir)/*.s)))

OBJECTS = $(LIB_COMPILER_RT_BUILTINS) $(addprefix $(BUILD_DIR)/, $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) $(ASMFILES:.s=.o))

$(BUILD_DIR)/%.o: %.s
	$(AS) $(AS_FLAGS) -filetype=obj -o $@ $<

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CC_FLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -o $@ -c $<

$(BUILD_DIR)/$(NAME).elf: $(OBJECTS)
	$(LD) $(LD_FLAGS) -o $@ $(OBJECTS)

%.nso: $(BUILD_DIR)/%.elf
	$(LINKLE) nso $< $@

clean: clean_compiler-rt
	rm -rf $(OBJECTS) $(BUILD_DIR)/$(NAME).elf $(NAME).nso