#include "stack.h"

static long STACK_SIZE = 8*1024*1024;
static long STACK_STORE_SIZE = 0x5000;
static long STACK_STR_SIZE = 0x5000;


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

void exitWrapper() {
  exit(0);
}

char* findINTERPPath(char* buffer) {
  Elf64_Elf_Hdr* ehdr = (Elf64_Elf_Hdr*) buffer;
  Elf64_Program_Hdr* phdr = (Elf64_Program_Hdr*)(buffer+ehdr->e_phoff);

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
    base = (size_t)mmap(NULL,128*getPageSize(),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    munmap((void*)base,128*getPageSize());
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



    void* mapStart = (void*) memFloor(phdr[i].p_vaddr);
      int size = (void*) phdr[i].p_vaddr - mapStart;
      int mapSize = memCeil(phdr[i].p_memsz+size);

      mmap(base+mapStart,mapSize,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON|MAP_FIXED,-1,0);
      memcpy((void*) base + phdr[i].p_vaddr,elfStartPtr+phdr[i].p_offset,phdr[i].p_filesz);

      if (phdr[i].p_memsz > phdr[i].p_filesz) {
        memset((void*)(base+phdr[i].p_vaddr+phdr[i].p_filesz),0,phdr[i].p_memsz - phdr[i].p_filesz);
      }

      elfProt = getMemoryFlags(phdr[i].p_flags);
      mprotect((unsigned char*) (base + mapStart), mapSize, elfProt);

      if(baseAddr != NULL && (*baseAddr == -1 || *baseAddr > (size_t)(base + mapStart))) {
        *baseAddr = (size_t)(base + mapStart);
      }
    }
}


void executeProgram(void* stackPtr,void* entryPtr,void* exitFunc) {
  //printf("rsp mod 16 = %lu\n", ((uintptr_t)stackPtr%16));

  register long rsp __asm__("rsp") = (long) stackPtr;
	register long rdx __asm__("rdx") = (long) exitFunc;

	__asm__ __volatile__(
		"jmp *%0\n"
		:
		: "r" (entryPtr), "r" (rsp), "r" (rdx)
		:
	);

  __builtin_unreachable();
}

int main(int argc, char** argv, char** envp) {
  if (argc < 2) {
    return 0;
  }

  FILE* fptr = fopen(argv[1],"rb");
  fseek(fptr,0,SEEK_END);

  int buffSize = ftell(fptr);
  char* elfBuff = malloc(buffSize);

  fseek(fptr,0,SEEK_SET);
  fread(elfBuff,buffSize,1,fptr);

  Elf64_Elf_Hdr* ehdr = (Elf64_Elf_Hdr*)elfBuff;



  bool valid = validateElfHeader(ehdr) && validateAllHdrSizes(ehdr);
  if (!valid) return 1;


  size_t elfBaseAddr, elfEntryAddr;
  size_t interpBaseAddr = 0;
  size_t interpEntryAddr = 0;
  int strLen;
  int strPtr = 0;
  int stackPtr = 1;
  int counter = 0;

  void* stack = mmap(0, STACK_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANON, -1, 0);
  elfLoad(elfBuff, stack, STACK_SIZE, &elfBaseAddr, &elfEntryAddr);

  char* interpName = findINTERPPath(elfBuff);

  if(interpName) {
    int f = open(interpName, O_RDONLY, 0);
    int size = lseek(f, 0, SEEK_END);

    lseek(f, 0, SEEK_SET);
    void* elfLoader = mmap(0, memCeil(size), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);

    read(f, elfLoader, size);
    elfLoad(elfLoader, stack, STACK_SIZE, &interpBaseAddr, &interpEntryAddr);
    munmap(elfLoader, memCeil(size));
  }

  memset(stack, 0, STACK_STORE_SIZE);

  unsigned long* stackStorage = stack + STACK_SIZE - STACK_STORE_SIZE - STACK_STR_SIZE;
  char* string_storage =  stack + STACK_SIZE - STACK_STR_SIZE;

  allocateStack(argc,argv,envp,
    stackStorage,string_storage,
    elfBuff,ehdr,&elfBaseAddr,&elfEntryAddr,
    &interpBaseAddr,&interpEntryAddr);


  if (interpEntryAddr) {
    executeProgram(stackStorage,interpEntryAddr,exitWrapper);
  } else {
    executeProgram(stackStorage,elfEntryAddr,exitWrapper);
  }

  return 0;
}