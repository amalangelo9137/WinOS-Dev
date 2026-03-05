#pragma once
#include <stdint.h>

#define WFS_MAGIC 0x53465711 // "WFS" + Version 1.1

typedef struct {
    uint32_t Magic;          // Should match WFS_MAGIC
    uint32_t TotalBlocks;    // Size of the drive
    uint32_t InodeTablePtr;  // Sector where the file list starts
    uint32_t DataRegionPtr;  // Sector where raw data starts
    char     VolumeName[16];
} WFS_Superblock;

typedef struct {
    char     FileName[32];
    uint32_t StartBlock;     // Offset from DataRegionPtr
    uint32_t FileSize;       // In bytes
    uint16_t Flags;          // 0 = Deleted, 1 = File, 2 = Directory
} WFS_Inode;