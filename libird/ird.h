/*
 * Copyright 2014: Cobra Team
 *
 * This software is released under the GPLv3 license
 */
#ifndef __IRD_H__
#define __IRD_H__

#ifdef WIN32
#include "stdint.h"
#else
#include <stdint.h>
#endif

#include <stdbool.h>

#define IRD_MAGIC 0x44524933
#define BD_SECTOR_SIZE 0x800

#ifdef __GNUC__
#define PACKED __attribute__((packed))
#else
#define PACKED
#endif

typedef struct _FileList
{
	char *path;
	uint32_t lba;
	uint64_t size;
	bool multipart;
	struct _FileList *next;
} FileList;

#pragma pack (push, 1)

typedef struct 
{
	uint32_t magic;
	uint8_t version;
	uint8_t game_id[9];
	uint8_t titleLen;
	char *title;
	uint8_t updateVersion[4];
	uint8_t gameVersion[5];
	uint8_t appVersion[5];
} PACKED IRDHeader;

typedef struct 
{
	uint8_t hash[16];
} PACKED IRDRegionHash;

typedef struct 
{
	uint32_t sector;
	uint32_t zero;
	uint8_t hash[16];
} PACKED IRDFileHash;

typedef struct 
{
	uint16_t reserved1;
	uint16_t reserved2;
	uint8_t d1[16];
	uint8_t d2[16];
	uint8_t pic[115];
} PACKED IRDFooter;

typedef struct 
{
	IRDHeader header;
	uint32_t zFileHeaderLen;
	uint8_t *zFileHeader;
	uint32_t fileHeaderLen;
	uint8_t *fileHeader;
	uint32_t zFileFooterLen;
	uint8_t *zFileFooter;
	uint32_t fileFooterLen;
	uint8_t *fileFooter;
	uint8_t numRegionHashes;
	IRDRegionHash *regionHashes;
	uint32_t numFileHashes;
	IRDFileHash *fileHashes;
	IRDFooter footer;
	uint32_t v8_unknown;
	uint32_t crc;
} PACKED IRDFile;
#pragma pack (pop)


IRDFile *readIRD(const char *irdFile);
void freeIRD(IRDFile *ird);
FileList * buildFileListFromIRD(const char *inDir, IRDFile *ird);
unsigned char *getFileHash(IRDFile *ird, FileList *file);
void freeFileList(FileList *fileList);

#endif /* __IRD_H__ */

