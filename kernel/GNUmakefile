# Nuke built-in rules and variables.
override MAKEFLAGS += -rR

# This is the name that our final kernel executable will have.
# Change as needed.
override KERNEL := kernel.elf

# Convenience macro to reliably declare user overridable variables.
define DEFAULT_VAR =
	ifeq ($(origin $1),default)
		override $(1) := $(2)
	endif
	ifeq ($(origin $1),undefined)
		override $(1) := $(2)
	endif
endef

# Target architecture to build for. Default to x86_64.
override DEFAULT_ARCH := x86_64
$(eval $(call DEFAULT_VAR,ARCH,$(DEFAULT_ARCH)))

# It is highly recommended to use a custom built cross toolchain to build a kernel.
# We are only using "cc" as a placeholder here. It may work by using
# the host system's toolchain, but this is not guaranteed.

ifeq ($(ARCH), x86_64)
	override DEFAULT_CC := $(ARCH)-elf-gcc
else
	override DEFAULT_CC := clang
endif

$(eval $(call DEFAULT_VAR,CC,$(DEFAULT_CC)))

override CC_IS_CLANG := no
ifeq ($(shell $(CC) --version 2>&1 | grep -i 'clang' >/dev/null 2>&1 && echo 1),1)
	override CC_IS_CLANG := yes
endif

override DEFAULT_CXX := clang++
$(eval $(call DEFAULT_VAR,CXX,$(DEFAULT_CXX)))

# Same thing for "ld" (the linker).
override DEFAULT_LD := ld
$(eval $(call DEFAULT_VAR,LD,$(DEFAULT_LD)))

COMMON_FLAGS = -g3 -pipe -Wno-attributes -Wcast-align -Wcast-qual \
			   -Wsign-conversion -Werror -Wfloat-equal -Wredundant-decls \
			   -Wshadow -Wstrict-overflow=2 -Wundef -Wnull-dereference \
			   -funsigned-char -Werror=format-security -I. -I../ \
			   -Iarch/$(ARCH) -DBUILD_KERNEL -Wno-address-of-packed-member \
			   -fno-omit-frame-pointer -fsanitize=undefined

ifeq ($(ARCH), riscv64)
	override COMMON_FLAGS += -D__riscv64
endif

# User controllable C flags.
override DEFAULT_CFLAGS := $(COMMON_FLAGS)
$(eval $(call DEFAULT_VAR,CFLAGS,$(DEFAULT_CFLAGS)))

# User controllable C preprocessor flags. We set none by default.
override DEFAULT_CPPFLAGS :=
$(eval $(call DEFAULT_VAR,CPPFLAGS,$(DEFAULT_CPPFLAGS)))

ifeq ($(ARCH), x86_64)
# User controllable nasm flags.
override DEFAULT_NASMFLAGS := -F dwarf -g
$(eval $(call DEFAULT_VAR,NASMFLAGS,$(DEFAULT_NASMFLAGS)))
endif

# User controllable linker flags. We set none by default.
override DEFAULT_LDFLAGS :=
$(eval $(call DEFAULT_VAR,LDFLAGS,$(DEFAULT_LDFLAGS)))

# User controllable C preprocessor flags. We set none by default.
# User controllable linker flags. We set none by default.
override DEFAULT_LDFLAGS :=
$(eval $(call DEFAULT_VAR,LDFLAGS,$(DEFAULT_LDFLAGS)))

# Internal C flags that should not be changed by the user.
override CFLAGS += \
	-Wall \
	-Wextra \
	-std=gnu11 \
	-ffreestanding \
	-fms-extensions \
	-fno-stack-protector \
	-fno-stack-check \
	-fno-lto \
	-fPIE

# Internal C preprocessor flags that should not be changed by the user.
override CPPFLAGS := \
	-I.              \
	$(CPPFLAGS)      \
	-MMD             \
	-MP

ifeq ($(ARCH),x86_64)
	# Internal nasm flags that should not be changed by the user.
	override NASMFLAGS += \
		-Wall
endif

# Architecture specific internal flags.
ifeq ($(ARCH),x86_64)
	ifeq ($(CC_IS_CLANG),yes)
		override CFLAGS += -target x86_64-elf
	endif

	override CFLAGS += \
		-m64 \
		-march=x86-64 \
		-mabi=sysv \
		-mno-80387 \
		-mno-mmx \
		-mno-sse \
		-mno-sse2 \
		-mno-red-zone
	override LDFLAGS += \
		-m elf_x86_64
	override NASMFLAGS += \
		-f elf64
else ifeq ($(ARCH),aarch64)
	ifeq ($(CC_IS_CLANG),yes)
		override CFLAGS += -target aarch64-elf
	endif

	override CFLAGS += \
		-mgeneral-regs-only
	override LDFLAGS += \
		-m aarch64elf
else ifeq ($(ARCH),riscv64)
	ifeq ($(CC_IS_CLANG),yes)
		override CFLAGS += \
			-target riscv64-elf \
			-march=rv64imac
	else
		override CFLAGS += \
			-march=rv64imac_zicsr_zifencei
	endif
	override CFLAGS += \
		-mabi=lp64 \
		-mno-relax
	override LDFLAGS += \
        -m elf64lriscv \
        --no-relax
else
	$(error Architecture $(ARCH) not supported)
endif

# Internal linker flags that should not be changed by the user.
override LDFLAGS += \
	-nostdlib \
	-static \
	--no-dynamic-linker \
	-z text \
	-z max-page-size=0x1000 \
	-T arch/$(ARCH)/linker.ld \
	-pie

define FIND_FILES
	override $(1) += $(shell find . -type f -name $(2) -not -path "./arch/*")
	override $(1) += $(shell find arch/$(ARCH) -type f -name $(2))
endef

# Use "find" to glob all *.c, *.S, and *.asm files in the tree and obtain the
# object and header dependency file names.
$(eval $(call FIND_FILES,CFILES, "*.c"))
$(eval $(call FIND_FILES,ASFILES, "*.S"))
$(eval $(call FIND_FILES,NASMFILES, "*.asm"))

LIBFILES += \
	../lib/convert.c ../lib/ctype.c ../lib/parse_printf.c ../lib/align.c \
	../lib/format.c ../lib/adt/string_view.c ../lib/adt/range.c ../lib/util.c \
	../lib/refcount.c ../lib/adt/string.c ../lib/time.c \
	../lib/parse_strftime.c ../lib/adt/mutable_buffer.c \
	../lib/adt/growable_buffer.c ../lib/string.c ../lib/adt/avltree.c \
	../lib/adt/array.c ../lib/math.c ../lib/adt/bitmap.c ../lib/bits.c \
	../lib/memory.c ../lib/adt/addrspace.c

override OBJ := $(foreach obj, $(CFILES:./%=%), obj/$(basename $(obj)).o) \
				$(foreach obj, $(ASFILES:./%=%), obj/$(basename $(obj)).S.o) \
				$(foreach obj, $(NASMFILES:./%=%), obj/$(basename $(obj)).asm.o) \
				$(foreach obj, $(LIBFILES:../lib/%=lib/%), obj/$(basename $(obj)).c.o)

override HEADER_DEPS := $(CFILES:.c=.d) $(ASFILES:.S=.d)

# Default target.
.PHONY: all
all: $(KERNEL)

limine.h:
	curl -Lo $@ https://github.com/limine-bootloader/limine/raw/trunk/limine.h

# Link rules for the final kernel executable.
$(KERNEL): $(OBJ)
	$(LD) $(OBJ) $(LDFLAGS) -o $@

# Include header dependencies.
-include $(HEADER_DEPS)

# Compilation rules for *.c files.
obj/%.o: %.c limine.h
	@mkdir -p $(dir $@)
	$(CC) -std=gnu17 $(CFLAGS) -c $< -o $@

obj/lib/%.c.o: ../lib/%.c
	@mkdir -p $(dir $@)
	$(CC) -std=gnu17 $(CFLAGS) -c $< -o $@

# Compilation rules for *.S files.
obj/%.S.o: %.S limine.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Compilation rules for *.asm (nasm) files.
obj/%.asm.o: %.asm
	@mkdir -p $(dir $@)
	nasm $(NASMFLAGS) $< -o $@

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf $(KERNEL) $(HEADER_DEPS)
	rm -rf obj

.PHONY: distclean
distclean: clean
	rm -f limine.h

printv:
	@echo $(OBJ)
