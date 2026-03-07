@echo off
SET QEMU_PATH="C:\Program Files\qemu\qemu-system-x86_64.exe"
SET BIOS_PATH="..\OVMF_X64.fd"
SET DRIVE_PATH="..\VirtualDrive"

%QEMU_PATH% -bios %BIOS_PATH% -drive format=raw,file=fat:rw:%DRIVE_PATH% -net none