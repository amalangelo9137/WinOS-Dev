#include "Shared.h"

static char NormalizeAssetChar(char value) {
    if (value >= 'a' && value <= 'z') {
        return (char)(value - ('a' - 'A'));
    }

    return value;
}

static bool AssetNameEquals(const char* left, const char* right) {
    for (int index = 0; index < 32; index++) {
        if (NormalizeAssetChar(left[index]) != NormalizeAssetChar(right[index])) {
            return false;
        }

        if (left[index] == '\0') {
            return true;
        }
    }

    return true;
}

ASSET_ENTRY* GetAssetEntry(const char* name, BOOT_CONFIG* config) {
    if (config == nullptr || config->Assets == nullptr || name == nullptr) {
        return nullptr;
    }

    for (uint32_t i = 0; i < config->Assets->Count; i++) {
        if (AssetNameEquals(config->Assets->Entries[i].Name, name)) {
            return &config->Assets->Entries[i];
        }
    }

    return nullptr;
}

void* GetAsset(const char* name, BOOT_CONFIG* config) {
    ASSET_ENTRY* entry = GetAssetEntry(name, config);
    if (entry == nullptr) {
        return nullptr;
    }

    return entry->Address;
}
