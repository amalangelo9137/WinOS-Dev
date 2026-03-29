#pragma once

#include "Shared.h"

static const int APP_MAX_ELEMENTS = 64;
static const int APP_MAX_EVENTS = 64;
static const int APP_MAX_BINDINGS = 128;
static const int APP_MAX_ASSETS = 16;
static const int APP_TEXT_CAPACITY = 64;
static const int APP_ASSET_ALIAS_CAPACITY = 32;

enum AppElementType : uint32_t {
    AppElementTypeRootWindow = 0,
    AppElementTypePanel = 1,
    AppElementTypeImage = 2,
    AppElementTypeLabel = 3,
    AppElementTypeButton = 4,
    AppElementTypeSlider = 5,
    AppElementTypeTextField = 6,
};

enum AppElementFlags : uint32_t {
    AppElementFlagVisible = 1 << 0,
    AppElementFlagInteractive = 1 << 1,
    AppElementFlagDraggableRegion = 1 << 2,
    AppElementFlagClipChildren = 1 << 3,
    AppElementFlagWindowFrame = 1 << 4,
};

enum AppEventType : uint32_t {
    AppEventPointerDown = 0,
    AppEventPointerUp = 1,
    AppEventPointerMove = 2,
    AppEventClick = 3,
    AppEventValueChanged = 4,
    AppEventFocusChanged = 5,
};

enum AppActionType : uint32_t {
    AppActionNone = 0,
    AppActionSetProperty = 1,
    AppActionSetTextFromSourceValue = 2,
    AppActionCloseWindow = 3,
    AppActionBringToFront = 4,
    AppActionShutdown = 5,
    AppActionRestart = 6,
    AppActionLaunchApp = 7,
    AppActionPlayTone = 8,
    AppActionMinimizeWindow = 9,
    AppActionMaximizeWindow = 10,
    AppActionRestoreWindow = 11,
    AppActionSetTextFromSystemState = 12,
    AppActionLaunchNextApp = 13,
};

enum AppPropertyType : uint32_t {
    AppPropertyNone = 0,
    AppPropertyBackgroundColor = 1,
    AppPropertyForegroundColor = 2,
    AppPropertyBorderColor = 3,
    AppPropertyText = 4,
    AppPropertyValue = 5,
    AppPropertyVisible = 6,
};

struct AppBinding {
    uint32_t ActionType;
    uint32_t TargetElementId;
    uint32_t PropertyType;
    int32_t IntValue;
    uint32_t ColorValue;
    char StringValue[APP_TEXT_CAPACITY];
};

struct AppEvent {
    uint32_t SourceElementId;
    uint32_t EventType;
    uint32_t FirstBindingIndex;
    uint32_t BindingCount;
};

struct AppElement {
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
    char Text[APP_TEXT_CAPACITY];
    char AssetAlias[APP_ASSET_ALIAS_CAPACITY];
    void* AssetAddress;
};

struct AppScene {
    bool Loaded;
    bool WindowOpen;
    int CanvasWidth;
    int CanvasHeight;
    uint32_t RootElementId;
    int ElementCount;
    int EventCount;
    int BindingCount;
    int AssetCount;
    int PressedElementId;
    int CapturedElementId;
    int FocusedElementId;
    bool SurfaceDirty;
    AppElement Elements[APP_MAX_ELEMENTS];
    AppEvent Events[APP_MAX_EVENTS];
    AppBinding Bindings[APP_MAX_BINDINGS];
    char AssetAliases[APP_MAX_ASSETS][APP_ASSET_ALIAS_CAPACITY];
};

bool LoadAppScene(BOOT_CONFIG* config, const char* packageAlias, AppScene* scene);
bool LoadBootAppScene(BOOT_CONFIG* config, AppScene* scene);
AppElement* FindAppElement(AppScene* scene, uint32_t elementId);
const AppElement* FindAppElement(const AppScene* scene, uint32_t elementId);
