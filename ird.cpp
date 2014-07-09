#include "ird.h"

#include "libird/ird.h"

ird::ird() {
  
}

ird::~ird() {
  
}

ird::loadIrdFile( const std::string& path ) {
  IRDFile *ird = readIRD( path.c_str() );
  FileList *list = buildFileListFromIRD("", ird);
  unsigned int *hash = (unsigned int *) getFileHash(ird, list);

  printf("Hash for file %s is : %X%X%X%X\n", list->path, hash[0], hash[1], hash[2], hash[3]);

  printf("\nPress ENTER to quit.");
  while (getchar() != '\n');
  return 0;
}
