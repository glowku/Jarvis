# JARJARVIS Operating System Makefile
# Complete build system for JARJARVIS OS using Limine
#==============================================================================
# Configuration
#==============================================================================

# Target architecture
ARCH := x86_64
TARGET := $(ARCH)-elf

# Directories
BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/iso
KERNEL_DIR := kernel
LIBC_DIR := libc
FS_DIR := filesystem
DRIVERS_DIR := drivers
AI_DIR := ai_core
GUI_DIR := gui

# Toolchain
CC := $(TARGET)-gcc
CXX := $(TARGET)-g++
AS := nasm
LD := $(TARGET)-ld
AR := $(TARGET)-ar
OBJCOPY := $(TARGET)-objcopy
OBJDUMP := $(TARGET)-objdump

# Flags
CFLAGS := -ffreestanding -O2 -Wall -Wextra -nostdlib -nostartfiles
CFLAGS += -fno-exceptions -fno-stack-protector -fno-pie -fno-pic -mno-red-zone
CFLAGS += -mno-red-zone -mno-mmx -mno-sse -mno-sse2
CFLAGS += -mcmodel=kernel -m64
CFLAGS += -I$(KERNEL_DIR)/include -I$(LIBC_DIR)/include
CFLAGS += -I$(FS_DIR)/include -I$(DRIVERS_DIR)/include
CFLAGS += -I$(AI_DIR)/include -I$(GUI_DIR)/include

# /!\ On rajoute -I./limine pour qu'il trouve limine.h sans faire d'erreur
CFLAGS += -I./limine
CFLAGS += -D__JARJARVIS__ -D$(ARCH)

# /!\ Ajout des flags Miniz et Debug vital pour JARVIS !
CFLAGS += -DMINIZ_NO_STDIO -DMINIZ_NO_ARCHIVE_APIS -g

CXXFLAGS := $(CFLAGS) -std=c++17 -fno-rtti
ASFLAGS := -f elf64 -F dwarf

# /!\ Correction du LDFLAGS pour enlever les permissions RWX
LDFLAGS = -T kernel/linker.ld -nostdlib -z max-page-size=0x1000 -z noexecstack

#==============================================================================
# Source Files
#==============================================================================

# Kernel sources
KERNEL_ASM := $(wildcard $(KERNEL_DIR)/arch/$(ARCH)/*.asm)
KERNEL_C := $(wildcard $(KERNEL_DIR)/src/*.c)
KERNEL_OBJ := $(patsubst $(KERNEL_DIR)/arch/$(ARCH)/%.asm,$(BUILD_DIR)/kernel/%.o,$(KERNEL_ASM))
KERNEL_OBJ += $(patsubst $(KERNEL_DIR)/src/%.c,$(BUILD_DIR)/kernel/%.o,$(KERNEL_C))

# Libc sources
LIBC_C := $(wildcard $(LIBC_DIR)/src/*.c)
LIBC_OBJ := $(patsubst $(LIBC_DIR)/src/%.c,$(BUILD_DIR)/libc/%.o,$(LIBC_C))

# Filesystem sources
FS_C := $(wildcard $(FS_DIR)/src/*.c)
FS_OBJ := $(patsubst $(FS_DIR)/src/%.c,$(BUILD_DIR)/fs/%.o,$(FS_C))

# Driver sources
DRIVERS_C := $(wildcard $(DRIVERS_DIR)/src/*.c)
DRIVERS_OBJ := $(patsubst $(DRIVERS_DIR)/src/%.c,$(BUILD_DIR)/drivers/%.o,$(DRIVERS_C))

# AI Core sources
AI_C := $(wildcard $(AI_DIR)/src/*.c)
AI_OBJ := $(patsubst $(AI_DIR)/src/%.c,$(BUILD_DIR)/ai/%.o,$(AI_C))

# GUI sources
GUI_C := $(wildcard $(GUI_DIR)/src/*.c)
GUI_OBJ := $(patsubst $(GUI_DIR)/src/%.c,$(BUILD_DIR)/gui/%.o,$(GUI_C))

# All objects
ALL_OBJ := $(KERNEL_OBJ) $(LIBC_OBJ) $(FS_OBJ) $(DRIVERS_OBJ) $(AI_OBJ) $(GUI_OBJ)

#==============================================================================
# Targets
#==============================================================================

.PHONY: all clean img iso images run debug info

# Default target
all: $(BUILD_DIR)/jarjarvis.kernel

# Target pour construire l'IMG et l'ISO d'un seul coup
images: img iso

# Create build directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)/kernel
	@mkdir -p $(BUILD_DIR)/libc
	@mkdir -p $(BUILD_DIR)/fs
	@mkdir -p $(BUILD_DIR)/drivers
	@mkdir -p $(BUILD_DIR)/ai
	@mkdir -p $(BUILD_DIR)/gui

# Compile assembly files
$(BUILD_DIR)/kernel/%.o: $(KERNEL_DIR)/arch/$(ARCH)/%.asm | $(BUILD_DIR)
	@echo "AS $<"
	@$(AS) $(ASFLAGS) -o $@ $<

# Compile C files
$(BUILD_DIR)/kernel/%.o: $(KERNEL_DIR)/src/%.c | $(BUILD_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/libc/%.o: $(LIBC_DIR)/src/%.c | $(BUILD_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/fs/%.o: $(FS_DIR)/src/%.c | $(BUILD_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/drivers/%.o: $(DRIVERS_DIR)/src/%.c | $(BUILD_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/ai/%.o: $(AI_DIR)/src/%.c | $(BUILD_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/gui/%.o: $(GUI_DIR)/src/%.c | $(BUILD_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

# Link kernel
$(BUILD_DIR)/jarjarvis.kernel: $(ALL_OBJ) $(KERNEL_DIR)/linker.ld
	@echo "LD $@"
	@$(LD) $(LDFLAGS) -o $@ $(ALL_OBJ)
	@echo "Kernel built successfully!"

# Create HDD Image (.img) - MÉTHODE INFAILLIBLE FAT32 - 256 Mo
img: $(BUILD_DIR)/jarjarvis.img
$(BUILD_DIR)/jarjarvis.img: $(BUILD_DIR)/jarjarvis.kernel
	@echo "Creating Limine HDD image (.img)..."
	@rm -rf $(ISO_DIR)
	@mkdir -p $(ISO_DIR)
	@# 1. Copie des fichiers vitaux
	@cp $(BUILD_DIR)/jarjarvis.kernel $(ISO_DIR)/jarvis.elf
	@cp limine/limine-bios.sys $(ISO_DIR)/
	@# 2. Fichier de config EN .CFG
	@echo "Writing config file (limine.cfg)..."
	@printf "TIMEOUT=0\nVERBOSE=yes\n\n:JARVIS OS\n    PROTOCOL=limine\n    KERNEL_PATH=boot:///jarvis.elf\n    RESOLUTION=1024x768x32\n" > $(ISO_DIR)/limine.cfg
	@# Ajout de l'animation si présente
	@if [ -f "animation.bin" ]; then \
		cp animation.bin $(ISO_DIR)/; \
		printf "    MODULE_PATH=boot:///animation.bin\n" >> $(ISO_DIR)/limine.cfg; \
		echo "      -> Animation module ajouté."; \
	fi
	@# 3. Création du disque de 256 Mo
	@echo "Allocating 256MB disk space..."
	@dd if=/dev/zero of=$@ bs=1M count=256 2>/dev/null
	@# 4. Table des partitions MBR et formatage
	@parted -s $@ mklabel msdos
	@parted -s $@ mkpart primary fat32 2048s 100%
	@parted -s $@ set 1 boot on
	@# 5. Formatage FAT32 et copie
	@echo "Formatting FAT32 partition..."
	@mformat -i $@@@1M -F
	@echo "Copying files to disk..."
	@mcopy -i $@@@1M $(ISO_DIR)/* ::/
	@# 6. Injection de Limine
	@echo "Installing Limine bootloader..."
	@./limine/limine bios-install $@ > /dev/null
	@echo "✅ HDD Image created: $@"

# Create CD-ROM Image (.iso)
iso: $(BUILD_DIR)/jarjarvis.iso
$(BUILD_DIR)/jarjarvis.iso: $(BUILD_DIR)/jarjarvis.kernel
	@echo "Creating Limine pure CD-ROM image (.iso)..."
	@rm -rf $(ISO_DIR)
	@mkdir -p $(ISO_DIR)
	@# 1. Copie des fichiers vitaux
	@cp $(BUILD_DIR)/jarjarvis.kernel $(ISO_DIR)/jarvis.elf
	@cp limine/limine-bios.sys $(ISO_DIR)/
	@cp limine/limine-bios-cd.bin $(ISO_DIR)/ 2>/dev/null || true
	@# 2. Fichier de config EN .CFG
	@echo "Writing config file (limine.cfg)..."
	@printf "TIMEOUT=0\nVERBOSE=yes\n\n:JARVIS OS\n    PROTOCOL=limine\n    KERNEL_PATH=boot:///jarvis.elf\n    RESOLUTION=1024x768x32\n" > $(ISO_DIR)/limine.cfg
	@# Ajout de l'animation si présente
	@if [ -f "animation.bin" ]; then \
		cp animation.bin $(ISO_DIR)/; \
		printf "    MODULE_PATH=boot:///animation.bin\n" >> $(ISO_DIR)/limine.cfg; \
		echo "      -> Animation module ajouté."; \
	fi
	@# 3. Création de l'ISO avec xorriso (Norme stricte pour VirtualBox)
	@echo "Generating ISO with xorriso..."
	@xorriso -as mkisofs -R -J -V "JARVIS" \
		-c boot.cat \
		-b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		$(ISO_DIR) -o $@
	@echo "✅ Pure CD-ROM ISO Image created: $@"


# Run in QEMU (lance l'image .img par défaut)
run: img
	@echo "🚀 Starting JARVIS in QEMU..."
	@qemu-system-$(ARCH) \
		-drive format=raw,file=$(BUILD_DIR)/jarjarvis.img \
		-m 1G \
		-vga virtio \
		-serial stdio \
		-d int,guest_errors \
		-no-reboot -no-shutdown

# Clean build files
clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete!"

# Print build info
info:
	@echo "JARJARVIS Build Information"
	@echo "==========================="
	@echo "Architecture: $(ARCH)"
	@echo "Compiler: $(CC)"
