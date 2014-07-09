/*
 * Copyright 2014: Cobra Team
 *
 * This software is released under the GPLv3 license
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>

#include <zlib.h>

#ifndef WIN32
#include <unistd.h>
#endif


#include "ird.h"  
#include "iso9660.h"  

#define CHUNK (1024 * 1024)

int inflateIRD(const unsigned char *zird, long zirdSize, IRDFile *ird)
{
	int ret;
	z_stream strm;

	memset(ird, 0, sizeof(IRDFile));

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = zirdSize;
	strm.next_in = (Bytef *) zird;

	ret = inflateInit2(&strm, 16 + MAX_WBITS);
	if (ret != Z_OK)
	 	return ret;


	/* Magic - Version - Game ID - Title Len */
	strm.avail_out = sizeof(uint32_t) + sizeof(uint8_t) + 9 * sizeof(uint8_t) + sizeof(uint8_t);
	strm.next_out = (Bytef *) (&ird->header);
	if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
		goto error;
	if (ird->header.magic != IRD_MAGIC || (ird->header.version != 6 && ird->header.version != 8 && ird->header.version != 9))
	{
		inflateEnd(&strm);
		return -1;
	}
	/* Title */
	ird->header.title = new char[ird->header.titleLen];
	if (!ird->header.title)
		goto error;
	strm.avail_out = ird->header.titleLen;
	strm.next_out = (Bytef *) (ird->header.title);
	if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
		goto error;

	/* Update version - Game version - App version */
	strm.avail_out = (4 + 5 + 5) * sizeof(uint8_t);
	strm.next_out = (Bytef *) (&ird->header.updateVersion);
	if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
		goto error;
	/* File header */
	strm.avail_out = sizeof(uint32_t);
	strm.next_out = (Bytef *) (&ird->zFileHeaderLen);
	if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
		goto error;
	ird->zFileHeader = new uint8_t[ird->zFileHeaderLen];
	if (!ird->zFileHeader)
		goto error;
	strm.avail_out = ird->zFileHeaderLen;
	strm.next_out = (Bytef *) (ird->zFileHeader);
	if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
		goto error;
	/* File footer */
	strm.avail_out = sizeof(uint32_t);
	strm.next_out = (Bytef *) (&ird->zFileFooterLen);
	if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
		goto error;
	ird->zFileFooter = new uint8_t[ird->zFileFooterLen];
	if (!ird->zFileFooter)
		goto error;
	strm.avail_out = ird->zFileFooterLen;
	strm.next_out = (Bytef *) (ird->zFileFooter);
	if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
		goto error;
	/* Region hashes */
	strm.avail_out = sizeof(uint8_t);
	strm.next_out = (Bytef *) (&ird->numRegionHashes);
	if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
		goto error;
	ird->regionHashes = new IRDRegionHash[ird->numRegionHashes];
	if (!ird->regionHashes)
		goto error;
	strm.avail_out = ird->numRegionHashes * sizeof(IRDRegionHash);
	strm.next_out = (Bytef *) (ird->regionHashes);
	if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
		goto error;
	/* File hashes */
	strm.avail_out = sizeof(uint32_t);
	strm.next_out = (Bytef *) (&ird->numFileHashes);
	if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
		goto error;
	ird->fileHashes = new IRDFileHash[ird->numFileHashes];
	if (!ird->fileHashes)
		goto error;
	strm.avail_out = ird->numFileHashes * sizeof(IRDFileHash);
	strm.next_out = (Bytef *) (ird->fileHashes);
	if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
		goto error;
	/* File footer */
	if (ird->header.version >= 9) {
		/* Reserved */
		strm.avail_out = sizeof(uint16_t) + sizeof(uint16_t);
		strm.next_out = (Bytef *) (&ird->footer.reserved1);
		if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
			goto error;

		/* PIC Zone */
		strm.avail_out = sizeof(uint8_t) * 115;
		strm.next_out = (Bytef *) (&ird->footer.pic);
		if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
			goto error;

		/* D1 - D2 */
		strm.avail_out = sizeof(uint8_t) * 32;
		strm.next_out = (Bytef *) (&ird->footer.d1);
		if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
			goto error;

		/* Unknown */
		strm.avail_out = sizeof(uint32_t);
		strm.next_out = (Bytef *) (&ird->v8_unknown);
		if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
			goto error;
	} else {
		strm.avail_out = sizeof(IRDFooter);
		strm.next_out = (Bytef *) (&ird->footer);
		if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
			goto error;
		if (ird->header.version == 8) {
			strm.avail_out = sizeof(uint32_t);
			strm.next_out = (Bytef *) (&ird->v8_unknown);
			if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_OK)
				goto error;
		}
	}
	{
	uint32_t crc = strm.adler;

	strm.avail_out = sizeof(uint32_t);
	strm.next_out = (Bytef *) (&ird->crc);
	if ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_STREAM_END)
		goto error;

	if (crc != ird->crc)
	{
		ret = Z_DATA_ERROR;
		goto error;
	}
        }

	inflateEnd(&strm);
	return Z_OK;

error:
	inflateEnd(&strm);
	return ret;
}


int inflateInMemory(const unsigned char *zdata, long zdataSize, uint8_t **data, uint32_t *dataSize)
{
    int ret;
    z_stream strm;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = zdataSize;
    strm.next_in = (Bytef *) zdata;

    ret = inflateInit2(&strm, 16 + MAX_WBITS);
    if (ret != Z_OK)
        return ret;

	*dataSize = 0;
	while (strm.avail_in != 0) {
		*data = (uint8_t *) realloc(*data, *dataSize + CHUNK);
		strm.avail_out = CHUNK;
		strm.next_out = (Bytef *) *data + *dataSize;
		ret = inflate(&strm, Z_NO_FLUSH);
		if (ret != Z_OK && ret != Z_STREAM_END)
			goto error;
		
		*dataSize += CHUNK - strm.avail_out;
		if (ret == Z_STREAM_END || strm.avail_out != 0)
			break;
	}

	inflateEnd(&strm);
	return Z_OK;

error:
	inflateEnd(&strm);
	free(*data);
	*data = NULL;
	*dataSize = 0;
	return ret;
}

static char *createPathFromUTF16(const char *dir, const char *file, unsigned char len)
{
	char *ret = new char[strlen(dir) + (len / 2) + 2];
	char *p1;
	const char *p2;

	sprintf(ret, "%s/", dir);
	p1 = ret + strlen(ret);
	p2 = file;

	while (p2 < file + len)
	{
		if (p2[0] != 0) {
			fprintf(stderr, "Fatal error: this version of genps3iso only supports ansi characters in filenames\n");
			exit(-1);
			return NULL;
		}
		p1[0] = p2[1];
		p1++;
		p2 += 2;
	}
	p1[0] = 0;
	return ret;
}

static int _find_files_in_directory_r(IRDFile *ird, const char *directory, uint32_t start, uint32_t length, FileList **fileList)
{
	uint32_t position = 0;
	bool last_multiextent = false;

	while (position < length) {
		Iso9660DirectoryRecord *dirent = (Iso9660DirectoryRecord *) (ird->fileHeader + (start * BD_SECTOR_SIZE) + position);
		if (dirent->len_dr > 0) {
			position += dirent->len_dr;
			if (dirent->fileFlags == ISO_DIRECTORY && dirent->len_fi == 1 && (dirent->fi == 0x00 || dirent->fi == 0x01))
				continue;

			if (dirent->fileFlags == ISO_DIRECTORY) {
				char *path = createPathFromUTF16(directory, &dirent->fi, dirent->len_fi);
				if (_find_files_in_directory_r(ird, path, dirent->lsbStart, dirent->lsbDataLength, fileList) != 0) {
					delete []path;
					return -1;
				}
				delete []path;
			} else if (dirent->fileFlags == ISO_FILE || dirent->fileFlags == ISO_MULTIEXTENT) {
				FileList *file = NULL;

				file = new FileList;
				
				file->path = createPathFromUTF16(directory, &dirent->fi, dirent->len_fi);

				if (strcmp(file->path + strlen(file->path) - 2, ";1") == 0)
					file->path[strlen(file->path) - 2] = 0;
				file->lba = dirent->lsbStart;
				file->size = dirent->lsbDataLength;
				last_multiextent = file->multipart = (dirent->fileFlags == ISO_MULTIEXTENT) ? true : false;
				file->next = NULL;
				if (*fileList == NULL) {
					*fileList = file;
				} else {
					FileList *iter = NULL;
					FileList *current = NULL;

					for (iter = *fileList; iter; iter = iter->next) {
						if (iter->lba < file->lba) {
							current = iter;
						} else {
							break;
						}
					}
					if  (current && current->multipart) {
						if (strcmp(current->path, file->path) != 0 ||
							current->lba + (current->size / BD_SECTOR_SIZE) != file->lba) {
							fprintf(stderr, "Invalid multi-extent file detected in ISO\n");
							freeFileList(file);
							return -1;
						}
						current->multipart = file->multipart;
						current->size += file->size;
						freeFileList(file);
					} else {
						file->next = iter;
						if (current == NULL) {
							*fileList = file;
						} else {
							current->next = file;
						}
					}
				}
			} else {
				fprintf(stderr, "ISO contains an unknown file type\n");
				return -1;
			}
		} else {
			position += 0x800 - (position&0x7ff);
		}
	}

	if (last_multiextent) {
		fprintf(stderr, "Invalid multi-extent file detected in ISO\n");
		return -1;
	}
	return 0;
}

FileList * buildFileListFromIRD(const char *inDir, IRDFile *ird)
{
	FileList *fileList = NULL;
	Iso9660PVD *pvd;
	Iso9660DirectoryRecord *dirent;

	if (ird->fileHeader == NULL)
	{
		if (inflateInMemory(ird->zFileHeader, ird->zFileHeaderLen, &ird->fileHeader, &ird->fileHeaderLen) != 0)
		{
			fprintf(stderr, "Unable to extract file header\n");
			return NULL;
		}
	}
	pvd = (Iso9660PVD *)(ird->fileHeader + 0x8800);
	if (pvd->VDType == 2 && memcmp(pvd->VSStdId, "CD001", 5) == 0)
	{
		dirent = (Iso9660DirectoryRecord *) &(pvd->rootDirectoryRecord);
		if (_find_files_in_directory_r(ird, inDir, dirent->lsbStart, dirent->lsbDataLength, &fileList) != 0)
		{
			freeFileList(fileList);
			fileList = NULL;
		}
	}

	return fileList;
}

unsigned char *getFileHash(IRDFile *ird, FileList *file)
{
	uint32_t i;

	for (i = 0; i < ird->numFileHashes; i++)
	{
		if (ird->fileHashes[i].sector == file->lba)
		{
			return ird->fileHashes[i].hash;
		}
	}

	return NULL;
}

IRDFile *readIRD(const char *irdFile)
{
	FILE *f = fopen(irdFile, "rb");
	long zirdSize;
	IRDFile *ird;
	
	if (!f)
	{
		fprintf(stderr, "Cannot open %s\n", irdFile);
		return NULL;
	}
	fseek(f, 0, SEEK_END);
	zirdSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	unsigned char *zird = (unsigned char *) malloc(zirdSize);
	ird = new IRDFile;

	if (!zird || !ird) 
	{
		fprintf(stderr, "Could not allocate memory for IRD file\n");
		fclose(f);
		if (ird)
			free(ird);
		if (zird)
			free(zird);
		return NULL;
	}
	if (fread(zird, 1, zirdSize, f) != zirdSize)
	{
		fprintf(stderr, "Could not read IRD file\n");
		fclose(f);
		free(zird);
		free(ird);
		return NULL;
	}
	fclose(f);

	if (inflateIRD(zird, zirdSize, ird) != 0)
	{
		fprintf(stderr, "Invalid IRD file\n");
		free(zird);
		freeIRD(ird);
		return NULL;
	}

	free(zird);
	return ird;
}


void freeIRD(IRDFile *ird)
{
	if (ird->header.title)
		delete[] ird->header.title;
	if (ird->zFileHeader)
		delete[] ird->zFileHeader;
	if (ird->zFileFooter)
		delete[] ird->zFileFooter;
	if (ird->regionHashes)
		delete[] ird->regionHashes;
	if (ird->fileHashes)
		delete[] ird->fileHashes;
	if (ird->fileHeader)
		free(ird->fileHeader);
	if (ird->fileFooter)
		free(ird->fileFooter);
	delete ird;
}

void freeFileList(FileList *fileList)
{
	while (fileList)
	{
		FileList *nextFile = fileList->next;
		
		if (fileList->path)
			delete[] fileList->path;
		
		delete fileList;
		fileList = nextFile;
	}
}
