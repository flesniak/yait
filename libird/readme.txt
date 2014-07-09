IRD parsing library released by the Cobra Team.
This library is released under GPLv3 license.

You should read the code to understand how it works.
The library depends on zlib.

API:
readIRD to parse the IRD file
freeIRD when done
buildFileListFromIRD to parse the iso inside the IRD
getFileHash to get the hash of a file, use FileList as iterator
freeFileList when done
