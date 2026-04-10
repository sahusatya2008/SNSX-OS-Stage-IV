TARGET := SNSX
ARCH_PREFIX ?= i686-elf-
CC = $(ARCH_PREFIX)gcc
LD = $(ARCH_PREFIX)ld
OBJCOPY = $(ARCH_PREFIX)objcopy
AS = nasm
GRUB_MKRESCUE ?= $(ARCH_PREFIX)grub-mkrescue
PYTHON ?= python3
LLVM_PREFIX ?= /opt/homebrew/opt/llvm/bin
LLD_PREFIX ?= /opt/homebrew/opt/lld/bin
MTOOLS_PREFIX ?= /opt/homebrew/bin
ARM64_CC ?= $(LLVM_PREFIX)/clang
ARM64_LD ?= $(LLD_PREFIX)/lld-link
MFORMAT ?= $(MTOOLS_PREFIX)/mformat
MMD ?= $(MTOOLS_PREFIX)/mmd
MCOPY ?= $(MTOOLS_PREFIX)/mcopy
QEMU_AARCH64 ?= qemu-system-aarch64
QEMU_AARCH64_EFI ?= $(shell ls -1 /opt/homebrew/Cellar/qemu/*/share/qemu/edk2-aarch64-code.fd 2>/dev/null | tail -n 1)

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin
ROOTFS_IMG := $(BUILD_DIR)/rootfs.img
CUSTOM_BOOT := $(BUILD_DIR)/boot.bin
CUSTOM_STAGE2 := $(BUILD_DIR)/stage2.bin
ISO_OUTPUT := $(TARGET).iso
IMG_OUTPUT := $(TARGET).img
ARM64_BUILD_DIR := build-arm64
ARM64_OBJ_DIR := $(ARM64_BUILD_DIR)/obj
ARM64_EFI := $(ARM64_BUILD_DIR)/BOOTAA64.EFI
ARM64_BOOT_IMG := $(ARM64_BUILD_DIR)/efiboot.img
ARM64_ISO_OUTPUT := SNSX-VBox-ARM64.iso
ARM64_ISO_ROOT := iso-arm64
ARM64_PERSIST_RAW := SNSX-PERSIST.raw

CFLAGS := -std=gnu11 -ffreestanding -O2 -Wall -Wextra -Wpedantic -Wno-unused-parameter -m32 -fno-pic -fno-stack-protector -fno-builtin -fno-omit-frame-pointer -Iinclude
LDFLAGS := -T kernel/linker.ld -nostdlib
NASM_ELF_FLAGS := -f elf32
NASM_BIN_FLAGS := -f bin
ARM64_CFLAGS := --target=aarch64-unknown-windows -ffreestanding -fshort-wchar -fno-stack-protector -fno-builtin -nostdlib -Wall -Wextra -Wpedantic -Iinclude -Iarm64/include

C_SOURCES := \
	kernel/kmain.c \
	kernel/interrupts.c \
	kernel/paging.c \
	kernel/syscall.c \
	kernel/fs.c \
	kernel/task.c \
	kernel/shell.c \
	drivers/gdt.c \
	drivers/idt.c \
	drivers/vga.c \
	drivers/timer.c \
	drivers/keyboard.c \
	drivers/ata.c \
	lib/string.c

ASM_SOURCES := \
	boot/kernel_entry.asm \
	boot/gdt_flush.asm \
	boot/idt_flush.asm \
	boot/isr_stub.asm

PROGRAM_OUTPUTS := rootfs/bin/hello.app rootfs/bin/clear.app
ROOTFS_TEXT_INPUTS := $(shell find rootfs -type f ! -name '*.app' | sort)
ROOTFS_INPUTS := $(ROOTFS_TEXT_INPUTS) $(PROGRAM_OUTPUTS)

C_OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SOURCES))
ASM_OBJECTS := $(patsubst %.asm,$(OBJ_DIR)/%.o,$(ASM_SOURCES))
OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)
ARM64_SOURCES := arm64/kernel/main.c lib/string.c
ARM64_OBJECTS := $(patsubst %.c,$(ARM64_OBJ_DIR)/%.obj,$(ARM64_SOURCES))

.PHONY: all iso custom-image clean run vbox-arm64 virtualbox run-arm64 run-arm64-headless run-arm64-persist vbox-arm64-vm virtualbox-vm virtualbox-refresh virtualbox-repair virtualbox-fresh virtualbox-online https-bridge https-bridge-stop https-bridge-status dev-bridge dev-bridge-stop dev-bridge-status

all: $(ISO_OUTPUT)

iso: $(ISO_OUTPUT)

custom-image: $(IMG_OUTPUT)

vbox-arm64: $(ARM64_ISO_OUTPUT)

virtualbox: $(ARM64_ISO_OUTPUT)

vbox-arm64-vm: $(ARM64_ISO_OUTPUT)
	./tools/create_virtualbox_arm64_vm.sh "$(VM_NAME)" "$(abspath $(ARM64_ISO_OUTPUT))"

virtualbox-vm: vbox-arm64-vm

virtualbox-refresh: $(ARM64_ISO_OUTPUT)
	./tools/create_virtualbox_arm64_vm.sh "$(VM_NAME)" "$(abspath $(ARM64_ISO_OUTPUT))"

virtualbox-repair: virtualbox-refresh

virtualbox-fresh: $(ARM64_ISO_OUTPUT)
	REPLACE=1 RECREATE_DISK=1 ./tools/create_virtualbox_arm64_vm.sh "$(VM_NAME)" "$(abspath $(ARM64_ISO_OUTPUT))"

virtualbox-online: $(ARM64_ISO_OUTPUT)
	VM_NAME="$(VM_NAME)" ISO_PATH="$(abspath $(ARM64_ISO_OUTPUT))" ./tools/start_virtualbox_online.sh

https-bridge:
	./tools/run_https_bridge.sh

https-bridge-stop:
	pkill -f "tools/snsx_https_bridge.py" || true

https-bridge-status:
	lsof -nP -iTCP:8787 -sTCP:LISTEN

dev-bridge:
	./tools/run_dev_bridge.sh

dev-bridge-stop:
	pkill -f "tools/snsx_dev_bridge.py" || true

dev-bridge-status:
	lsof -nP -iTCP:8790 -sTCP:LISTEN

$(ISO_OUTPUT): $(KERNEL_ELF) $(ROOTFS_IMG) iso/boot/grub/grub.cfg
	mkdir -p iso/boot
	cp $(KERNEL_ELF) iso/boot/kernel.elf
	cp $(ROOTFS_IMG) iso/boot/rootfs.img
	$(GRUB_MKRESCUE) -o $@ iso

$(IMG_OUTPUT): $(CUSTOM_BOOT) $(CUSTOM_STAGE2) $(KERNEL_BIN)
	dd if=/dev/zero of=$@ bs=512 count=2880
	dd if=$(CUSTOM_BOOT) of=$@ conv=notrunc
	dd if=$(CUSTOM_STAGE2) of=$@ conv=notrunc seek=1
	dd if=$(KERNEL_BIN) of=$@ conv=notrunc seek=18

$(ARM64_ISO_OUTPUT): $(ARM64_BOOT_IMG)
	mkdir -p $(ARM64_ISO_ROOT)/EFI/BOOT
	cp $(ARM64_BOOT_IMG) $(ARM64_ISO_ROOT)/efiboot.img
	cp $(ARM64_EFI) $(ARM64_ISO_ROOT)/EFI/BOOT/BOOTAA64.EFI
	xorriso -as mkisofs -R -J -V SNSXARM64 -e efiboot.img -no-emul-boot -o $@ $(ARM64_ISO_ROOT)

$(KERNEL_ELF): $(OBJECTS) kernel/linker.ld
	mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

$(ARM64_EFI): $(ARM64_OBJECTS)
	mkdir -p $(dir $@)
	$(ARM64_LD) /subsystem:efi_application /entry:efi_main /nodefaultlib /machine:arm64 /out:$@ $(ARM64_OBJECTS)

$(ARM64_BOOT_IMG): $(ARM64_EFI)
	mkdir -p $(dir $@)
	rm -f $@
	$(MFORMAT) -i $@ -C -f 1440 ::
	$(MMD) -i $@ ::EFI ::EFI/BOOT
	$(MCOPY) -i $@ $(ARM64_EFI) ::EFI/BOOT/BOOTAA64.EFI

$(ARM64_PERSIST_RAW):
	qemu-img create -f raw $@ 64M
	$(MFORMAT) -i $@ -F ::

$(ROOTFS_IMG): tools/mkinitrd.py $(ROOTFS_INPUTS)
	$(PYTHON) tools/mkinitrd.py $@ rootfs

$(CUSTOM_BOOT): boot/custom_boot.asm
	mkdir -p $(dir $@)
	$(AS) $(NASM_BIN_FLAGS) -o $@ $<

$(CUSTOM_STAGE2): boot/custom_stage2.asm $(KERNEL_BIN)
	mkdir -p $(dir $@)
	KERNEL_SECTORS=$$((($$(wc -c < $(KERNEL_BIN)) + 511) / 512)); \
	$(AS) $(NASM_BIN_FLAGS) -D KERNEL_SECTORS=$$KERNEL_SECTORS -o $@ $<
	test $$(wc -c < $@) -le $$((17 * 512))

rootfs/bin/%.app: programs/%.asm
	mkdir -p $(dir $@)
	$(AS) $(NASM_BIN_FLAGS) -o $@ $<

$(OBJ_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.asm
	mkdir -p $(dir $@)
	$(AS) $(NASM_ELF_FLAGS) -o $@ $<

$(ARM64_OBJ_DIR)/%.obj: %.c
	mkdir -p $(dir $@)
	$(ARM64_CC) $(ARM64_CFLAGS) -c $< -o $@

run: $(ISO_OUTPUT)
	qemu-system-i386 -cdrom $(ISO_OUTPUT) -m 512M

run-arm64: $(ARM64_ISO_OUTPUT)
	$(QEMU_AARCH64) -machine virt -cpu cortex-a72 -m 512M -bios $(QEMU_AARCH64_EFI) -drive if=none,format=raw,file=$(ARM64_ISO_OUTPUT),id=cdrom -device virtio-scsi-pci -device scsi-cd,drive=cdrom -netdev user,id=n1 -device e1000,netdev=n1

run-arm64-headless: $(ARM64_ISO_OUTPUT)
	$(QEMU_AARCH64) -machine virt -cpu cortex-a72 -m 512M -bios $(QEMU_AARCH64_EFI) -drive if=none,format=raw,file=$(ARM64_ISO_OUTPUT),id=cdrom -device virtio-scsi-pci -device scsi-cd,drive=cdrom -netdev user,id=n1 -device e1000,netdev=n1 -nographic

run-arm64-persist: $(ARM64_ISO_OUTPUT) $(ARM64_PERSIST_RAW)
	$(QEMU_AARCH64) -machine virt -cpu cortex-a72 -m 512M -bios $(QEMU_AARCH64_EFI) -drive if=none,format=raw,file=$(ARM64_ISO_OUTPUT),id=cdrom -device virtio-scsi-pci -device scsi-cd,drive=cdrom -drive if=none,format=raw,file=$(ARM64_PERSIST_RAW),id=persist -device virtio-blk-pci,drive=persist -netdev user,id=n1 -device e1000,netdev=n1

clean:
	rm -rf build
	rm -rf build-arm64
	rm -f SNSX.iso SNSX.img myOS.iso myOS.img
	rm -f SNSX-VBox-ARM64.iso
	rm -f iso/boot/kernel.elf iso/boot/rootfs.img
	rm -rf iso-arm64
	rm -f rootfs/bin/*.app
