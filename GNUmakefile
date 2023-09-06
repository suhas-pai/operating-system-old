# Nuke built-in rules and variables.
override MAKEFLAGS += -rR

override IMAGE_NAME := barebones

# Convenience macro to reliably declare user overridable variables.
define DEFAULT_VAR =
	ifeq ($(origin $1),default)
		override $(1) := $(2)
	endif
	ifeq ($(origin $1),undefined)
		override $(1) := $(2)
	endif
endef

# Compiler for building the 'limine' executable for the host.
override DEFAULT_HOST_CC := cc
$(eval $(call DEFAULT_VAR,HOST_CC,$(DEFAULT_HOST_CC)))

# Target architecture to build for. Default to x86_64.
override DEFAULT_ARCH := x86_64
$(eval $(call DEFAULT_VAR,ARCH,$(DEFAULT_ARCH)))

EXTRA_QEMU_ARGS=-d guest_errors -d unimp -d int -D ./log.txt -rtc base=localtime
ifeq ($(DEBUG), 1)
	EXTRA_QEMU_ARGS += -s -S -serial stdio
else ifeq ($(CONSOLE), 1)
	EXTRA_QEMU_ARGS += -s -S -monitor stdio -no-reboot -no-shutdown
else
	EXTRA_QEMU_ARGS += -serial stdio
endif

.PHONY: all
all: $(IMAGE_NAME).iso

.PHONY: all-hdd
all-hdd: $(IMAGE_NAME).hdd

.PHONY: run
run: run-$(ARCH)

.PHONY: run-hdd
run-hdd: run-hdd-$(ARCH)

.PHONY: run-x86_64
run-x86_64: ovmf-x86_64 $(IMAGE_NAME).iso
	qemu-system-x86_64 -M q35 -cpu max -m 2G -bios ovmf-x86_64/OVMF.fd -cdrom $(IMAGE_NAME).iso -boot d $(EXTRA_QEMU_ARGS)

.PHONY: run-hdd-x86_64
run-hdd-x86_64: ovmf-x86_64 $(IMAGE_NAME).hdd
	qemu-system-x86_64 -M q35 -cpu max -m 2G -bios ovmf-x86_64/OVMF.fd -hda $(IMAGE_NAME).hdd $(EXTRA_QEMU_ARGS)

.PHONY: run-aarch64
run-aarch64: ovmf-aarch64 $(IMAGE_NAME).iso
	qemu-system-aarch64 -M virt -cpu cortex-a72 -device ramfb -device qemu-xhci -device usb-kbd -m 2G -bios ovmf-aarch64/OVMF.fd -cdrom $(IMAGE_NAME).iso -boot d $(EXTRA_QEMU_ARGS)

.PHONY: run-hdd-aarch64
run-hdd-aarch64: ovmf-aarch64 $(IMAGE_NAME).hdd
	qemu-system-aarch64 -M virt -cpu cortex-a72 -device ramfb -device qemu-xhci -device usb-kbd -m 2G -bios ovmf-aarch64/OVMF.fd -hda $(IMAGE_NAME).hdd $(EXTRA_QEMU_ARGS)

.PHONY: run-riscv64
run-riscv64: ovmf-riscv64 $(IMAGE_NAME).iso
	qemu-system-riscv64 -M virt,acpi=off -cpu rv64 -device ramfb -device qemu-xhci -device usb-kbd -m 2G -drive if=pflash,unit=1,format=raw,file=ovmf-riscv64/OVMF.fd -device virtio-scsi-pci,id=scsi -device scsi-cd,drive=cd0 -drive id=cd0,format=raw,file=$(IMAGE_NAME).iso $(EXTRA_QEMU_ARGS)

.PHONY: run-hdd-riscv64
run-hdd-riscv64: ovmf-riscv64 $(IMAGE_NAME).hdd
	qemu-system-riscv64 -M virt,acpi=off -cpu rv64 -device ramfb -device qemu-xhci -device usb-kbd -m 2G -drive if=pflash,unit=1,format=raw,file=ovmf-riscv64/OVMF.fd -device virtio-scsi-pci,id=scsi -device scsi-hd,drive=hd0 -drive id=hd0,format=raw,file=$(IMAGE_NAME).hdd $(EXTRA_QEMU_ARGS)

.PHONY: run-bios
run-bios: $(IMAGE_NAME).iso
	qemu-system-x86_64 -M q35 -cpu max -m 2G -cdrom $(IMAGE_NAME).iso -boot d $(EXTRA_QEMU_ARGS)

.PHONY: run-hdd-bios
run-hdd-bios: $(IMAGE_NAME).hdd
	qemu-system-x86_64 -M q35 -cpu max -m 2G -hda $(IMAGE_NAME).hdd $(EXTRA_QEMU_ARGS)

.PHONY: ovmf
ovmf: ovmf-$(ARCH)

ovmf-x86_64:
	mkdir -p ovmf-x86_64
	cd ovmf-x86_64 && curl -o OVMF.fd https://retrage.github.io/edk2-nightly/bin/RELEASEX64_OVMF.fd

ovmf-aarch64:
	mkdir -p ovmf-aarch64
	cd ovmf-aarch64 && curl -o OVMF.fd https://retrage.github.io/edk2-nightly/bin/RELEASEAARCH64_QEMU_EFI.fd

ovmf-riscv64:
	mkdir -p ovmf-riscv64
	cd ovmf-riscv64 && curl -o OVMF.fd https://retrage.github.io/edk2-nightly/bin/RELEASERISCV64_VIRT.fd && dd if=/dev/zero of=OVMF.fd bs=1 count=0 seek=33554432

limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v5.x-branch-binary --depth=1
	unset CC; unset CFLAGS; unset CPPFLAGS; unset LDFLAGS; unset LIBS; $(MAKE) -C limine CC="$(HOST_CC)"

.PHONY: kernel
kernel:
	$(MAKE) -C kernel

$(IMAGE_NAME).iso: limine kernel
	rm -rf iso_root
	mkdir -p iso_root
	cp kernel/kernel.elf limine.cfg iso_root/
	mkdir -p iso_root/EFI/BOOT
ifeq ($(ARCH),x86_64)
	cp -v limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/
	cp -v limine/BOOTX64.EFI iso_root/EFI/BOOT/
	cp -v limine/BOOTIA32.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
	./limine/limine bios-install $(IMAGE_NAME).iso
else ifeq ($(ARCH),aarch64)
	cp -v limine/limine-uefi-cd.bin iso_root/
	cp -v limine/BOOTAA64.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
else ifeq ($(ARCH),riscv64)
	cp -v limine/limine-uefi-cd.bin iso_root/
	cp -v limine/BOOTRISCV64.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
endif
	rm -rf iso_root

$(IMAGE_NAME).hdd: limine kernel
	rm -f $(IMAGE_NAME).hdd
	dd if=/dev/zero bs=1M count=0 seek=64 of=$(IMAGE_NAME).hdd
	sgdisk $(IMAGE_NAME).hdd -n 1:2048 -t 1:ef00
ifeq ($(ARCH),x86_64)
	./limine/limine bios-install $(IMAGE_NAME).hdd
endif
	mformat -i $(IMAGE_NAME).hdd@@1M
	mmd -i $(IMAGE_NAME).hdd@@1M ::/EFI ::/EFI/BOOT
	mcopy -i $(IMAGE_NAME).hdd@@1M kernel/kernel.elf limine.cfg ::/
ifeq ($(ARCH),x86_64)
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/limine-bios.sys ::/EFI/BOOT
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTX64.EFI ::/EFI/BOOT
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTIA32.EFI ::/EFI/BOOT
else ifeq ($(ARCH),aarch64)
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTAA64.EFI ::/EFI/BOOT
else ifeq ($(ARCH),riscv64)
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTRISCV64.EFI ::/EFI/BOOT
endif

.PHONY: clean
clean:
	rm -rf iso_root $(IMAGE_NAME).iso $(IMAGE_NAME).hdd
	$(MAKE) -C kernel clean

.PHONY: distclean
distclean: clean
	rm -rf limine ovmf*
	$(MAKE) -C kernel distclean
