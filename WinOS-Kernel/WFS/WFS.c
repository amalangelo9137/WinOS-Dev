#include "WFS.h"
#include "..\Graphics.h"

// This function finds a file by name and returns its raw memory pointer
void* WFS_GetFile(void* FSRoot, const char* name) {
    WFS_Superblock* sb = (WFS_Superblock*)FSRoot;
    if (sb->Magic != WFS_MAGIC) return 0; // Not a WFS drive!

    WFS_Inode* inodes = (WFS_Inode*)((uint8_t*)FSRoot + (sb->InodeTablePtr * 512));

    for (int i = 0; i < 128; i++) { // Search first 128 file slots
        if (StrCmp(inodes[i].FileName, name)) {
            // Calculate actual memory address
            return (uint8_t*)FSRoot + (sb->DataRegionPtr * 512) + (inodes[i].StartBlock * 512);
        }
    }
    return 0;
}