/*
 * Copyright 2014: Cobra Team
 *
 * This software is released under the GPLv3 license
 */
#include <stdio.h>
#include "ird.h"

int main(int argc, char *argv[])
{
	IRDFile *ird = readIRD(argv[1]);
	FileList *list = buildFileListFromIRD("", ird);
	unsigned int *hash = (unsigned int *) getFileHash(ird, list);

	printf("Hash for file %s is : %X%X%X%X\n", list->path, hash[0], hash[1], hash[2], hash[3]);

	printf("\nPress ENTER to quit.");
	while (getchar() != '\n');
	return 0;
}

