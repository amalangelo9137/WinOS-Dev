#include "Shared.h"

void* GetAsset(const char* name, BOOT_CONFIG* config) {
    for (uint32_t i = 0; i < config->Assets->Count; i++) {
        bool match = true;
        for (int j = 0; j < 32; j++) {
            if (config->Assets->Entries[i].Name[j] != name[j]) { match = false; break; }
            if (name[j] == '\0') break;
        }
        if (match) return config->Assets->Entries[i].Address;
    }
    return nullptr;
}