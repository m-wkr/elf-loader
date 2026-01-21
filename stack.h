#include "elf.h"
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/auxv.h>

#include <unistd.h>
#include <fcntl.h>

long getPageSize();

long memCeil(long memSize);
long memFloor(long memSize);

void allocateStack(int argc, char** argv, char**envp,unsigned long* stackStore, char* stringStore, void* elfBuff,
  Elf64_Elf_Hdr* ehdr, size_t* elfBaseAddr, size_t* elfEntryAddr, size_t* interpBaseAddr, size_t* interpEntryAddr);