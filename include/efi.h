#ifndef EFI_H
#define EFI_H

// EFI status codes
#define EFI_SUCCESS 0
#define EFI_ERROR 1

// EFI data structures
typedef struct {
    unsigned long long Signature;
    unsigned long long Revision;
    void *Header;
} EFI_SYSTEM_TABLE;

// Function prototypes
EFI_SYSTEM_TABLE* EFIAPI InitializeSystemTable(void);
void EFIAPI LoadKernel(const char* kernelPath);

#endif // EFI_H