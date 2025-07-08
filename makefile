EFI_INC = include
BOOT_SRC = boot/bootloader.c
KERNEL_SRC = kernel/kernel.c
UTILS_SRC = utils/utils.c

CC = gcc
CFLAGS = -I$(EFI_INC) -ffreestanding -m64 -O2

all: bootloader.efi kernel.efi

bootloader.efi: $(BOOT_SRC) $(EFI_INC)/efi.h
    $(CC) $(CFLAGS) -o bootloader.efi $(BOOT_SRC)

kernel.efi: $(KERNEL_SRC) $(UTILS_SRC) $(EFI_INC)/efi.h
    $(CC) $(CFLAGS) -o kernel.efi $(KERNEL_SRC) $(UTILS_SRC)

clean:
    del /Q *.efi 2>nul || rm -f *.efi

.PHONY: all clean