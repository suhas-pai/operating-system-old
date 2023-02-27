# Nuke built-in rules and variables.
override MAKEFLAGS += -rR

# This is the name that our final kernel executable will have.
# Change as needed.
override KERNEL := kernel.elf

# Convenience macro to reliably declare overridable command variables.
define DEFAULT_VAR =
    ifeq ($(origin $1),default)
        override $(1) := $(2)
    endif
    ifeq ($(origin $1),undefined)
        override $(1) := $(2)
    endif
endef

# It is highly recommended to use a custom built cross toolchain to build a kernel.
# We are only using "cc" as a placeholder here. It may work by using
# the host system's toolchain, but this is not guaranteed.
$(eval $(call DEFAULT_VAR,CC,x86_64-elf-gcc))
$(eval $(call DEFAULT_VAR,CXX,x86_64-elf-g++))

# Same thing for "ld" (the linker).
$(eval $(call DEFAULT_VAR,LD,x86_64-elf-ld))

COMMON_FLAGS = -g -O2 -pipe -Wall -Wextra -Wno-attributes -Wcast-align       \
			   -Wcast-qual -Wsign-conversion -Werror -Wfloat-equal           \
			   -Wredundant-decls -Wshadow  -Wstrict-overflow=5 -Wundef       \
			   -Wnull-dereference -funsigned-char -Werror=format-security    \

# User controllable CFLAGS.
CFLAGS ?= $(COMMON_FLAGS)

# User controllable preprocessor flags. We set none by default.
CPPFLAGS ?= -std=c++20 $(COMMON_FLAGS)
ifeq ($(shell uname -s),Darwin)
	CPPFLAGS ?= $(CPPFLAGS) -isysroot \
				$(xcode-select -p)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
endif

# User controllable nasm flags.
NASMFLAGS ?= -F dwarf -g

# User controllable linker flags. We set none by default.
LDFLAGS ?=

# Internal C flags that should not be changed by the user.
override CFLAGS +=       \
    -ffreestanding       \
    -fno-stack-protector \
    -fno-stack-check     \
    -fno-lto             \
    -fno-pie             \
    -fno-pic             \
    -m64                 \
    -march=x86-64        \
    -mabi=sysv           \
    -mno-80387           \
    -mno-mmx             \
    -mno-sse             \
    -mno-sse2            \
    -mno-red-zone        \
    -mcmodel=kernel

override CPPFLAGS := \
    -I.              \
    $(CPPFLAGS)      \
    -MMD

# Internal linker flags that should not be changed by the user.
override LDFLAGS +=         \
    -nostdlib               \
    -static                 \
    -m elf_x86_64           \
    -z max-page-size=0x1000 \
    -T linker.ld

# Check if the linker supports -no-pie and enable it if it does.
ifeq ($(shell $(LD) --help 2>&1 | grep 'no-pie' >/dev/null 2>&1; echo $$?),0)
    override LDFLAGS += -no-pie
endif

# Internal nasm flags that should not be changed by the user.
override NASMFLAGS += \
    -f elf64

# Use find to glob all *.c, *.S, and *.asm files in the directory and extract the object names.
override CFILES := $(shell find . -type f -name '*.c')
override CPPFILES := $(shell find . -type f -name '*.cpp')
override ASFILES := $(shell find . -type f -name '*.S')
override NASMFILES := $(shell find . -type f -name '*.asm')
override OBJ := $(CFILES:.c=.o) $(CPPFILES:.cpp=.cpp.o) $(ASFILES:.S=.S.o) $(NASMFILES:.asm=.asm.o)
override HEADER_DEPS := $(CFILES:.c=.d) $(CPPFILES:.cpp=.cpp.d) $(ASFILES:.S=.d)

# Default target.
.PHONY: all
all: $(KERNEL)

limine.h:
	curl https://raw.githubusercontent.com/limine-bootloader/limine/trunk/limine.h -o $@

# Link rules for the final kernel executable.
$(KERNEL): $(OBJ)
	$(LD) $(OBJ) $(LDFLAGS) -o $@

# Include header dependencies.
-include $(HEADER_DEPS)

# Compilation rules for *.c files.
%.o: %.c limine.h
	$(CC) -std=gnu17 $(CFLAGS) -c $< -o $@

%.cpp.o: %.cpp limine.h
	$(CXX) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Compilation rules for *.S files.
%.S.o: %.S limine.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Compilation rules for *.asm (nasm) files.
%.asm.o: %.asm
	nasm $(NASMFLAGS) $< -o $@

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf $(KERNEL) $(OBJ) $(HEADER_DEPS)

.PHONY: distclean
distclean: clean
	rm -f limine.h
