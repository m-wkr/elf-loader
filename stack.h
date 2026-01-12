#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/auxv.h>

uint64_t* allocateStack(Elf64_auxv_t* auxv,int argc, char** argv,char** envp);