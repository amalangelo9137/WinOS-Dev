#include "Wdexe.h"

#include "../InitFS/Assets.h"

#pragma pack(push, 1)
struct WdexeHeader {
    char Magic[4];
    uint16_t Version;
    uint16_t HeaderSize;
    uint32_t CanvasWidth;
    uint32_t CanvasHeight;
    uint32_t RootElementId;
    uint32_t ElementCount;
    uint32_t EventCount;
    uint32_t BindingCount;
    uint32_t AssetCount;
    uint32_t StringTableSize;
    uint32_t ElementOffset;
    uint32_t EventOffset;
    uint32_t BindingOffset;
    uint32_t AssetOffset;
    uint32_t StringOffset;
};

struct WdexeElementRecord {
    uint32_t Id;
    int32_t ParentId;
    uint32_t Type;
    uint32_t Flags;
    int32_t X;
    int32_t Y;
    int32_t Width;
    int32_t Height;
    int32_t ZIndex;
    uint32_t BackgroundColor;
    uint32_t ForegroundColor;
    uint32_t BorderColor;
    int32_t Value;
    int32_t MinValue;
    int32_t MaxValue;
    uint32_t TextOffset;
    uint32_t AssetIndex;
};

struct WdexeEventRecord {
    uint32_t SourceElementId;
    uint32_t EventType;
    uint32_t FirstBindingIndex;
    uint32_t BindingCount;
};

struct WdexeBindingRecord {
    uint32_t ActionType;
    uint32_t TargetElementId;
    uint32_t PropertyType;
    int32_t IntValue;
    uint32_t ColorValue;
    uint32_t StringOffset;
};

struct WdexeAssetRecord {
    uint32_t AliasOffset;
};
#pragma pack(pop)

static const uint32_t INVALID_OFFSET = 0xFFFFFFFF;
static const uint32_t INVALID_ASSET_INDEX = 0xFFFFFFFF;

static void CopyText(char* destination, int capacity, const char* source) {
    if (capacity <= 0) {
        return;
    }

    if (source == nullptr) {
        destination[0] = '\0';
        return;
    }

    int index = 0;
    while (index < capacity - 1 && source[index] != '\0') {
        destination[index] = source[index];
        index++;
    }

    destination[index] = '\0';
}

static void ResetScene(AppScene* scene) {
    if (scene == nullptr) {
        return;
    }

    scene->Loaded = false;
    scene->WindowOpen = false;
    scene->CanvasWidth = 0;
    scene->CanvasHeight = 0;
    scene->RootElementId = 0;
    scene->ElementCount = 0;
    scene->EventCount = 0;
    scene->BindingCount = 0;
    scene->AssetCount = 0;
    scene->PressedElementId = -1;
    scene->CapturedElementId = -1;
    scene->FocusedElementId = -1;
    scene->SurfaceDirty = false;

    for (int index = 0; index < APP_MAX_ELEMENTS; index++) {
        scene->Elements[index].Id = 0;
        scene->Elements[index].ParentId = -1;
        scene->Elements[index].Type = 0;
        scene->Elements[index].Flags = 0;
        scene->Elements[index].X = 0;
        scene->Elements[index].Y = 0;
        scene->Elements[index].Width = 0;
        scene->Elements[index].Height = 0;
        scene->Elements[index].ZIndex = 0;
        scene->Elements[index].BackgroundColor = 0;
        scene->Elements[index].ForegroundColor = 0;
        scene->Elements[index].BorderColor = 0;
        scene->Elements[index].Value = 0;
        scene->Elements[index].MinValue = 0;
        scene->Elements[index].MaxValue = 0;
        scene->Elements[index].Text[0] = '\0';
        scene->Elements[index].AssetAlias[0] = '\0';
        scene->Elements[index].AssetAddress = nullptr;
    }

    for (int index = 0; index < APP_MAX_EVENTS; index++) {
        scene->Events[index].SourceElementId = 0;
        scene->Events[index].EventType = 0;
        scene->Events[index].FirstBindingIndex = 0;
        scene->Events[index].BindingCount = 0;
    }

    for (int index = 0; index < APP_MAX_BINDINGS; index++) {
        scene->Bindings[index].ActionType = 0;
        scene->Bindings[index].TargetElementId = 0;
        scene->Bindings[index].PropertyType = 0;
        scene->Bindings[index].IntValue = 0;
        scene->Bindings[index].ColorValue = 0;
        scene->Bindings[index].StringValue[0] = '\0';
    }

    for (int index = 0; index < APP_MAX_ASSETS; index++) {
        scene->AssetAliases[index][0] = '\0';
    }
}

static bool SectionFits(uint32_t offset, uint32_t size, uint32_t assetSize) {
    if (offset > assetSize) {
        return false;
    }

    return size <= (assetSize - offset);
}

static const char* ResolveString(const uint8_t* packageBytes, const WdexeHeader* header, uint32_t offset, uint32_t packageSize) {
    if (offset == INVALID_OFFSET) {
        return nullptr;
    }

    if (offset >= header->StringTableSize) {
        return nullptr;
    }

    uint32_t absoluteOffset = header->StringOffset + offset;
    if (absoluteOffset >= packageSize) {
        return nullptr;
    }

    return (const char*)(packageBytes + absoluteOffset);
}

AppElement* FindAppElement(AppScene* scene, uint32_t elementId) {
    if (scene == nullptr) {
        return nullptr;
    }

    for (int index = 0; index < scene->ElementCount; index++) {
        if (scene->Elements[index].Id == elementId) {
            return &scene->Elements[index];
        }
    }

    return nullptr;
}

const AppElement* FindAppElement(const AppScene* scene, uint32_t elementId) {
    if (scene == nullptr) {
        return nullptr;
    }

    for (int index = 0; index < scene->ElementCount; index++) {
        if (scene->Elements[index].Id == elementId) {
            return &scene->Elements[index];
        }
    }

    return nullptr;
}

bool LoadAppScene(BOOT_CONFIG* config, const char* packageAlias, AppScene* scene) {
    ResetScene(scene);

    if (config == nullptr || scene == nullptr || packageAlias == nullptr || packageAlias[0] == '\0') {
        return false;
    }

    ASSET_ENTRY* package = GetAssetEntry(packageAlias, config);
    if (package == nullptr || package->Address == nullptr || package->Size < sizeof(WdexeHeader)) {
        return false;
    }

    const uint8_t* packageBytes = (const uint8_t*)package->Address;
    const WdexeHeader* header = (const WdexeHeader*)packageBytes;

    if (header->Magic[0] != 'W' || header->Magic[1] != 'D' || header->Magic[2] != 'X' || header->Magic[3] != 'E') {
        return false;
    }

    if (header->Version != 1 || header->HeaderSize < sizeof(WdexeHeader)) {
        return false;
    }

    if (header->ElementCount > APP_MAX_ELEMENTS ||
        header->EventCount > APP_MAX_EVENTS ||
        header->BindingCount > APP_MAX_BINDINGS ||
        header->AssetCount > APP_MAX_ASSETS) {
        return false;
    }

    if (!SectionFits(header->ElementOffset, header->ElementCount * sizeof(WdexeElementRecord), package->Size) ||
        !SectionFits(header->EventOffset, header->EventCount * sizeof(WdexeEventRecord), package->Size) ||
        !SectionFits(header->BindingOffset, header->BindingCount * sizeof(WdexeBindingRecord), package->Size) ||
        !SectionFits(header->AssetOffset, header->AssetCount * sizeof(WdexeAssetRecord), package->Size) ||
        !SectionFits(header->StringOffset, header->StringTableSize, package->Size)) {
        return false;
    }

    const WdexeAssetRecord* assetRecords = (const WdexeAssetRecord*)(packageBytes + header->AssetOffset);
    for (uint32_t assetIndex = 0; assetIndex < header->AssetCount; assetIndex++) {
        const char* alias = ResolveString(packageBytes, header, assetRecords[assetIndex].AliasOffset, package->Size);
        CopyText(scene->AssetAliases[assetIndex], APP_ASSET_ALIAS_CAPACITY, alias);
    }

    const WdexeElementRecord* elementRecords = (const WdexeElementRecord*)(packageBytes + header->ElementOffset);
    for (uint32_t elementIndex = 0; elementIndex < header->ElementCount; elementIndex++) {
        const WdexeElementRecord* source = &elementRecords[elementIndex];
        AppElement* destination = &scene->Elements[elementIndex];

        destination->Id = source->Id;
        destination->ParentId = source->ParentId;
        destination->Type = source->Type;
        destination->Flags = source->Flags;
        destination->X = source->X;
        destination->Y = source->Y;
        destination->Width = source->Width;
        destination->Height = source->Height;
        destination->ZIndex = source->ZIndex;
        destination->BackgroundColor = source->BackgroundColor;
        destination->ForegroundColor = source->ForegroundColor;
        destination->BorderColor = source->BorderColor;
        destination->Value = source->Value;
        destination->MinValue = source->MinValue;
        destination->MaxValue = source->MaxValue;
        destination->AssetAddress = nullptr;

        const char* text = ResolveString(packageBytes, header, source->TextOffset, package->Size);
        CopyText(destination->Text, APP_TEXT_CAPACITY, text);

        destination->AssetAlias[0] = '\0';
        if (source->AssetIndex != INVALID_ASSET_INDEX && source->AssetIndex < header->AssetCount) {
            CopyText(destination->AssetAlias, APP_ASSET_ALIAS_CAPACITY, scene->AssetAliases[source->AssetIndex]);
            destination->AssetAddress = GetAsset(destination->AssetAlias, config);
        }
    }

    const WdexeEventRecord* eventRecords = (const WdexeEventRecord*)(packageBytes + header->EventOffset);
    for (uint32_t eventIndex = 0; eventIndex < header->EventCount; eventIndex++) {
        scene->Events[eventIndex].SourceElementId = eventRecords[eventIndex].SourceElementId;
        scene->Events[eventIndex].EventType = eventRecords[eventIndex].EventType;
        scene->Events[eventIndex].FirstBindingIndex = eventRecords[eventIndex].FirstBindingIndex;
        scene->Events[eventIndex].BindingCount = eventRecords[eventIndex].BindingCount;
    }

    const WdexeBindingRecord* bindingRecords = (const WdexeBindingRecord*)(packageBytes + header->BindingOffset);
    for (uint32_t bindingIndex = 0; bindingIndex < header->BindingCount; bindingIndex++) {
        scene->Bindings[bindingIndex].ActionType = bindingRecords[bindingIndex].ActionType;
        scene->Bindings[bindingIndex].TargetElementId = bindingRecords[bindingIndex].TargetElementId;
        scene->Bindings[bindingIndex].PropertyType = bindingRecords[bindingIndex].PropertyType;
        scene->Bindings[bindingIndex].IntValue = bindingRecords[bindingIndex].IntValue;
        scene->Bindings[bindingIndex].ColorValue = bindingRecords[bindingIndex].ColorValue;

        const char* stringValue = ResolveString(packageBytes, header, bindingRecords[bindingIndex].StringOffset, package->Size);
        CopyText(scene->Bindings[bindingIndex].StringValue, APP_TEXT_CAPACITY, stringValue);
    }

    scene->Loaded = true;
    scene->WindowOpen = true;
    scene->CanvasWidth = (int)header->CanvasWidth;
    scene->CanvasHeight = (int)header->CanvasHeight;
    scene->RootElementId = header->RootElementId;
    scene->ElementCount = (int)header->ElementCount;
    scene->EventCount = (int)header->EventCount;
    scene->BindingCount = (int)header->BindingCount;
    scene->AssetCount = (int)header->AssetCount;
    scene->PressedElementId = -1;
    scene->CapturedElementId = -1;
    scene->FocusedElementId = -1;
    scene->SurfaceDirty = true;

    if (scene->CanvasWidth <= 0 || scene->CanvasHeight <= 0) {
        const AppElement* root = FindAppElement(scene, scene->RootElementId);
        if (root != nullptr) {
            scene->CanvasWidth = root->Width;
            scene->CanvasHeight = root->Height;
        }
    }

    if (scene->CanvasWidth <= 0 || scene->CanvasHeight <= 0) {
        scene->Loaded = false;
        scene->WindowOpen = false;
        return false;
    }

    return true;
}

bool LoadBootAppScene(BOOT_CONFIG* config, AppScene* scene) {
    return LoadAppScene(config, "BOOTAPP", scene);
}
