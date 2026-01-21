#include "stack.h"

long getPageSize() {
  return sysconf(_SC_PAGE_SIZE);
}

int countEnvp(char** envp) {
  int envc = 0;
  
  while(envp[envc]) envc++;

  return envc;
}

void getRandomVal(char* buffer, int buffSize) {
  int fd = open("/dev/urandom",O_RDONLY,0);
  read(fd,(unsigned char* )buffer,buffSize);
  close(fd);
}


void allocateStack(int argc, char** argv, char**envp,unsigned long* stackStore, char* stringStore, void* elfBuff,
  Elf64_Elf_Hdr* ehdr, size_t* elfBaseAddr, size_t* elfEntryAddr, size_t* interpBaseAddr, size_t* interpEntryAddr) {
  unsigned long* spArgc = stackStore;
  unsigned long* spArgv = &stackStore[1];
  int strLen;
  int strPtr = 0;
  int stackPtr = 1;
  int envc = countEnvp(envp);

  *spArgc = argc;

  for(int i = 0; i < argc; i++) {
    strLen =  strlen(argv[i]) + 1;

    memcpy(&stringStore[strPtr], argv[i], strLen);

    spArgv[i] = (unsigned long) &stringStore[strPtr];

    strPtr += strLen;
    stackPtr++;
  }

  stackStore[stackPtr++] = 0;

  unsigned long* spEnv = &stackStore[stackPtr];

  for(int i = 0; i < envc; i++) {
    strLen =  strlen(envp[i]) + 1;

    memcpy(&stringStore[strPtr], envp[i], strLen);

    spEnv[i] = (unsigned long) &stringStore[strPtr];

    strPtr += strLen;
    stackPtr++;
  }

  stackStore[stackPtr++] = 0;

  Elf64_Section_Hdr* init = getSection(".init", elfBuff);
  Elf64_Section_Hdr* initArr = getSection(".initArr", elfBuff);

  size_t base = 0;
  if(ehdr->e_type == ET_DYN) {
    base = *elfBaseAddr;
   }

  //printf("%ld",base);

  int (*ptr)(int,char**,char**);

  if(init) {
    ptr = (int (*)(int, char**, char**))base + init->sh_addr;
    ptr(argc, argv, envp);
   }


  if(initArr){
    for(int i = 0; i < initArr->sh_size / sizeof(void *); i++){
      ptr = (int (*)(int, char**, char**))base + *((long *)(base + initArr->sh_addr + (i * sizeof(void *))));
      ptr(argc, argv, envp);
    }
  }

  char randomBytes[16];

  getRandomVal(randomBytes,16);

  struct Elf64_auxv_t* auxv = (struct Elf64_auxv_t*) &stackStore[stackPtr];
  int auxc = 0;

  auxv[auxc].a_type = AT_PHDR;
  auxv[auxc++].a_val = (size_t)(*elfBaseAddr + ehdr->e_phoff);
  auxv[auxc].a_type = AT_PHENT;
  auxv[auxc++].a_val = sizeof(Elf64_Program_Hdr);
  auxv[auxc].a_type = AT_PHNUM;
  auxv[auxc++].a_val = ehdr->e_phnum;
  auxv[auxc].a_type = AT_PAGESZ;
  auxv[auxc++].a_val = getPageSize();
  // Interp base Address
  auxv[auxc].a_type = AT_BASE;
  auxv[auxc++].a_val = *interpBaseAddr;
  auxv[auxc].a_type = AT_FLAGS;
  auxv[auxc++].a_val = 0;
  auxv[auxc].a_type = AT_ENTRY;
  auxv[auxc++].a_val = *elfEntryAddr;
  auxv[auxc].a_type = AT_UID;
  auxv[auxc++].a_val = getuid();
  auxv[auxc].a_type = AT_EUID;
  auxv[auxc++].a_val = geteuid();
  auxv[auxc].a_type = AT_GID;
  auxv[auxc++].a_val = getgid();
  auxv[auxc].a_type = AT_EGID;
  auxv[auxc++].a_val = getegid();
  auxv[auxc].a_type = AT_RANDOM;
  auxv[auxc++].a_val = (size_t)randomBytes;
  auxv[auxc].a_type = AT_NULL;
  auxv[auxc++].a_val = 0;
}