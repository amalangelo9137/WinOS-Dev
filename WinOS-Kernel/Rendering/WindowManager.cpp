#include "WindowManager.h"
#include "Renderer.h"

#include <intrin.h>

#include "../InitFS/Assets.h"

static const int MAX_DESKTOP_PIXELS = 1920 * 1080;
static const int MAX_WINDOW_PIXELS = 1920 * 1080;
static const int MAX_DIRTY_RECTS = 32;
static const int TASKBAR_HEIGHT = 36;
static const int DESKTOP_MARGIN = 12;
static const int CURSOR_FALLBACK_WIDTH = 12;
static const int CURSOR_FALLBACK_HEIGHT = 18;
static const int WINDOW_SHADOW_SIZE = 6;
static const int RESIZE_HANDLE_SIZE = 16;
static const int MIN_WINDOW_WIDTH = 240;
static const int MIN_WINDOW_HEIGHT = 160;
static const int TASKBAR_APP_BUTTON_X = 96;
static const int TASKBAR_APP_BUTTON_WIDTH = 180;
static const int TASKBAR_APP_BUTTON_HEIGHT = 24;
static const int MAX_APP_PACKAGES = 16;
static const int MAX_GIF_ANIMATIONS = 4;
static const int MAX_GIF_FRAMES = 64;
static const int MAX_GIF_CANVAS_PIXELS = 512 * 512;

static const uint32_t DESKTOP_COLOR = 0x0F1720;
static const uint32_t TASKBAR_COLOR = 0x111922;
static const uint32_t TASKBAR_EDGE_COLOR = 0x2F7D55;
static const uint32_t WINDOW_SHADOW_COLOR = 0x081019;
static const uint32_t WINDOW_TEXT_COLOR = 0xF4F8FC;
static const uint32_t WINDOW_TEXT_MUTED_COLOR = 0xB7C1CC;
static const uint32_t WINDOW_TEXT_DIM_COLOR = 0x92A0AD;

static BOOT_CONFIG* DesktopConfig = nullptr;
static AppScene BootAppScene = {};
static AppScene AppLoadBuffer = {};
static WindowSurface DesktopSurface = {};
static WindowSurface AppSurface = {};
static DirtyRect DirtyRects[MAX_DIRTY_RECTS] = {};
static int DirtyRectCount = 0;
static int WindowX = 0;
static int WindowY = 0;
static int WindowWidth = 0;
static int WindowHeight = 0;
static int RestoredWindowX = 0;
static int RestoredWindowY = 0;
static int RestoredWindowWidth = 0;
static int RestoredWindowHeight = 0;

static uint32_t DesktopSurfacePixels[MAX_DESKTOP_PIXELS] = {};
static uint32_t AppSurfacePixels[MAX_WINDOW_PIXELS] = {};

static void* DesktopBackground = nullptr;
static void* CursorAsset = nullptr;
static int CursorX = 0;
static int CursorY = 0;
static int PresentedCursorX = 0;
static int PresentedCursorY = 0;
static bool CursorPresented = false;
static bool CursorPresentNeeded = false;
static bool PreviousLeftDown = false;
static bool WindowDragging = false;
static bool WindowResizing = false;
static bool WindowMaximized = false;
static bool WindowMinimized = false;
static int DragOffsetX = 0;
static int DragOffsetY = 0;
static int ResizeOriginX = 0;
static int ResizeOriginY = 0;
static int ResizeOriginWidth = 0;
static int ResizeOriginHeight = 0;
static int AppPackageCount = 0;
static int CurrentAppIndex = -1;
static int LastRenderedSecond = -1;
static uint64_t TicksPerMillisecond = 3000000;
static int CursorBitmapWidth = 0;
static int CursorBitmapHeight = 0;
static int CursorStride = 0;
static uint32_t CursorPixelOffset = 0;
static uint16_t CursorBitsPerPixel = 0;
static bool CursorTopDown = false;
static char AppPackageAliases[MAX_APP_PACKAGES][APP_ASSET_ALIAS_CAPACITY] = {};
static char CurrentAppAlias[APP_ASSET_ALIAS_CAPACITY] = {};

struct GifFrameMeta {
    int X;
    int Y;
    int Width;
    int Height;
    uint16_t DelayCentiseconds;
    uint8_t DisposalMethod;
    bool Transparent;
    uint8_t TransparentIndex;
    bool Interlaced;
    uint8_t LzwMinimumCodeSize;
    uint32_t DataOffset;
    uint32_t LocalColorTableOffset;
    int LocalColorCount;
};

struct GifAnimationState {
    bool Active;
    bool Parsed;
    bool Animated;
    bool Valid;
    uint32_t ElementId;
    void* AssetAddress;
    int CanvasWidth;
    int CanvasHeight;
    int GlobalColorCount;
    uint32_t GlobalColorTableOffset;
    int FrameCount;
    int CurrentFrame;
    uint64_t NextFrameTicks;
    GifFrameMeta Frames[MAX_GIF_FRAMES];
    uint32_t CanvasPixels[MAX_GIF_CANVAS_PIXELS];
    uint8_t CanvasMask[MAX_GIF_CANVAS_PIXELS];
};

static GifAnimationState GifAnimations[MAX_GIF_ANIMATIONS] = {};

static const char* CursorMask[CURSOR_FALLBACK_HEIGHT] = {
    "X...........",
    "XX..........",
    "XOX.........",
    "XOOX........",
    "XOOOX.......",
    "XOOOOX......",
    "XOOOOOX.....",
    "XOOOOOOX....",
    "XOOOOOOOX...",
    "XOOOOOOOOX..",
    "XOOOOXXXXX..",
    "XOOXXOOX....",
    "XOX..XOOX...",
    "XX....XOOX..",
    "X......XOOX.",
    "........XOOX",
    ".........XX.",
    "............"
};

static void RenderDesktopSurface();
static void UpdateSystemStateBindings();
static bool OpenAppByAlias(const char* packageAlias);
static void CopyText(char* destination, int capacity, const char* source);
static bool TextEqualsIgnoreCase(const char* left, const char* right);
static void ResetGifAnimations();

static bool PointInRect(int x, int y, int rectX, int rectY, int rectWidth, int rectHeight) {
    return x >= rectX && x < (rectX + rectWidth) && y >= rectY && y < (rectY + rectHeight);
}

static int ClampInt(int value, int minimum, int maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static int WorkAreaHeight() {
    if (DesktopConfig == nullptr) return 0;
    return (int)DesktopConfig->Height - TASKBAR_HEIGHT - DESKTOP_MARGIN;
}

static int WindowDisplayWidth() {
    if (WindowWidth > 0) return WindowWidth;
    if (AppSurface.Valid) return AppSurface.Width;
    return BootAppScene.CanvasWidth;
}

static int WindowDisplayHeight() {
    if (WindowHeight > 0) return WindowHeight;
    if (AppSurface.Valid) return AppSurface.Height;
    return BootAppScene.CanvasHeight;
}

static const AppElement* RootWindowElement() {
    if (!BootAppScene.Loaded) {
        return nullptr;
    }

    return FindAppElement(&BootAppScene, BootAppScene.RootElementId);
}

static bool AppHasWindowFrame() {
    const AppElement* root = RootWindowElement();
    return root != nullptr && (root->Flags & AppElementFlagWindowFrame) != 0;
}

static bool AppWindowVisible() {
    return BootAppScene.Loaded && BootAppScene.WindowOpen && AppSurface.Valid && !WindowMinimized;
}

static int TaskbarButtonY() {
    if (DesktopConfig == nullptr) {
        return 0;
    }

    return (int)DesktopConfig->Height - TASKBAR_HEIGHT + ((TASKBAR_HEIGHT - TASKBAR_APP_BUTTON_HEIGHT) / 2);
}

static uint8_t ReadRtcRegister(uint8_t reg) {
    __outbyte(0x70, reg);
    return __inbyte(0x71);
}

static int BcdToInt(uint8_t value) {
    return ((value >> 4) * 10) + (value & 0x0F);
}

static bool ReadRtcTime(int* hour, int* minute, int* second) {
    if (hour == nullptr || minute == nullptr || second == nullptr) {
        return false;
    }

    while ((ReadRtcRegister(0x0A) & 0x80) != 0) {
    }

    uint8_t rawSecond = ReadRtcRegister(0x00);
    uint8_t rawMinute = ReadRtcRegister(0x02);
    uint8_t rawHour = ReadRtcRegister(0x04);
    uint8_t registerB = ReadRtcRegister(0x0B);

    bool binaryMode = (registerB & 0x04) != 0;
    bool twentyFourHour = (registerB & 0x02) != 0;

    int decodedSecond = binaryMode ? rawSecond : BcdToInt(rawSecond);
    int decodedMinute = binaryMode ? rawMinute : BcdToInt(rawMinute);
    int decodedHour = binaryMode ? (rawHour & 0x7F) : BcdToInt(rawHour & 0x7F);

    if (!twentyFourHour && (rawHour & 0x80) != 0) {
        decodedHour = (decodedHour + 12) % 24;
    }

    *hour = decodedHour;
    *minute = decodedMinute;
    *second = decodedSecond;
    return true;
}

static void CalibrateTsc() {
    int hour = 0;
    int minute = 0;
    int second = 0;
    if (!ReadRtcTime(&hour, &minute, &second)) {
        TicksPerMillisecond = 3000000;
        return;
    }

    int startSecond = second;
    while (ReadRtcTime(&hour, &minute, &second) && second == startSecond) {
    }

    uint64_t startTicks = __rdtsc();
    startSecond = second;
    while (ReadRtcTime(&hour, &minute, &second) && second == startSecond) {
    }

    uint64_t elapsedTicks = __rdtsc() - startTicks;
    if (elapsedTicks == 0) {
        TicksPerMillisecond = 3000000;
        return;
    }

    TicksPerMillisecond = elapsedTicks / 1000;
    if (TicksPerMillisecond == 0) {
        TicksPerMillisecond = 3000000;
    }
}

static void FormatTimeText(char* buffer, int capacity) {
    if (buffer == nullptr || capacity <= 0) {
        return;
    }

    int hour = 0;
    int minute = 0;
    int second = 0;
    if (!ReadRtcTime(&hour, &minute, &second)) {
        CopyText(buffer, capacity, "00:00:00 GMT");
        return;
    }

    buffer[0] = (char)('0' + ((hour / 10) % 10));
    buffer[1] = (char)('0' + (hour % 10));
    buffer[2] = ':';
    buffer[3] = (char)('0' + ((minute / 10) % 10));
    buffer[4] = (char)('0' + (minute % 10));
    buffer[5] = ':';
    buffer[6] = (char)('0' + ((second / 10) % 10));
    buffer[7] = (char)('0' + (second % 10));
    buffer[8] = ' ';
    buffer[9] = 'G';
    buffer[10] = 'M';
    buffer[11] = 'T';
    buffer[12] = '\0';
}

static bool AssetLooksLikeAppPackage(const ASSET_ENTRY* entry) {
    if (entry == nullptr || entry->Address == nullptr || entry->Size < 4) {
        return false;
    }

    const uint8_t* bytes = (const uint8_t*)entry->Address;
    return bytes[0] == 'W' && bytes[1] == 'D' && bytes[2] == 'X' && bytes[3] == 'E';
}

static void DiscoverAppPackages() {
    AppPackageCount = 0;
    CurrentAppIndex = -1;

    if (DesktopConfig == nullptr || DesktopConfig->Assets == nullptr) {
        return;
    }

    for (uint32_t assetIndex = 0; assetIndex < DesktopConfig->Assets->Count && AppPackageCount < MAX_APP_PACKAGES; assetIndex++) {
        const ASSET_ENTRY* entry = &DesktopConfig->Assets->Entries[assetIndex];
        if (!AssetLooksLikeAppPackage(entry)) {
            continue;
        }

        CopyText(AppPackageAliases[AppPackageCount], APP_ASSET_ALIAS_CAPACITY, entry->Name);
        AppPackageCount++;
    }
}

static void SetCurrentAppAlias(const char* alias) {
    CopyText(CurrentAppAlias, APP_ASSET_ALIAS_CAPACITY, alias);
    CurrentAppIndex = -1;

    for (int index = 0; index < AppPackageCount; index++) {
        if (TextEqualsIgnoreCase(AppPackageAliases[index], CurrentAppAlias)) {
            CurrentAppIndex = index;
            break;
        }
    }
}

static int SceneXFromWindowX(int screenX) {
    int displayWidth = WindowDisplayWidth();
    if (displayWidth <= 0 || AppSurface.Width <= 0) {
        return 0;
    }

    int localX = screenX - WindowX;
    localX = ClampInt(localX, 0, displayWidth - 1);
    return ClampInt((localX * AppSurface.Width) / displayWidth, 0, AppSurface.Width - 1);
}

static int SceneYFromWindowY(int screenY) {
    int displayHeight = WindowDisplayHeight();
    if (displayHeight <= 0 || AppSurface.Height <= 0) {
        return 0;
    }

    int localY = screenY - WindowY;
    localY = ClampInt(localY, 0, displayHeight - 1);
    return ClampInt((localY * AppSurface.Height) / displayHeight, 0, AppSurface.Height - 1);
}

static void CopyText(char* destination, int capacity, const char* source) {
    if (capacity <= 0) return;

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

static char ToUpperAscii(char value) {
    if (value >= 'a' && value <= 'z') {
        return (char)(value - ('a' - 'A'));
    }

    return value;
}

static bool TextEqualsIgnoreCase(const char* left, const char* right) {
    if (left == nullptr || right == nullptr) {
        return false;
    }

    int index = 0;
    while (left[index] != '\0' || right[index] != '\0') {
        if (ToUpperAscii(left[index]) != ToUpperAscii(right[index])) {
            return false;
        }

        index++;
    }

    return true;
}

static void IntToText(int value, char* buffer, int capacity) {
    if (capacity <= 0) {
        return;
    }

    if (value == 0) {
        if (capacity > 1) {
            buffer[0] = '0';
            buffer[1] = '\0';
        }
        else {
            buffer[0] = '\0';
        }
        return;
    }

    bool negative = value < 0;
    unsigned int working = negative ? (unsigned int)(-value) : (unsigned int)value;
    char digits[16];
    int digitCount = 0;

    while (working > 0 && digitCount < 16) {
        digits[digitCount++] = (char)('0' + (working % 10));
        working /= 10;
    }

    int position = 0;
    if (negative && position < capacity - 1) {
        buffer[position++] = '-';
    }

    while (digitCount > 0 && position < capacity - 1) {
        buffer[position++] = digits[--digitCount];
    }

    buffer[position] = '\0';
}

static void AppendText(char* destination, int capacity, const char* source) {
    if (destination == nullptr || source == nullptr || capacity <= 0) {
        return;
    }

    int index = 0;
    while (index < capacity - 1 && destination[index] != '\0') {
        index++;
    }

    int sourceIndex = 0;
    while (index < capacity - 1 && source[sourceIndex] != '\0') {
        destination[index++] = source[sourceIndex++];
    }

    destination[index] = '\0';
}

static uint16_t ReadUint16(const uint8_t* bytes) {
    return (uint16_t)(bytes[0] | (bytes[1] << 8));
}

static void ResetGifAnimations() {
    for (int animationIndex = 0; animationIndex < MAX_GIF_ANIMATIONS; animationIndex++) {
        GifAnimations[animationIndex].Active = false;
        GifAnimations[animationIndex].Parsed = false;
        GifAnimations[animationIndex].Animated = false;
        GifAnimations[animationIndex].Valid = false;
        GifAnimations[animationIndex].ElementId = 0;
        GifAnimations[animationIndex].AssetAddress = nullptr;
        GifAnimations[animationIndex].CanvasWidth = 0;
        GifAnimations[animationIndex].CanvasHeight = 0;
        GifAnimations[animationIndex].GlobalColorCount = 0;
        GifAnimations[animationIndex].GlobalColorTableOffset = 0;
        GifAnimations[animationIndex].FrameCount = 0;
        GifAnimations[animationIndex].CurrentFrame = 0;
        GifAnimations[animationIndex].NextFrameTicks = 0;

        for (int pixelIndex = 0; pixelIndex < MAX_GIF_CANVAS_PIXELS; pixelIndex++) {
            GifAnimations[animationIndex].CanvasPixels[pixelIndex] = 0;
            GifAnimations[animationIndex].CanvasMask[pixelIndex] = 0;
        }
    }
}

static uint32_t AdjustColor(uint32_t color, int delta) {
    int red = (int)((color >> 16) & 0xFF) + delta;
    int green = (int)((color >> 8) & 0xFF) + delta;
    int blue = (int)(color & 0xFF) + delta;

    red = ClampInt(red, 0, 255);
    green = ClampInt(green, 0, 255);
    blue = ClampInt(blue, 0, 255);

    return ((uint32_t)red << 16) | ((uint32_t)green << 8) | (uint32_t)blue;
}

static void InitializeSurface(WindowSurface* surface, uint32_t* pixels, int width, int height, int pixelCapacity) {
    surface->Pixels = pixels;
    surface->Width = width;
    surface->Height = height;
    surface->Stride = width;
    surface->Valid = width > 0 && height > 0 && ((width * height) <= pixelCapacity);
}

static WindowSurface CreateBackBufferSurface() {
    WindowSurface surface = {};
    if (DesktopConfig == nullptr || DesktopConfig->BackBuffer == nullptr) {
        return surface;
    }

    surface.Pixels = DesktopConfig->BackBuffer;
    surface.Width = (int)DesktopConfig->Width;
    surface.Height = (int)DesktopConfig->Height;
    surface.Stride = (int)DesktopConfig->PixelsPerScanLine;
    surface.Valid = surface.Width > 0 && surface.Height > 0;
    return surface;
}

static void SurfaceFill(WindowSurface* surface, uint32_t color) {
    if (surface == nullptr || !surface->Valid) return;

    for (int y = 0; y < surface->Height; y++) {
        for (int x = 0; x < surface->Width; x++) {
            surface->Pixels[x + (y * surface->Stride)] = color;
        }
    }
}

static void SurfaceFillRect(WindowSurface* surface, int x, int y, int width, int height, uint32_t color) {
    if (surface == nullptr || !surface->Valid || width <= 0 || height <= 0) return;

    int startX = ClampInt(x, 0, surface->Width);
    int startY = ClampInt(y, 0, surface->Height);
    int endX = ClampInt(x + width, 0, surface->Width);
    int endY = ClampInt(y + height, 0, surface->Height);

    for (int drawY = startY; drawY < endY; drawY++) {
        for (int drawX = startX; drawX < endX; drawX++) {
            surface->Pixels[drawX + (drawY * surface->Stride)] = color;
        }
    }
}

static void SurfaceDrawBorder(WindowSurface* surface, int x, int y, int width, int height, uint32_t color) {
    SurfaceFillRect(surface, x, y, width, 1, color);
    SurfaceFillRect(surface, x, y + height - 1, width, 1, color);
    SurfaceFillRect(surface, x, y, 1, height, color);
    SurfaceFillRect(surface, x + width - 1, y, 1, height, color);
}

static void SurfacePutChar(WindowSurface* surface, char c, int xOffset, int yOffset, uint32_t color) {
    if (surface == nullptr || !surface->Valid || DesktopConfig == nullptr || DesktopConfig->Font == nullptr) {
        return;
    }

    PSF1_HEADER* font = DesktopConfig->Font;
    unsigned char* glyphPtr = (unsigned char*)font + sizeof(PSF1_HEADER) + ((unsigned char)c * font->CharSize);

    for (unsigned long y = 0; y < font->CharSize; y++) {
        for (unsigned long x = 0; x < 8; x++) {
            if ((glyphPtr[y] & (0b10000000 >> x)) == 0) {
                continue;
            }

            int drawX = xOffset + (int)x;
            int drawY = yOffset + (int)y;
            if (drawX < 0 || drawX >= surface->Width || drawY < 0 || drawY >= surface->Height) {
                continue;
            }

            surface->Pixels[drawX + (drawY * surface->Stride)] = color;
        }
    }
}

static void SurfacePrint(WindowSurface* surface, const char* text, int x, int y, uint32_t color) {
    if (text == nullptr) return;

    int cursorX = x;
    while (*text != '\0') {
        SurfacePutChar(surface, *text, cursorX, y, color);
        cursorX += 8;
        text++;
    }
}

static void SurfaceDrawBMP(WindowSurface* surface, void* assetAddress, int targetX, int targetY, int targetWidth, int targetHeight) {
    if (surface == nullptr || !surface->Valid || assetAddress == nullptr || targetWidth <= 0 || targetHeight <= 0) {
        return;
    }

    uint8_t* bytes = (uint8_t*)assetAddress;
    if (bytes[0] != 'B' || bytes[1] != 'M') {
        return;
    }

    uint32_t pixelOffset = *(uint32_t*)(bytes + 10);
    int32_t sourceWidth = *(int32_t*)(bytes + 18);
    int32_t sourceHeight = *(int32_t*)(bytes + 22);
    uint16_t bitsPerPixel = *(uint16_t*)(bytes + 28);

    if ((bitsPerPixel != 24 && bitsPerPixel != 32) || sourceWidth == 0 || sourceHeight == 0) {
        return;
    }

    bool topDown = sourceHeight < 0;
    if (sourceWidth < 0) sourceWidth = -sourceWidth;
    if (sourceHeight < 0) sourceHeight = -sourceHeight;

    int sourceStride = ((sourceWidth * (bitsPerPixel / 8)) + 3) & ~3;
    uint8_t* pixelData = bytes + pixelOffset;

    for (int y = 0; y < targetHeight; y++) {
        int sourceY = (y * sourceHeight) / targetHeight;
        int bitmapY = topDown ? sourceY : (sourceHeight - 1 - sourceY);

        for (int x = 0; x < targetWidth; x++) {
            int sourceX = (x * sourceWidth) / targetWidth;
            int byteIndex = (sourceX * (bitsPerPixel / 8)) + (bitmapY * sourceStride);

            uint32_t color = 0;
            if (bitsPerPixel == 32) {
                color = *(uint32_t*)(pixelData + byteIndex);
                if ((color >> 24) == 0) {
                    continue;
                }
                color &= 0x00FFFFFF;
            }
            else {
                uint8_t blue = pixelData[byteIndex];
                uint8_t green = pixelData[byteIndex + 1];
                uint8_t red = pixelData[byteIndex + 2];
                color = ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
                if (color == 0x00FF00FF) {
                    continue;
                }
            }

            int drawX = targetX + x;
            int drawY = targetY + y;
            if (drawX < 0 || drawX >= surface->Width || drawY < 0 || drawY >= surface->Height) {
                continue;
            }

            surface->Pixels[drawX + (drawY * surface->Stride)] = color;
        }
    }
}

static bool AssetLooksLikeGif(void* assetAddress) {
    if (assetAddress == nullptr) {
        return false;
    }

    uint8_t* bytes = (uint8_t*)assetAddress;
    return bytes[0] == 'G' && bytes[1] == 'I' && bytes[2] == 'F';
}

static int GetGifColorCount(uint8_t packedField) {
    return 1 << ((packedField & 0x07) + 1);
}

static int GetGifFrameDelayMs(const GifFrameMeta* frame) {
    if (frame == nullptr) {
        return 100;
    }

    int delay = (int)frame->DelayCentiseconds * 10;
    return delay > 0 ? delay : 100;
}

static int MapInterlacedGifRow(int row, int height) {
    static const int passStart[4] = { 0, 4, 2, 1 };
    static const int passStep[4] = { 8, 8, 4, 2 };

    int index = 0;
    for (int pass = 0; pass < 4; pass++) {
        for (int mappedRow = passStart[pass]; mappedRow < height; mappedRow += passStep[pass]) {
            if (index == row) {
                return mappedRow;
            }

            index++;
        }
    }

    return row;
}

static uint32_t ReadGifColor(const uint8_t* bytes, uint32_t assetSize, uint32_t tableOffset, int colorCount, int colorIndex, bool* valid) {
    if (valid != nullptr) {
        *valid = false;
    }

    if (bytes == nullptr || colorIndex < 0 || colorIndex >= colorCount) {
        return 0;
    }

    uint32_t colorOffset = tableOffset + ((uint32_t)colorIndex * 3);
    if ((colorOffset + 2) >= assetSize) {
        return 0;
    }

    if (valid != nullptr) {
        *valid = true;
    }

    uint8_t red = bytes[colorOffset];
    uint8_t green = bytes[colorOffset + 1];
    uint8_t blue = bytes[colorOffset + 2];
    return ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
}

struct GifSubBlockReader {
    const uint8_t* Bytes;
    uint32_t AssetSize;
    uint32_t Offset;
    int RemainingInBlock;
    bool Finished;
};

static void InitGifSubBlockReader(GifSubBlockReader* reader, const uint8_t* bytes, uint32_t assetSize, uint32_t offset) {
    reader->Bytes = bytes;
    reader->AssetSize = assetSize;
    reader->Offset = offset;
    reader->RemainingInBlock = 0;
    reader->Finished = false;
}

static int ReadGifSubBlockByte(GifSubBlockReader* reader) {
    if (reader == nullptr || reader->Finished) {
        return -1;
    }

    while (reader->RemainingInBlock == 0) {
        if (reader->Offset >= reader->AssetSize) {
            reader->Finished = true;
            return -1;
        }

        reader->RemainingInBlock = reader->Bytes[reader->Offset++];
        if (reader->RemainingInBlock == 0) {
            reader->Finished = true;
            return -1;
        }

        if (reader->Offset + (uint32_t)reader->RemainingInBlock > reader->AssetSize) {
            reader->Finished = true;
            return -1;
        }
    }

    reader->RemainingInBlock--;
    return reader->Bytes[reader->Offset++];
}

static int SkipGifSubBlocks(const uint8_t* bytes, uint32_t assetSize, uint32_t offset) {
    while (offset < assetSize) {
        uint8_t blockSize = bytes[offset++];
        if (blockSize == 0) {
            return (int)offset;
        }

        if (offset + blockSize > assetSize) {
            return -1;
        }

        offset += blockSize;
    }

    return -1;
}

static bool DecodeGifFrameIndices(const uint8_t* bytes, uint32_t assetSize, const GifFrameMeta* frame, uint8_t* output, int outputCapacity) {
    if (bytes == nullptr || frame == nullptr || output == nullptr) {
        return false;
    }

    int pixelCount = frame->Width * frame->Height;
    if (pixelCount <= 0 || pixelCount > outputCapacity) {
        return false;
    }

    GifSubBlockReader reader;
    InitGifSubBlockReader(&reader, bytes, assetSize, frame->DataOffset);

    uint16_t prefix[4096];
    uint8_t suffix[4096];
    uint8_t stack[4097];

    int clearCode = 1 << frame->LzwMinimumCodeSize;
    int endCode = clearCode + 1;
    int available = clearCode + 2;
    int oldCode = -1;
    int codeSize = frame->LzwMinimumCodeSize + 1;
    int codeMask = (1 << codeSize) - 1;
    int bits = 0;
    int datum = 0;
    int first = 0;
    int top = 0;
    int outputIndex = 0;

    for (int code = 0; code < clearCode; code++) {
        prefix[code] = 0;
        suffix[code] = (uint8_t)code;
    }

    while (outputIndex < pixelCount) {
        if (top == 0) {
            while (bits < codeSize) {
                int nextByte = ReadGifSubBlockByte(&reader);
                if (nextByte < 0) {
                    return false;
                }

                datum |= nextByte << bits;
                bits += 8;
            }

            int code = datum & codeMask;
            datum >>= codeSize;
            bits -= codeSize;

            if (code == clearCode) {
                codeSize = frame->LzwMinimumCodeSize + 1;
                codeMask = (1 << codeSize) - 1;
                available = clearCode + 2;
                oldCode = -1;
                continue;
            }

            if (code == endCode) {
                break;
            }

            if (oldCode == -1) {
                output[outputIndex++] = suffix[code];
                oldCode = code;
                first = code;
                continue;
            }

            int inCode = code;
            if (code >= available) {
                stack[top++] = (uint8_t)first;
                code = oldCode;
            }

            while (code >= clearCode) {
                stack[top++] = suffix[code];
                code = prefix[code];
            }

            first = suffix[code];
            stack[top++] = (uint8_t)first;

            if (available < 4096) {
                prefix[available] = (uint16_t)oldCode;
                suffix[available] = (uint8_t)first;
                available++;

                if ((available & codeMask) == 0 && available < 4096) {
                    codeSize++;
                    codeMask = (1 << codeSize) - 1;
                }
            }

            oldCode = inCode;
        }

        top--;
        output[outputIndex++] = stack[top];
    }

    return outputIndex == pixelCount;
}

static bool ParseGifAnimationState(GifAnimationState* state, const AppElement* element) {
    if (state == nullptr || element == nullptr || DesktopConfig == nullptr) {
        return false;
    }

    ASSET_ENTRY* entry = GetAssetEntry(element->AssetAlias, DesktopConfig);
    if (entry == nullptr || entry->Address == nullptr || entry->Size < 13) {
        return false;
    }

    const uint8_t* bytes = (const uint8_t*)entry->Address;
    if (!AssetLooksLikeGif(entry->Address)) {
        return false;
    }

    uint16_t canvasWidth = ReadUint16(bytes + 6);
    uint16_t canvasHeight = ReadUint16(bytes + 8);
    if (canvasWidth == 0 || canvasHeight == 0) {
        return false;
    }

    if (((int)canvasWidth * (int)canvasHeight) > MAX_GIF_CANVAS_PIXELS) {
        return false;
    }

    state->Active = true;
    state->Parsed = true;
    state->Valid = false;
    state->Animated = false;
    state->ElementId = element->Id;
    state->AssetAddress = entry->Address;
    state->CanvasWidth = canvasWidth;
    state->CanvasHeight = canvasHeight;
    state->FrameCount = 0;
    state->CurrentFrame = 0;
    state->GlobalColorCount = 0;
    state->GlobalColorTableOffset = 0;

    uint8_t packedField = bytes[10];
    uint32_t offset = 13;
    if ((packedField & 0x80) != 0) {
        state->GlobalColorCount = GetGifColorCount(packedField);
        state->GlobalColorTableOffset = offset;
        offset += (uint32_t)state->GlobalColorCount * 3;
        if (offset > entry->Size) {
            return false;
        }
    }

    uint16_t pendingDelay = 10;
    uint8_t pendingDisposal = 0;
    bool pendingTransparent = false;
    uint8_t pendingTransparentIndex = 0;

    while (offset < entry->Size && state->FrameCount < MAX_GIF_FRAMES) {
        uint8_t introducer = bytes[offset++];
        if (introducer == 0x3B) {
            break;
        }

        if (introducer == 0x21) {
            if (offset >= entry->Size) {
                break;
            }

            uint8_t label = bytes[offset++];
            if (label == 0xF9 && offset + 5 <= entry->Size) {
                uint8_t blockSize = bytes[offset++];
                if (blockSize == 4 && offset + 4 <= entry->Size) {
                    uint8_t gcePacked = bytes[offset++];
                    pendingDisposal = (gcePacked >> 2) & 0x07;
                    pendingTransparent = (gcePacked & 0x01) != 0;
                    pendingDelay = ReadUint16(bytes + offset);
                    offset += 2;
                    pendingTransparentIndex = bytes[offset++];
                    offset++;
                }
                else {
                    offset += blockSize;
                    offset++;
                }
            }
            else {
                int skippedOffset = SkipGifSubBlocks(bytes, entry->Size, offset);
                if (skippedOffset < 0) {
                    return false;
                }

                offset = (uint32_t)skippedOffset;
            }

            continue;
        }

        if (introducer != 0x2C || offset + 9 > entry->Size) {
            break;
        }

        GifFrameMeta* frame = &state->Frames[state->FrameCount++];
        frame->X = ReadUint16(bytes + offset);
        frame->Y = ReadUint16(bytes + offset + 2);
        frame->Width = ReadUint16(bytes + offset + 4);
        frame->Height = ReadUint16(bytes + offset + 6);
        uint8_t descriptorPacked = bytes[offset + 8];
        offset += 9;

        frame->DelayCentiseconds = pendingDelay;
        frame->DisposalMethod = pendingDisposal;
        frame->Transparent = pendingTransparent;
        frame->TransparentIndex = pendingTransparentIndex;
        frame->Interlaced = (descriptorPacked & 0x40) != 0;
        frame->LocalColorCount = 0;
        frame->LocalColorTableOffset = 0;

        if ((descriptorPacked & 0x80) != 0) {
            frame->LocalColorCount = GetGifColorCount(descriptorPacked);
            frame->LocalColorTableOffset = offset;
            offset += (uint32_t)frame->LocalColorCount * 3;
            if (offset > entry->Size) {
                return false;
            }
        }

        if (offset >= entry->Size) {
            return false;
        }

        frame->LzwMinimumCodeSize = bytes[offset++];
        frame->DataOffset = offset;
        int nextOffset = SkipGifSubBlocks(bytes, entry->Size, offset);
        if (nextOffset < 0) {
            return false;
        }

        offset = (uint32_t)nextOffset;
        pendingDelay = 10;
        pendingDisposal = 0;
        pendingTransparent = false;
        pendingTransparentIndex = 0;
    }

    state->Animated = state->FrameCount > 1;
    state->Valid = state->FrameCount > 0;
    return state->Valid;
}

static void ClearGifRect(GifAnimationState* state, int x, int y, int width, int height) {
    if (state == nullptr) {
        return;
    }

    int startX = ClampInt(x, 0, state->CanvasWidth);
    int startY = ClampInt(y, 0, state->CanvasHeight);
    int endX = ClampInt(x + width, 0, state->CanvasWidth);
    int endY = ClampInt(y + height, 0, state->CanvasHeight);

    for (int drawY = startY; drawY < endY; drawY++) {
        for (int drawX = startX; drawX < endX; drawX++) {
            int pixelIndex = drawX + (drawY * state->CanvasWidth);
            state->CanvasMask[pixelIndex] = 0;
            state->CanvasPixels[pixelIndex] = 0;
        }
    }
}

static bool ApplyGifFrameToState(GifAnimationState* state, const ASSET_ENTRY* entry, const GifFrameMeta* frame) {
    if (state == nullptr || entry == nullptr || frame == nullptr) {
        return false;
    }

    int pixelCount = frame->Width * frame->Height;
    if (pixelCount <= 0 || pixelCount > MAX_GIF_CANVAS_PIXELS) {
        return false;
    }

    static uint8_t decodedIndices[MAX_GIF_CANVAS_PIXELS] = {};
    if (!DecodeGifFrameIndices((const uint8_t*)entry->Address, entry->Size, frame, decodedIndices, MAX_GIF_CANVAS_PIXELS)) {
        return false;
    }

    for (int sourceRow = 0; sourceRow < frame->Height; sourceRow++) {
        int mappedRow = frame->Interlaced ? MapInterlacedGifRow(sourceRow, frame->Height) : sourceRow;
        int targetY = frame->Y + mappedRow;
        if (targetY < 0 || targetY >= state->CanvasHeight) {
            continue;
        }

        for (int sourceX = 0; sourceX < frame->Width; sourceX++) {
            int targetX = frame->X + sourceX;
            if (targetX < 0 || targetX >= state->CanvasWidth) {
                continue;
            }

            uint8_t colorIndex = decodedIndices[sourceX + (sourceRow * frame->Width)];
            if (frame->Transparent && colorIndex == frame->TransparentIndex) {
                continue;
            }

            bool colorValid = false;
            uint32_t color = 0;
            const uint8_t* bytes = (const uint8_t*)entry->Address;
            if (frame->LocalColorCount > 0) {
                color = ReadGifColor(bytes, entry->Size, frame->LocalColorTableOffset, frame->LocalColorCount, colorIndex, &colorValid);
            }
            else {
                color = ReadGifColor(bytes, entry->Size, state->GlobalColorTableOffset, state->GlobalColorCount, colorIndex, &colorValid);
            }

            if (!colorValid) {
                continue;
            }

            int targetIndex = targetX + (targetY * state->CanvasWidth);
            state->CanvasPixels[targetIndex] = color;
            state->CanvasMask[targetIndex] = 1;
        }
    }

    return true;
}

static bool RebuildGifAnimationState(GifAnimationState* state, const ASSET_ENTRY* entry) {
    if (state == nullptr || entry == nullptr || !state->Valid) {
        return false;
    }

    for (int pixelIndex = 0; pixelIndex < state->CanvasWidth * state->CanvasHeight; pixelIndex++) {
        state->CanvasPixels[pixelIndex] = 0;
        state->CanvasMask[pixelIndex] = 0;
    }

    uint8_t previousDisposal = 0;
    int previousX = 0;
    int previousY = 0;
    int previousWidth = 0;
    int previousHeight = 0;

    for (int frameIndex = 0; frameIndex <= state->CurrentFrame && frameIndex < state->FrameCount; frameIndex++) {
        if (frameIndex > 0 && previousDisposal == 2) {
            ClearGifRect(state, previousX, previousY, previousWidth, previousHeight);
        }

        const GifFrameMeta* frame = &state->Frames[frameIndex];
        if (!ApplyGifFrameToState(state, entry, frame)) {
            return false;
        }

        previousDisposal = frame->DisposalMethod;
        previousX = frame->X;
        previousY = frame->Y;
        previousWidth = frame->Width;
        previousHeight = frame->Height;
    }

    return true;
}

static GifAnimationState* GetGifAnimationState(const AppElement* element) {
    if (element == nullptr || element->AssetAddress == nullptr || element->AssetAlias[0] == '\0') {
        return nullptr;
    }

    if (!AssetLooksLikeGif(element->AssetAddress)) {
        return nullptr;
    }

    for (int animationIndex = 0; animationIndex < MAX_GIF_ANIMATIONS; animationIndex++) {
        GifAnimationState* state = &GifAnimations[animationIndex];
        if (state->Active && state->ElementId == element->Id && state->AssetAddress == element->AssetAddress) {
            return state->Valid ? state : nullptr;
        }
    }

    GifAnimationState* freeState = nullptr;
    for (int animationIndex = 0; animationIndex < MAX_GIF_ANIMATIONS; animationIndex++) {
        if (!GifAnimations[animationIndex].Active) {
            freeState = &GifAnimations[animationIndex];
            break;
        }
    }

    if (freeState == nullptr) {
        return nullptr;
    }

    ASSET_ENTRY* entry = GetAssetEntry(element->AssetAlias, DesktopConfig);
    if (entry == nullptr) {
        return nullptr;
    }

    if (!ParseGifAnimationState(freeState, element)) {
        return nullptr;
    }

    freeState->CurrentFrame = 0;
    if (!RebuildGifAnimationState(freeState, entry)) {
        freeState->Active = false;
        return nullptr;
    }

    freeState->NextFrameTicks = __rdtsc() + ((uint64_t)GetGifFrameDelayMs(&freeState->Frames[0]) * TicksPerMillisecond);
    return freeState;
}

static void SurfaceDrawGif(WindowSurface* surface, const AppElement* element, int targetX, int targetY, int targetWidth, int targetHeight) {
    if (surface == nullptr || !surface->Valid || element == nullptr || targetWidth <= 0 || targetHeight <= 0) {
        return;
    }

    GifAnimationState* state = GetGifAnimationState(element);
    if (state == nullptr || !state->Valid || state->CanvasWidth <= 0 || state->CanvasHeight <= 0) {
        return;
    }

    for (int y = 0; y < targetHeight; y++) {
        int sourceY = (y * state->CanvasHeight) / targetHeight;
        for (int x = 0; x < targetWidth; x++) {
            int sourceX = (x * state->CanvasWidth) / targetWidth;
            int sourceIndex = sourceX + (sourceY * state->CanvasWidth);
            if (state->CanvasMask[sourceIndex] == 0) {
                continue;
            }

            int drawX = targetX + x;
            int drawY = targetY + y;
            if (drawX < 0 || drawX >= surface->Width || drawY < 0 || drawY >= surface->Height) {
                continue;
            }

            surface->Pixels[drawX + (drawY * surface->Stride)] = state->CanvasPixels[sourceIndex];
        }
    }
}

static int CurrentCursorWidth() {
    return CursorAsset != nullptr ? CursorBitmapWidth : CURSOR_FALLBACK_WIDTH;
}

static int CurrentCursorHeight() {
    return CursorAsset != nullptr ? CursorBitmapHeight : CURSOR_FALLBACK_HEIGHT;
}

static void LoadCursorMetadata() {
    CursorAsset = nullptr;
    CursorBitmapWidth = 0;
    CursorBitmapHeight = 0;
    CursorStride = 0;
    CursorPixelOffset = 0;
    CursorBitsPerPixel = 0;
    CursorTopDown = false;

    if (DesktopConfig == nullptr) {
        return;
    }

    CursorAsset = GetAsset("CURSOR", DesktopConfig);
    if (CursorAsset == nullptr) {
        return;
    }

    uint8_t* bytes = (uint8_t*)CursorAsset;
    if (bytes[0] != 'B' || bytes[1] != 'M') {
        CursorAsset = nullptr;
        return;
    }

    CursorPixelOffset = *(uint32_t*)(bytes + 10);
    int32_t width = *(int32_t*)(bytes + 18);
    int32_t height = *(int32_t*)(bytes + 22);
    CursorBitsPerPixel = *(uint16_t*)(bytes + 28);

    if (width == 0 || height == 0 || (CursorBitsPerPixel != 24 && CursorBitsPerPixel != 32)) {
        CursorAsset = nullptr;
        return;
    }

    if (width < 0) width = -width;
    CursorTopDown = height < 0;
    if (height < 0) height = -height;

    CursorBitmapWidth = width;
    CursorBitmapHeight = height;
    CursorStride = ((CursorBitmapWidth * (CursorBitsPerPixel / 8)) + 3) & ~3;
}

static void RestoreCursorArea(int cursorX, int cursorY) {
    if (DesktopConfig == nullptr) return;

    int cursorWidth = CurrentCursorWidth();
    int cursorHeight = CurrentCursorHeight();

    for (int y = 0; y < cursorHeight; y++) {
        for (int x = 0; x < cursorWidth; x++) {
            int screenX = cursorX + x;
            int screenY = cursorY + y;
            if (screenX < 0 || screenX >= (int)DesktopConfig->Width || screenY < 0 || screenY >= (int)DesktopConfig->Height) {
                continue;
            }

            DesktopConfig->BaseAddress[screenX + (screenY * DesktopConfig->PixelsPerScanLine)] =
                DesktopConfig->BackBuffer[screenX + (screenY * DesktopConfig->PixelsPerScanLine)];
        }
    }
}

static void DrawFallbackCursorToFrontBuffer(int cursorX, int cursorY) {
    for (int y = 0; y < CURSOR_FALLBACK_HEIGHT; y++) {
        for (int x = 0; x < CURSOR_FALLBACK_WIDTH; x++) {
            char pixel = CursorMask[y][x];
            if (pixel == '.') {
                continue;
            }

            int screenX = cursorX + x;
            int screenY = cursorY + y;
            if (screenX < 0 || screenX >= (int)DesktopConfig->Width || screenY < 0 || screenY >= (int)DesktopConfig->Height) {
                continue;
            }

            DesktopConfig->BaseAddress[screenX + (screenY * DesktopConfig->PixelsPerScanLine)] = pixel == 'X' ? 0x000000 : 0xFFFFFF;
        }
    }
}

static void DrawBitmapCursorToFrontBuffer(int cursorX, int cursorY) {
    uint8_t* bytes = (uint8_t*)CursorAsset;
    uint8_t* pixelData = bytes + CursorPixelOffset;

    for (int y = 0; y < CursorBitmapHeight; y++) {
        int sourceY = CursorTopDown ? y : (CursorBitmapHeight - 1 - y);
        for (int x = 0; x < CursorBitmapWidth; x++) {
            int byteIndex = (x * (CursorBitsPerPixel / 8)) + (sourceY * CursorStride);
            uint32_t color = 0;

            if (CursorBitsPerPixel == 32) {
                color = *(uint32_t*)(pixelData + byteIndex);
                if ((color >> 24) == 0) {
                    continue;
                }
                color &= 0x00FFFFFF;
            }
            else {
                uint8_t blue = pixelData[byteIndex];
                uint8_t green = pixelData[byteIndex + 1];
                uint8_t red = pixelData[byteIndex + 2];
                color = ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
                if (color == 0x00FF00FF) {
                    continue;
                }
            }

            int screenX = cursorX + x;
            int screenY = cursorY + y;
            if (screenX < 0 || screenX >= (int)DesktopConfig->Width || screenY < 0 || screenY >= (int)DesktopConfig->Height) {
                continue;
            }

            DesktopConfig->BaseAddress[screenX + (screenY * DesktopConfig->PixelsPerScanLine)] = color;
        }
    }
}

static void DrawCursorToFrontBuffer(int cursorX, int cursorY) {
    if (DesktopConfig == nullptr) {
        return;
    }

    if (CursorAsset != nullptr) {
        DrawBitmapCursorToFrontBuffer(cursorX, cursorY);
    }
    else {
        DrawFallbackCursorToFrontBuffer(cursorX, cursorY);
    }
}

static void PresentCursorOnly() {
    if (DesktopConfig == nullptr) {
        return;
    }

    if (CursorPresented) {
        RestoreCursorArea(PresentedCursorX, PresentedCursorY);
    }

    DrawCursorToFrontBuffer(CursorX, CursorY);
    PresentedCursorX = CursorX;
    PresentedCursorY = CursorY;
    CursorPresented = true;
    CursorPresentNeeded = false;
}

static bool RectsTouchOrOverlap(const DirtyRect& left, const DirtyRect& right) {
    return !(left.X + left.Width < right.X ||
        right.X + right.Width < left.X ||
        left.Y + left.Height < right.Y ||
        right.Y + right.Height < left.Y);
}

static DirtyRect UnionRects(const DirtyRect& left, const DirtyRect& right) {
    DirtyRect result;
    int rightEdge = (left.X + left.Width) > (right.X + right.Width) ? (left.X + left.Width) : (right.X + right.Width);
    int bottomEdge = (left.Y + left.Height) > (right.Y + right.Height) ? (left.Y + left.Height) : (right.Y + right.Height);
    result.X = left.X < right.X ? left.X : right.X;
    result.Y = left.Y < right.Y ? left.Y : right.Y;
    result.Width = rightEdge - result.X;
    result.Height = bottomEdge - result.Y;
    return result;
}

static void AddDirtyRect(int x, int y, int width, int height) {
    if (DesktopConfig == nullptr || width <= 0 || height <= 0) {
        return;
    }

    DirtyRect rect;
    rect.X = ClampInt(x, 0, (int)DesktopConfig->Width);
    rect.Y = ClampInt(y, 0, (int)DesktopConfig->Height);
    int rectRight = ClampInt(x + width, 0, (int)DesktopConfig->Width);
    int rectBottom = ClampInt(y + height, 0, (int)DesktopConfig->Height);
    rect.Width = rectRight - rect.X;
    rect.Height = rectBottom - rect.Y;

    if (rect.Width <= 0 || rect.Height <= 0) {
        return;
    }

    for (int index = 0; index < DirtyRectCount; index++) {
        if (RectsTouchOrOverlap(DirtyRects[index], rect)) {
            DirtyRects[index] = UnionRects(DirtyRects[index], rect);
            return;
        }
    }

    if (DirtyRectCount < MAX_DIRTY_RECTS) {
        DirtyRects[DirtyRectCount++] = rect;
    }
    else {
        DirtyRects[0] = UnionRects(DirtyRects[0], rect);
    }
}

static void AddWindowDirtyRect() {
    if (!AppWindowVisible()) {
        return;
    }

    AddDirtyRect(WindowX - WINDOW_SHADOW_SIZE,
        WindowY - WINDOW_SHADOW_SIZE,
        WindowDisplayWidth() + (WINDOW_SHADOW_SIZE * 2),
        WindowDisplayHeight() + (WINDOW_SHADOW_SIZE * 2));
}

static void ClampWindowToDesktop() {
    if (DesktopConfig == nullptr || !BootAppScene.Loaded) {
        return;
    }

    int maxX = (int)DesktopConfig->Width - DESKTOP_MARGIN - WindowDisplayWidth();
    int maxY = WorkAreaHeight() - WindowDisplayHeight();

    if (maxX < DESKTOP_MARGIN) maxX = DESKTOP_MARGIN;
    if (maxY < DESKTOP_MARGIN) maxY = DESKTOP_MARGIN;

    WindowX = ClampInt(WindowX, DESKTOP_MARGIN, maxX);
    WindowY = ClampInt(WindowY, DESKTOP_MARGIN, maxY);
}

static void InvalidateDesktop() {
    RenderDesktopSurface();
    if (DesktopConfig != nullptr) {
        AddDirtyRect(0, 0, (int)DesktopConfig->Width, (int)DesktopConfig->Height);
    }
}

static void SaveRestoredWindowBounds() {
    if (WindowMaximized || WindowMinimized) {
        return;
    }

    RestoredWindowX = WindowX;
    RestoredWindowY = WindowY;
    RestoredWindowWidth = WindowDisplayWidth();
    RestoredWindowHeight = WindowDisplayHeight();
}

static void RestoreWindowState() {
    if (!BootAppScene.Loaded || DesktopConfig == nullptr) {
        return;
    }

    AddWindowDirtyRect();
    WindowMinimized = false;
    WindowMaximized = false;
    WindowDragging = false;
    WindowResizing = false;

    WindowWidth = RestoredWindowWidth > 0 ? RestoredWindowWidth : BootAppScene.CanvasWidth;
    WindowHeight = RestoredWindowHeight > 0 ? RestoredWindowHeight : BootAppScene.CanvasHeight;
    WindowX = RestoredWindowX;
    WindowY = RestoredWindowY;
    ClampWindowToDesktop();
    UpdateSystemStateBindings();
    InvalidateDesktop();
    AddWindowDirtyRect();
}

static void MinimizeWindow() {
    if (!AppHasWindowFrame() || WindowMinimized) {
        return;
    }

    SaveRestoredWindowBounds();
    AddWindowDirtyRect();
    WindowMinimized = true;
    WindowMaximized = false;
    WindowDragging = false;
    WindowResizing = false;
    InvalidateDesktop();
    UpdateSystemStateBindings();
}

static void MaximizeWindow() {
    if (!AppHasWindowFrame() || DesktopConfig == nullptr) {
        return;
    }

    if (WindowMaximized) {
        RestoreWindowState();
        return;
    }

    SaveRestoredWindowBounds();
    AddWindowDirtyRect();

    WindowMinimized = false;
    WindowMaximized = true;
    WindowDragging = false;
    WindowResizing = false;
    WindowX = DESKTOP_MARGIN;
    WindowY = DESKTOP_MARGIN;
    WindowWidth = (int)DesktopConfig->Width - (DESKTOP_MARGIN * 2);
    WindowHeight = WorkAreaHeight() - DESKTOP_MARGIN;
    if (WindowWidth < MIN_WINDOW_WIDTH) WindowWidth = MIN_WINDOW_WIDTH;
    if (WindowHeight < MIN_WINDOW_HEIGHT) WindowHeight = MIN_WINDOW_HEIGHT;
    ClampWindowToDesktop();
    UpdateSystemStateBindings();
    InvalidateDesktop();
    AddWindowDirtyRect();
}

static void GetElementAbsoluteBounds(const AppScene* scene, const AppElement* element, int* x, int* y, int* width, int* height) {
    int absoluteX = element->X;
    int absoluteY = element->Y;
    int currentParentId = element->ParentId;

    while (currentParentId >= 0) {
        const AppElement* parent = FindAppElement(scene, (uint32_t)currentParentId);
        if (parent == nullptr) {
            break;
        }

        absoluteX += parent->X;
        absoluteY += parent->Y;
        currentParentId = parent->ParentId;
    }

    *x = absoluteX;
    *y = absoluteY;
    *width = element->Width;
    *height = element->Height;
}

static int HitTestElement(const AppScene* scene, int localX, int localY, uint32_t requiredFlags) {
    if (scene == nullptr || !scene->Loaded || !scene->WindowOpen) {
        return -1;
    }

    for (int index = scene->ElementCount - 1; index >= 0; index--) {
        const AppElement* element = &scene->Elements[index];
        if ((element->Flags & AppElementFlagVisible) == 0 || (element->Flags & requiredFlags) != requiredFlags) {
            continue;
        }

        int x, y, width, height;
        GetElementAbsoluteBounds(scene, element, &x, &y, &width, &height);
        if (PointInRect(localX, localY, x, y, width, height)) {
            return index;
        }
    }

    return -1;
}

static void RenderDesktopToSurface(WindowSurface* surface) {
    if (surface == nullptr || !surface->Valid) {
        return;
    }

    SurfaceFill(surface, DESKTOP_COLOR);

    if (DesktopBackground != nullptr) {
        SurfaceDrawBMP(surface, DesktopBackground, 0, 0, surface->Width, surface->Height);
    }

    int taskbarY = surface->Height - TASKBAR_HEIGHT;
    SurfaceFillRect(surface, 0, taskbarY, surface->Width, TASKBAR_HEIGHT, TASKBAR_COLOR);
    SurfaceFillRect(surface, 0, taskbarY, surface->Width, 2, TASKBAR_EDGE_COLOR);
    SurfacePrint(surface, "WinOS", 16, taskbarY + 10, WINDOW_TEXT_COLOR);

    if (BootAppScene.Loaded && WindowMinimized) {
        const AppElement* root = RootWindowElement();
        const char* buttonText = (root != nullptr && root->Text[0] != '\0') ? root->Text : "Restore App";
        int buttonY = taskbarY + ((TASKBAR_HEIGHT - TASKBAR_APP_BUTTON_HEIGHT) / 2);
        SurfaceFillRect(surface, TASKBAR_APP_BUTTON_X, buttonY, TASKBAR_APP_BUTTON_WIDTH, TASKBAR_APP_BUTTON_HEIGHT, 0x1E3247);
        SurfaceDrawBorder(surface, TASKBAR_APP_BUTTON_X, buttonY, TASKBAR_APP_BUTTON_WIDTH, TASKBAR_APP_BUTTON_HEIGHT, 0x5A88B4);
        SurfacePrint(surface, buttonText, TASKBAR_APP_BUTTON_X + 10, buttonY + 5, WINDOW_TEXT_COLOR);
    }

    if (!BootAppScene.Loaded) {
        SurfacePrint(surface, "No BOOTAPP.wdexe exported yet.", 88, taskbarY + 10, WINDOW_TEXT_MUTED_COLOR);
        SurfacePrint(surface, "Export a boot app from WinOS-Editor to populate the desktop.", 24, 24, WINDOW_TEXT_MUTED_COLOR);
    }
    else if (!BootAppScene.WindowOpen) {
        SurfacePrint(surface, "BOOTAPP is loaded but currently closed.", 88, taskbarY + 10, WINDOW_TEXT_MUTED_COLOR);
        SurfacePrint(surface, "Re-export or reboot to reopen the boot application window.", 24, 24, WINDOW_TEXT_MUTED_COLOR);
    }
    else {
        SurfacePrint(surface, "Drag marked regions. Buttons and sliders stay interactive.", 88, taskbarY + 10, WINDOW_TEXT_MUTED_COLOR);
        SurfacePrint(surface, "App chrome comes from BOOTAPP.wdexe, not hard-coded kernel decorations.", 24, 24, WINDOW_TEXT_DIM_COLOR);
    }
}

static void RenderDesktopSurface() {
    if (!DesktopSurface.Valid) {
        return;
    }

    RenderDesktopToSurface(&DesktopSurface);
}

static void DrawButtonElement(const AppElement* element, int x, int y, bool pressed) {
    uint32_t background = pressed ? AdjustColor(element->BackgroundColor, -24) : element->BackgroundColor;
    SurfaceFillRect(&AppSurface, x, y, element->Width, element->Height, background);
    SurfaceDrawBorder(&AppSurface, x, y, element->Width, element->Height, element->BorderColor);

    int textWidth = 0;
    for (const char* character = element->Text; *character != '\0'; character++) {
        textWidth += 8;
    }

    int textX = x + ((element->Width - textWidth) / 2);
    int textY = y + ((element->Height - 16) / 2);
    SurfacePrint(&AppSurface, element->Text, textX, textY, element->ForegroundColor);
}

static void DrawSliderElement(const AppElement* element, int x, int y, bool active) {
    SurfaceFillRect(&AppSurface, x, y, element->Width, element->Height, element->BackgroundColor);
    SurfaceDrawBorder(&AppSurface, x, y, element->Width, element->Height, element->BorderColor);

    int trackX = x + 6;
    int trackY = y + (element->Height / 2) - 3;
    int trackWidth = element->Width - 12;
    if (trackWidth < 8) trackWidth = 8;
    SurfaceFillRect(&AppSurface, trackX, trackY, trackWidth, 6, AdjustColor(element->BackgroundColor, 18));

    int range = element->MaxValue - element->MinValue;
    int knobOffset = 0;
    if (range > 0) {
        knobOffset = ((element->Value - element->MinValue) * (trackWidth - 12)) / range;
    }

    int knobX = trackX + knobOffset;
    uint32_t knobColor = active ? AdjustColor(element->ForegroundColor, 20) : element->ForegroundColor;
    SurfaceFillRect(&AppSurface, knobX, y + 4, 12, element->Height - 8, knobColor);
    SurfaceDrawBorder(&AppSurface, knobX, y + 4, 12, element->Height - 8, element->BorderColor);
}

static void DrawTextFieldElement(const AppElement* element, int x, int y, bool focused) {
    uint32_t borderColor = focused ? AdjustColor(element->BorderColor, 24) : element->BorderColor;
    SurfaceFillRect(&AppSurface, x, y, element->Width, element->Height, element->BackgroundColor);
    SurfaceDrawBorder(&AppSurface, x, y, element->Width, element->Height, borderColor);
    SurfacePrint(&AppSurface, element->Text, x + 10, y + ((element->Height - 16) / 2), element->ForegroundColor);

    if (focused) {
        int textWidth = 0;
        for (const char* character = element->Text; *character != '\0'; character++) {
            textWidth += 8;
        }

        int caretX = x + 10 + textWidth;
        if (caretX > x + element->Width - 4) {
            caretX = x + element->Width - 4;
        }

        SurfaceFillRect(&AppSurface, caretX, y + 7, 2, element->Height - 14, borderColor);
    }
}

static void RestartSystem() {
    __outbyte(0x64, 0xFE);
    while (true) {
        __halt();
    }
}

static void ShutdownSystem() {
    while (true) {
        __halt();
    }
}

static void BusyWait(int iterations) {
    volatile int spin = 0;
    for (int index = 0; index < iterations; index++) {
        spin++;
    }
}

static void PlayTone(int frequency, int durationMilliseconds) {
    if (frequency < 37) frequency = 37;
    if (frequency > 16000) frequency = 16000;
    if (durationMilliseconds <= 0) durationMilliseconds = 120;
    if (durationMilliseconds > 2000) durationMilliseconds = 2000;

    uint16_t divisor = (uint16_t)(1193180 / frequency);
    __outbyte(0x43, 0xB6);
    __outbyte(0x42, (uint8_t)(divisor & 0xFF));
    __outbyte(0x42, (uint8_t)((divisor >> 8) & 0xFF));

    uint8_t speakerState = __inbyte(0x61);
    __outbyte(0x61, speakerState | 0x03);

    for (int millisecond = 0; millisecond < durationMilliseconds; millisecond++) {
        BusyWait(45000);
    }

    __outbyte(0x61, __inbyte(0x61) & 0xFC);
}

static void RenderAppSurface() {
    if (!BootAppScene.Loaded || !BootAppScene.WindowOpen) {
        AppSurface.Valid = false;
        return;
    }

    InitializeSurface(&AppSurface, AppSurfacePixels, BootAppScene.CanvasWidth, BootAppScene.CanvasHeight, MAX_WINDOW_PIXELS);
    if (!AppSurface.Valid) {
        BootAppScene.WindowOpen = false;
        return;
    }

    SurfaceFill(&AppSurface, 0x101822);

    for (int index = 0; index < BootAppScene.ElementCount; index++) {
        const AppElement* element = &BootAppScene.Elements[index];
        if ((element->Flags & AppElementFlagVisible) == 0) {
            continue;
        }

        int x, y, width, height;
        GetElementAbsoluteBounds(&BootAppScene, element, &x, &y, &width, &height);

        switch (element->Type) {
        case AppElementTypeRootWindow:
            SurfaceFillRect(&AppSurface, x, y, width, height, element->BackgroundColor);
            SurfaceDrawBorder(&AppSurface, x, y, width, height, element->BorderColor);
            break;
        case AppElementTypePanel:
            SurfaceFillRect(&AppSurface, x, y, width, height, element->BackgroundColor);
            SurfaceDrawBorder(&AppSurface, x, y, width, height, element->BorderColor);
            break;
        case AppElementTypeLabel:
            SurfaceFillRect(&AppSurface, x, y, width, height, element->BackgroundColor);
            SurfacePrint(&AppSurface, element->Text, x, y, element->ForegroundColor);
            break;
        case AppElementTypeButton:
            DrawButtonElement(element, x, y, BootAppScene.PressedElementId == (int)element->Id);
            break;
        case AppElementTypeSlider:
            DrawSliderElement(element, x, y, BootAppScene.CapturedElementId == (int)element->Id);
            break;
        case AppElementTypeTextField:
            DrawTextFieldElement(element, x, y, BootAppScene.FocusedElementId == (int)element->Id);
            break;
        case AppElementTypeImage:
            SurfaceFillRect(&AppSurface, x, y, width, height, element->BackgroundColor);
            SurfaceDrawBorder(&AppSurface, x, y, width, height, element->BorderColor);
            SurfaceDrawBMP(&AppSurface, element->AssetAddress, x, y, width, height);
            break;
        default:
            break;
        }
    }

    BootAppScene.SurfaceDirty = false;
}

static bool BuildSystemStateText(const char* stateKey, char* buffer, int capacity) {
    if (stateKey == nullptr || buffer == nullptr || capacity <= 0) {
        return false;
    }

    buffer[0] = '\0';

    if (TextEqualsIgnoreCase(stateKey, "TIME_GMT")) {
        FormatTimeText(buffer, capacity);
        return true;
    }

    if (TextEqualsIgnoreCase(stateKey, "CURRENT_APP")) {
        CopyText(buffer, capacity, CurrentAppAlias[0] != '\0' ? CurrentAppAlias : "BOOTAPP");
        return true;
    }

    if (TextEqualsIgnoreCase(stateKey, "APP_COUNT")) {
        IntToText(AppPackageCount, buffer, capacity);
        return true;
    }

    if (TextEqualsIgnoreCase(stateKey, "WINDOW_STATE")) {
        if (WindowMinimized) {
            CopyText(buffer, capacity, "Minimized");
        }
        else if (WindowMaximized) {
            CopyText(buffer, capacity, "Maximized");
        }
        else {
            CopyText(buffer, capacity, "Windowed");
        }

        return true;
    }

    if (TextEqualsIgnoreCase(stateKey, "SCREEN_SIZE")) {
        IntToText((int)DesktopConfig->Width, buffer, capacity);
        AppendText(buffer, capacity, "x");
        char heightBuffer[16];
        IntToText((int)DesktopConfig->Height, heightBuffer, 16);
        AppendText(buffer, capacity, heightBuffer);
        return true;
    }

    return false;
}

static void UpdateSystemStateBindings() {
    if (!BootAppScene.Loaded) {
        return;
    }

    bool changed = false;
    char textBuffer[APP_TEXT_CAPACITY];

    for (int bindingIndex = 0; bindingIndex < BootAppScene.BindingCount; bindingIndex++) {
        const AppBinding* binding = &BootAppScene.Bindings[bindingIndex];
        if (binding->ActionType != AppActionSetTextFromSystemState) {
            continue;
        }

        AppElement* target = FindAppElement(&BootAppScene, binding->TargetElementId);
        if (target == nullptr) {
            continue;
        }

        if (!BuildSystemStateText(binding->StringValue, textBuffer, APP_TEXT_CAPACITY)) {
            continue;
        }

        bool differs = false;
        for (int index = 0; index < APP_TEXT_CAPACITY; index++) {
            if (target->Text[index] != textBuffer[index]) {
                differs = true;
                break;
            }

            if (textBuffer[index] == '\0') {
                break;
            }
        }

        if (!differs) {
            continue;
        }

        CopyText(target->Text, APP_TEXT_CAPACITY, textBuffer);
        changed = true;
    }

    if (changed) {
        BootAppScene.SurfaceDirty = true;
        AddWindowDirtyRect();
    }
}

static bool LaunchNextApp() {
    if (AppPackageCount <= 1) {
        return false;
    }

    if (CurrentAppIndex < 0) {
        SetCurrentAppAlias(CurrentAppAlias[0] != '\0' ? CurrentAppAlias : "BOOTAPP");
    }

    int nextIndex = CurrentAppIndex >= 0 ? (CurrentAppIndex + 1) % AppPackageCount : 0;
    return OpenAppByAlias(AppPackageAliases[nextIndex]);
}

static bool OpenAppByAlias(const char* packageAlias) {
    if (DesktopConfig == nullptr || packageAlias == nullptr || packageAlias[0] == '\0') {
        return false;
    }

    AddWindowDirtyRect();

    if (!LoadAppScene(DesktopConfig, packageAlias, &AppLoadBuffer)) {
        return false;
    }

    BootAppScene = AppLoadBuffer;
    WindowDragging = false;
    WindowResizing = false;
    PreviousLeftDown = false;
    DragOffsetX = 0;
    DragOffsetY = 0;
    WindowMaximized = false;
    WindowMinimized = false;

    RenderAppSurface();
    WindowWidth = BootAppScene.CanvasWidth;
    WindowHeight = BootAppScene.CanvasHeight;
    WindowX = ((int)DesktopConfig->Width - BootAppScene.CanvasWidth) / 2;
    WindowY = ((WorkAreaHeight() - BootAppScene.CanvasHeight) / 2);
    ClampWindowToDesktop();
    SaveRestoredWindowBounds();
    SetCurrentAppAlias(packageAlias);
    UpdateSystemStateBindings();
    InvalidateDesktop();
    AddWindowDirtyRect();
    return true;
}

static void CopyBackBufferRectToFront(const DirtyRect& rect) {
    for (int y = rect.Y; y < rect.Y + rect.Height; y++) {
        for (int x = rect.X; x < rect.X + rect.Width; x++) {
            uint32_t pixel = DesktopConfig->BackBuffer[x + (y * DesktopConfig->PixelsPerScanLine)];
            DesktopConfig->BaseAddress[x + (y * DesktopConfig->PixelsPerScanLine)] = pixel;
        }
    }
}

static void CompositeAppWindowToSurface(WindowSurface* surface, const DirtyRect* clipRect) {
    if (surface == nullptr || !surface->Valid || !AppWindowVisible()) {
        return;
    }

    int clipLeft = clipRect != nullptr ? clipRect->X : 0;
    int clipTop = clipRect != nullptr ? clipRect->Y : 0;
    int clipRight = clipRect != nullptr ? (clipRect->X + clipRect->Width) : surface->Width;
    int clipBottom = clipRect != nullptr ? (clipRect->Y + clipRect->Height) : surface->Height;

    clipLeft = ClampInt(clipLeft, 0, surface->Width);
    clipTop = ClampInt(clipTop, 0, surface->Height);
    clipRight = ClampInt(clipRight, 0, surface->Width);
    clipBottom = ClampInt(clipBottom, 0, surface->Height);

    if (clipLeft >= clipRight || clipTop >= clipBottom) {
        return;
    }

    int windowWidth = WindowDisplayWidth();
    int windowHeight = WindowDisplayHeight();
    int shadowX = WindowX + WINDOW_SHADOW_SIZE;
    int shadowY = WindowY + WINDOW_SHADOW_SIZE;
    int shadowRight = shadowX + windowWidth;
    int shadowBottom = shadowY + windowHeight;

    int shadowStartX = shadowX > clipLeft ? shadowX : clipLeft;
    int shadowStartY = shadowY > clipTop ? shadowY : clipTop;
    int shadowEndX = shadowRight < clipRight ? shadowRight : clipRight;
    int shadowEndY = shadowBottom < clipBottom ? shadowBottom : clipBottom;

    for (int y = shadowStartY; y < shadowEndY; y++) {
        for (int x = shadowStartX; x < shadowEndX; x++) {
            surface->Pixels[x + (y * surface->Stride)] = WINDOW_SHADOW_COLOR;
        }
    }

    int windowRight = WindowX + windowWidth;
    int windowBottom = WindowY + windowHeight;
    int copyStartX = WindowX > clipLeft ? WindowX : clipLeft;
    int copyStartY = WindowY > clipTop ? WindowY : clipTop;
    int copyEndX = windowRight < clipRight ? windowRight : clipRight;
    int copyEndY = windowBottom < clipBottom ? windowBottom : clipBottom;

    for (int y = copyStartY; y < copyEndY; y++) {
        int sourceY = ((y - WindowY) * AppSurface.Height) / windowHeight;
        sourceY = ClampInt(sourceY, 0, AppSurface.Height - 1);
        for (int x = copyStartX; x < copyEndX; x++) {
            int sourceX = ((x - WindowX) * AppSurface.Width) / windowWidth;
            sourceX = ClampInt(sourceX, 0, AppSurface.Width - 1);
            surface->Pixels[x + (y * surface->Stride)] =
                AppSurface.Pixels[sourceX + (sourceY * AppSurface.Stride)];
        }
    }
}

static void RenderFullFrameToBackBuffer() {
    WindowSurface backBufferSurface = CreateBackBufferSurface();
    if (!backBufferSurface.Valid) {
        return;
    }

    RenderDesktopToSurface(&backBufferSurface);
    CompositeAppWindowToSurface(&backBufferSurface, nullptr);
}

static void ComposeDirtyRect(const DirtyRect& rect) {
    if (!DesktopSurface.Valid) {
        return;
    }

    for (int y = rect.Y; y < rect.Y + rect.Height; y++) {
        for (int x = rect.X; x < rect.X + rect.Width; x++) {
            DesktopConfig->BackBuffer[x + (y * DesktopConfig->PixelsPerScanLine)] =
                DesktopSurface.Pixels[x + (y * DesktopSurface.Stride)];
        }
    }

    WindowSurface backBufferSurface = CreateBackBufferSurface();
    CompositeAppWindowToSurface(&backBufferSurface, &rect);
}

static void PresentDirtyRects() {
    if (DesktopConfig == nullptr || DirtyRectCount == 0) {
        return;
    }

    if (CursorPresented) {
        RestoreCursorArea(PresentedCursorX, PresentedCursorY);
        CursorPresented = false;
    }

    for (int index = 0; index < DirtyRectCount; index++) {
        ComposeDirtyRect(DirtyRects[index]);
        CopyBackBufferRectToFront(DirtyRects[index]);
    }

    DirtyRectCount = 0;
    DrawCursorToFrontBuffer(CursorX, CursorY);
    PresentedCursorX = CursorX;
    PresentedCursorY = CursorY;
    CursorPresented = true;
    CursorPresentNeeded = false;
}

static bool SetElementVisible(AppElement* element, bool visible) {
    uint32_t oldFlags = element->Flags;
    if (visible) {
        element->Flags |= AppElementFlagVisible;
    }
    else {
        element->Flags &= ~AppElementFlagVisible;
    }

    return oldFlags != element->Flags;
}

static void ApplyBinding(const AppElement* sourceElement, const AppBinding* binding) {
    if (binding == nullptr) {
        return;
    }

    switch (binding->ActionType) {
    case AppActionSetProperty:
    {
        AppElement* target = FindAppElement(&BootAppScene, binding->TargetElementId);
        if (target == nullptr) {
            return;
        }

        bool changed = false;
        switch (binding->PropertyType) {
        case AppPropertyBackgroundColor:
            changed = target->BackgroundColor != binding->ColorValue;
            target->BackgroundColor = binding->ColorValue;
            break;
        case AppPropertyForegroundColor:
            changed = target->ForegroundColor != binding->ColorValue;
            target->ForegroundColor = binding->ColorValue;
            break;
        case AppPropertyBorderColor:
            changed = target->BorderColor != binding->ColorValue;
            target->BorderColor = binding->ColorValue;
            break;
        case AppPropertyText:
            CopyText(target->Text, APP_TEXT_CAPACITY, binding->StringValue);
            changed = true;
            break;
        case AppPropertyValue:
            changed = target->Value != binding->IntValue;
            target->Value = binding->IntValue;
            break;
        case AppPropertyVisible:
            changed = SetElementVisible(target, binding->IntValue != 0);
            break;
        default:
            break;
        }

        if (changed) {
            BootAppScene.SurfaceDirty = true;
            AddWindowDirtyRect();
        }
        break;
    }
    case AppActionSetTextFromSourceValue:
    {
        if (sourceElement == nullptr) {
            return;
        }

        AppElement* target = FindAppElement(&BootAppScene, binding->TargetElementId);
        if (target == nullptr) {
            return;
        }

        char valueBuffer[APP_TEXT_CAPACITY];
        IntToText(sourceElement->Value, valueBuffer, APP_TEXT_CAPACITY);
        CopyText(target->Text, APP_TEXT_CAPACITY, valueBuffer);
        BootAppScene.SurfaceDirty = true;
        AddWindowDirtyRect();
        break;
    }
    case AppActionCloseWindow:
        if (BootAppScene.WindowOpen) {
            AddWindowDirtyRect();
            BootAppScene.WindowOpen = false;
            BootAppScene.PressedElementId = -1;
            BootAppScene.CapturedElementId = -1;
            WindowDragging = false;
            WindowResizing = false;
            WindowMinimized = false;
            WindowMaximized = false;
            UpdateSystemStateBindings();
            InvalidateDesktop();
        }
        break;
    case AppActionBringToFront:
        break;
    case AppActionShutdown:
        ShutdownSystem();
        break;
    case AppActionRestart:
        RestartSystem();
        break;
    case AppActionLaunchApp:
        OpenAppByAlias(binding->StringValue);
        break;
    case AppActionPlayTone:
        PlayTone(binding->IntValue != 0 ? binding->IntValue : 880, 120);
        break;
    case AppActionMinimizeWindow:
        MinimizeWindow();
        break;
    case AppActionMaximizeWindow:
        MaximizeWindow();
        break;
    case AppActionRestoreWindow:
        RestoreWindowState();
        break;
    case AppActionSetTextFromSystemState:
        UpdateSystemStateBindings();
        break;
    case AppActionLaunchNextApp:
        LaunchNextApp();
        break;
    default:
        break;
    }
}

static void DispatchEvent(uint32_t sourceElementId, uint32_t eventType) {
    const AppElement* sourceElement = FindAppElement(&BootAppScene, sourceElementId);
    for (int eventIndex = 0; eventIndex < BootAppScene.EventCount; eventIndex++) {
        const AppEvent* appEvent = &BootAppScene.Events[eventIndex];
        if (appEvent->SourceElementId != sourceElementId || appEvent->EventType != eventType) {
            continue;
        }

        for (uint32_t bindingOffset = 0; bindingOffset < appEvent->BindingCount; bindingOffset++) {
            uint32_t bindingIndex = appEvent->FirstBindingIndex + bindingOffset;
            if (bindingIndex >= (uint32_t)BootAppScene.BindingCount) {
                break;
            }

            ApplyBinding(sourceElement, &BootAppScene.Bindings[bindingIndex]);
        }
    }
}

static bool UpdateSliderValue(AppElement* sliderElement, int localMouseX) {
    if (sliderElement == nullptr || sliderElement->Type != AppElementTypeSlider) {
        return false;
    }

    int elementX, elementY, elementWidth, elementHeight;
    GetElementAbsoluteBounds(&BootAppScene, sliderElement, &elementX, &elementY, &elementWidth, &elementHeight);

    int trackWidth = elementWidth - 12;
    if (trackWidth < 8) trackWidth = 8;

    int relative = localMouseX - (elementX + 6);
    relative = ClampInt(relative, 0, trackWidth - 12);

    int range = sliderElement->MaxValue - sliderElement->MinValue;
    int newValue = sliderElement->MinValue;
    if (range > 0 && trackWidth > 12) {
        newValue = sliderElement->MinValue + ((relative * range) / (trackWidth - 12));
    }

    if (newValue == sliderElement->Value) {
        return false;
    }

    sliderElement->Value = newValue;
    DispatchEvent(sliderElement->Id, AppEventValueChanged);
    BootAppScene.SurfaceDirty = true;
    AddWindowDirtyRect();
    return true;
}

void WindowManagerInit(BOOT_CONFIG* config) {
    DesktopConfig = config;
    DesktopBackground = nullptr;
    DirtyRectCount = 0;
    WindowX = DESKTOP_MARGIN * 3;
    WindowY = DESKTOP_MARGIN * 3;
    WindowWidth = 0;
    WindowHeight = 0;
    RestoredWindowX = 0;
    RestoredWindowY = 0;
    RestoredWindowWidth = 0;
    RestoredWindowHeight = 0;
    CursorX = 0;
    CursorY = 0;
    PresentedCursorX = 0;
    PresentedCursorY = 0;
    CursorPresented = false;
    CursorPresentNeeded = false;
    PreviousLeftDown = false;
    WindowDragging = false;
    WindowResizing = false;
    WindowMaximized = false;
    WindowMinimized = false;
    DragOffsetX = 0;
    DragOffsetY = 0;
    ResizeOriginX = 0;
    ResizeOriginY = 0;
    ResizeOriginWidth = 0;
    ResizeOriginHeight = 0;
    LastRenderedSecond = -1;
    CurrentAppAlias[0] = '\0';

    if (DesktopConfig == nullptr) {
        return;
    }

    InitializeSurface(&DesktopSurface, DesktopSurfacePixels, (int)DesktopConfig->Width, (int)DesktopConfig->Height, MAX_DESKTOP_PIXELS);
    DesktopBackground = GetAsset("BACKGROUND", DesktopConfig);
    CursorX = (int)DesktopConfig->Width / 2;
    CursorY = (int)DesktopConfig->Height / 2;

    LoadCursorMetadata();
    DiscoverAppPackages();
    LoadBootAppScene(DesktopConfig, &BootAppScene);

    if (BootAppScene.Loaded) {
        RenderAppSurface();
        WindowWidth = BootAppScene.CanvasWidth;
        WindowHeight = BootAppScene.CanvasHeight;
        WindowX = ((int)DesktopConfig->Width - BootAppScene.CanvasWidth) / 2;
        WindowY = ((WorkAreaHeight() - BootAppScene.CanvasHeight) / 2);
        ClampWindowToDesktop();
        SaveRestoredWindowBounds();
        SetCurrentAppAlias("BOOTAPP");
        UpdateSystemStateBindings();
    }

    RenderDesktopSurface();
    AddDirtyRect(0, 0, (int)DesktopConfig->Width, (int)DesktopConfig->Height);
}

void WindowManagerHandleMouseState(int mouseX, int mouseY, bool leftDown) {
    if (DesktopConfig == nullptr) {
        return;
    }

    mouseX = ClampInt(mouseX, 0, (int)DesktopConfig->Width - 1);
    mouseY = ClampInt(mouseY, 0, (int)DesktopConfig->Height - 1);

    bool moved = mouseX != CursorX || mouseY != CursorY;
    CursorX = mouseX;
    CursorY = mouseY;

    if (leftDown && !PreviousLeftDown) {
        if (WindowMinimized &&
            PointInRect(mouseX, mouseY, TASKBAR_APP_BUTTON_X, TaskbarButtonY(), TASKBAR_APP_BUTTON_WIDTH, TASKBAR_APP_BUTTON_HEIGHT)) {
            RestoreWindowState();
            PreviousLeftDown = leftDown;
            return;
        }

        if (AppWindowVisible() &&
            PointInRect(mouseX, mouseY, WindowX, WindowY, WindowDisplayWidth(), WindowDisplayHeight())) {
            BootAppScene.FocusedElementId = (int)BootAppScene.RootElementId;
            DispatchEvent(BootAppScene.RootElementId, AppEventFocusChanged);
            BootAppScene.SurfaceDirty = true;
            AddWindowDirtyRect();

            if (AppHasWindowFrame() && !WindowMaximized &&
                PointInRect(mouseX, mouseY,
                    WindowX + WindowDisplayWidth() - RESIZE_HANDLE_SIZE,
                    WindowY + WindowDisplayHeight() - RESIZE_HANDLE_SIZE,
                    RESIZE_HANDLE_SIZE,
                    RESIZE_HANDLE_SIZE)) {
                WindowResizing = true;
                ResizeOriginX = mouseX;
                ResizeOriginY = mouseY;
                ResizeOriginWidth = WindowDisplayWidth();
                ResizeOriginHeight = WindowDisplayHeight();
                SaveRestoredWindowBounds();
            }
            else {
                int localX = SceneXFromWindowX(mouseX);
                int localY = SceneYFromWindowY(mouseY);
                int interactiveIndex = HitTestElement(&BootAppScene, localX, localY, AppElementFlagInteractive);

                if (interactiveIndex >= 0) {
                    AppElement* target = &BootAppScene.Elements[interactiveIndex];
                    if (target->Type == AppElementTypeTextField) {
                        BootAppScene.FocusedElementId = (int)target->Id;
                        BootAppScene.SurfaceDirty = true;
                        AddWindowDirtyRect();
                    }
                    DispatchEvent(target->Id, AppEventPointerDown);

                    if (target->Type == AppElementTypeButton) {
                        BootAppScene.PressedElementId = (int)target->Id;
                        BootAppScene.SurfaceDirty = true;
                        AddWindowDirtyRect();
                    }
                    else if (target->Type == AppElementTypeSlider) {
                        BootAppScene.CapturedElementId = (int)target->Id;
                        UpdateSliderValue(target, localX);
                    }
                }
                else if (!WindowMaximized) {
                    int dragIndex = HitTestElement(&BootAppScene, localX, localY, AppElementFlagDraggableRegion);
                    if (dragIndex >= 0) {
                        WindowDragging = true;
                        DragOffsetX = mouseX - WindowX;
                        DragOffsetY = mouseY - WindowY;
                    }
                }
            }
        }
    }
    else if (!leftDown && PreviousLeftDown) {
        WindowDragging = false;
        WindowResizing = false;

        if (BootAppScene.CapturedElementId >= 0) {
            DispatchEvent((uint32_t)BootAppScene.CapturedElementId, AppEventPointerUp);
            BootAppScene.CapturedElementId = -1;
            BootAppScene.SurfaceDirty = true;
            AddWindowDirtyRect();
        }

        if (BootAppScene.PressedElementId >= 0) {
            int pressedId = BootAppScene.PressedElementId;
            BootAppScene.PressedElementId = -1;
            BootAppScene.SurfaceDirty = true;
            AddWindowDirtyRect();
            DispatchEvent((uint32_t)pressedId, AppEventPointerUp);

            AppElement* pressedElement = FindAppElement(&BootAppScene, (uint32_t)pressedId);
            if (pressedElement != nullptr && AppWindowVisible()) {
                int localX = SceneXFromWindowX(mouseX);
                int localY = SceneYFromWindowY(mouseY);
                int x, y, width, height;
                GetElementAbsoluteBounds(&BootAppScene, pressedElement, &x, &y, &width, &height);
                if (PointInRect(localX, localY, x, y, width, height)) {
                    DispatchEvent((uint32_t)pressedId, AppEventClick);
                }
            }
        }
    }

    if (leftDown && WindowResizing && moved && AppWindowVisible()) {
        int oldX = WindowX;
        int oldY = WindowY;
        int oldWidth = WindowDisplayWidth();
        int oldHeight = WindowDisplayHeight();

        WindowWidth = ResizeOriginWidth + (mouseX - ResizeOriginX);
        WindowHeight = ResizeOriginHeight + (mouseY - ResizeOriginY);
        if (WindowWidth < MIN_WINDOW_WIDTH) WindowWidth = MIN_WINDOW_WIDTH;
        if (WindowHeight < MIN_WINDOW_HEIGHT) WindowHeight = MIN_WINDOW_HEIGHT;
        WindowMaximized = false;
        ClampWindowToDesktop();
        SaveRestoredWindowBounds();

        AddDirtyRect(oldX - WINDOW_SHADOW_SIZE,
            oldY - WINDOW_SHADOW_SIZE,
            oldWidth + (WINDOW_SHADOW_SIZE * 2),
            oldHeight + (WINDOW_SHADOW_SIZE * 2));
        AddWindowDirtyRect();
    }
    else if (leftDown && WindowDragging && moved && AppWindowVisible()) {
        int oldX = WindowX;
        int oldY = WindowY;
        int oldWidth = WindowDisplayWidth();
        int oldHeight = WindowDisplayHeight();
        WindowX = mouseX - DragOffsetX;
        WindowY = mouseY - DragOffsetY;
        ClampWindowToDesktop();
        SaveRestoredWindowBounds();

        AddDirtyRect(oldX - WINDOW_SHADOW_SIZE,
            oldY - WINDOW_SHADOW_SIZE,
            oldWidth + (WINDOW_SHADOW_SIZE * 2),
            oldHeight + (WINDOW_SHADOW_SIZE * 2));
        AddWindowDirtyRect();
    }
    else if (leftDown && BootAppScene.CapturedElementId >= 0 && AppWindowVisible() && moved) {
        AppElement* capturedElement = FindAppElement(&BootAppScene, (uint32_t)BootAppScene.CapturedElementId);
        if (capturedElement != nullptr && capturedElement->Type == AppElementTypeSlider) {
            UpdateSliderValue(capturedElement, SceneXFromWindowX(mouseX));
            DispatchEvent(capturedElement->Id, AppEventPointerMove);
        }
    }

    if (moved && DirtyRectCount == 0) {
        CursorPresentNeeded = true;
    }

    PreviousLeftDown = leftDown;
}

void WindowManagerHandleKeyboardInput(KeyboardInputEvent inputEvent) {
    if (!BootAppScene.WindowOpen || WindowMinimized || BootAppScene.FocusedElementId < 0) {
        return;
    }

    AppElement* focusedElement = FindAppElement(&BootAppScene, (uint32_t)BootAppScene.FocusedElementId);
    if (focusedElement == nullptr || focusedElement->Type != AppElementTypeTextField) {
        return;
    }

    int textLength = 0;
    while (focusedElement->Text[textLength] != '\0' && textLength < APP_TEXT_CAPACITY - 1) {
        textLength++;
    }

    bool changed = false;
    if (inputEvent.Type == KeyboardInputCharacter) {
        if (textLength < APP_TEXT_CAPACITY - 1 && inputEvent.Character >= 32) {
            focusedElement->Text[textLength++] = inputEvent.Character;
            focusedElement->Text[textLength] = '\0';
            changed = true;
        }
    }
    else if (inputEvent.Type == KeyboardInputBackspace) {
        if (textLength > 0) {
            focusedElement->Text[textLength - 1] = '\0';
            changed = true;
        }
    }
    else if (inputEvent.Type == KeyboardInputEnter) {
        DispatchEvent(focusedElement->Id, AppEventClick);
    }

    if (changed) {
        DispatchEvent(focusedElement->Id, AppEventValueChanged);
        BootAppScene.SurfaceDirty = true;
        AddWindowDirtyRect();
    }
}

void WindowManagerTick() {
    if (!BootAppScene.Loaded) {
        return;
    }

    int hour = 0;
    int minute = 0;
    int second = 0;
    if (!ReadRtcTime(&hour, &minute, &second)) {
        return;
    }

    if (second != LastRenderedSecond) {
        LastRenderedSecond = second;
        UpdateSystemStateBindings();
    }
}

bool WindowManagerNeedsRedraw() {
    return DirtyRectCount > 0 || CursorPresentNeeded || (BootAppScene.WindowOpen && BootAppScene.SurfaceDirty);
}

void WindowManagerRender() {
    if (DesktopConfig == nullptr) {
        return;
    }

    if (BootAppScene.WindowOpen && BootAppScene.SurfaceDirty) {
        RenderAppSurface();
    }

    if (!DesktopSurface.Valid) {
        if (DirtyRectCount > 0) {
            if (CursorPresented) {
                RestoreCursorArea(PresentedCursorX, PresentedCursorY);
                CursorPresented = false;
            }

            RenderFullFrameToBackBuffer();
            SwitchBuffer(DesktopConfig);
            DirtyRectCount = 0;

            DrawCursorToFrontBuffer(CursorX, CursorY);
            PresentedCursorX = CursorX;
            PresentedCursorY = CursorY;
            CursorPresented = true;
            CursorPresentNeeded = false;
            return;
        }

        if (CursorPresentNeeded) {
            PresentCursorOnly();
        }

        return;
    }

    if (DirtyRectCount > 0) {
        PresentDirtyRects();
        return;
    }

    if (CursorPresentNeeded) {
        PresentCursorOnly();
    }
}
