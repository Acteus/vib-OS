# UnixOS Userspace Makefile

CC = clang
CFLAGS = -target aarch64-unknown-none-elf \
         -ffreestanding -fno-builtin \
         -nostdlib -nostdinc \
         -O2 -Wall -Wextra

LD = ld.lld
LDFLAGS = -nostdlib -static

BUILD_DIR = ../../build/userspace
SYSROOT = ../../build/sysroot

.PHONY: all clean init shell coreutils

all: init shell coreutils

# Ensure build directories exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/bin
	mkdir -p $(BUILD_DIR)/sbin

# Init program
init: $(BUILD_DIR)
	@echo "[BUILD] init"
	$(CC) $(CFLAGS) -c init/init.c -o $(BUILD_DIR)/init.o
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/sbin/init $(BUILD_DIR)/init.o

# Shell
shell: $(BUILD_DIR)
	@echo "[BUILD] shell"
	$(CC) $(CFLAGS) -c shell/shell.c -o $(BUILD_DIR)/shell.o
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/bin/sh $(BUILD_DIR)/shell.o

# Coreutils
coreutils: $(BUILD_DIR)
	@echo "[BUILD] coreutils (placeholder)"
	# Will be built when implemented

clean:
	rm -rf $(BUILD_DIR)
