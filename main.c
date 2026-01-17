#include "elf.h"
#include "stack.h"

#include <unistd.h>
#include <string.h>

long getPageSize() {
  return sysconf(_SC_PAGE_SIZE);
}

int getMemoryFlags(int elfFlags) {
  int flags = 0;

  if (elfFlags&PF_X) {
    flags |= PROT_EXEC;
  }

  if (elfFlags&PF_W) {
    flags |= PROT_WRITE;
  }

  if (elfFlags&PF_R) {
    flags |= PROT_READ;
  }

  return flags;
}

char* findINTERPPath(char* buffer) {
  Elf64_Elf_Hdr *ehdr = (Elf64_Elf_Hdr*) buffer;
  Elf64_Program_Hdr *phdr = (Elf64_Program_Hdr*)(buffer+ehdr->e_phoff);

  for (int i = 0; i < ehdr->e_phnum; i++) {
    if (phdr[i].p_type == PT_INTERP) {
      return buffer + phdr[i].p_offset;
    }
  }

  return NULL;
}

void elfLoad(char* elfStartPtr, void* stackPtr, int stackSize, size_t* baseAddr, size_t* entryPtr) {
  int elfProt = 0;
  int stackProt = 0;
  size_t base;

  Elf64_Elf_Hdr* ehdr = (Elf64_Elf_Hdr*)elfStartPtr;
  Elf64_Program_Hdr* phdr = (Elf64_Program_Hdr*)((char*)elfStartPtr+ehdr->e_phoff);

  if (ehdr->e_type == ET_DYN) {
    base = (size_t)mmap(NULL,getPageSize(),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    munmap((void*)base,getPageSize());
  } else {
    base = 0;
  }

  if (baseAddr != NULL) *baseAddr = -1;
  if (entryPtr != NULL) *entryPtr = base + ehdr->e_entry;


  for (int i = 0; i < ehdr->e_phnum; i++) {
    if(phdr[i].p_type == PT_GNU_STACK && stackPtr != NULL) {
      stackProt = getMemoryFlags(phdr[i].p_flags);

      mprotect((unsigned char*) stackPtr, stackSize, stackProt);
    }

    //skip non loadable segments and empty segments
    if (phdr[i].p_type != PT_LOAD) continue;
    if(!phdr[i].p_filesz) continue;



    void* mapStart = (void*) phdr[i].p_vaddr;
      int size = (void*) phdr[i].p_vaddr - mapStart;
      int mapSize = phdr[i].p_memsz+size;

      mmap(base+mapStart,mapSize,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON|MAP_FIXED,-1,0);
      memcpy((void*) base + phdr[i].p_vaddr,elfStartPtr+phdr[i].p_offset,phdr[i].p_filesz);

      if (phdr[i].p_memsz > phdr[i].p_filesz) {
        memset((void*)(base+phdr[i].p_vaddr+phdr[i].p_filesz),0,phdr[i].p_memsz - phdr[i].p_filesz);
      }

      elfProt = getMemoryFlags(phdr[i].p_flags);
      mprotect((unsigned char *) (base + mapStart), mapSize, elfProt);

      if(baseAddr != NULL && (*baseAddr == -1 || *baseAddr > (size_t)(base + mapStart))) {
        *baseAddr = (size_t)(base + mapStart);
      }
    }
}

void executeProgram(void* stackPtr,size_t* entryPtr) {
  printf("rsp mod 16 = %lu\n", ((uintptr_t)stackPtr%16));
  asm volatile(
    "mov %0, %%rsp;"
    "jmp *%1\n\t"
    :
    : "r"(stackPtr), "r"(entryPtr)
    : "memory"
  );

  __builtin_unreachable();
}

int main(int argc, char **argv, char** envp) {
  if (argc < 2) {
    return 0;
  }

  
  FILE* fptr = fopen(argv[1],"rb");
  fseek(fptr,0,SEEK_END);

  int buffSize = ftell(fptr);
  char* elfBuff = malloc(buffSize);

  fseek(fptr,0,SEEK_SET);
  fread(elfBuff,buffSize,1,fptr);

  return 0;
}