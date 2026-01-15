#include "elf.h"
#include "stack.h"

#include <unistd.h>
#include <string.h>


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

Elf64_Addr loadPhdr(const uint8_t phdrNumbers, FILE* fptr) {
  uint64_t totalMem = 0;

  Elf64_Program_Hdr segments[phdrNumbers];
  uint8_t segIndex = 0;

  Elf64_Addr startAddr = NULL;

  for (uint8_t i = 0; i < phdrNumbers; i++) {
    Elf64_Program_Hdr currentPHdr;

    fread(&currentPHdr,sizeof(currentPHdr),1,fptr);
    
    if (currentPHdr.p_type == 0x1) {
      printf("%lx\n wwwwww",currentPHdr.p_vaddr);
      void* addr = mmap(currentPHdr.p_vaddr,currentPHdr.p_memsz,getMemoryFlags(currentPHdr.p_flags),MAP_PRIVATE|MAP_FIXED,fileno(fptr),currentPHdr.p_offset);
      memset((void*)(addr + currentPHdr.p_filesz),0,currentPHdr.p_memsz - currentPHdr.p_filesz);

      if (startAddr == 0x0) {
        startAddr = addr;
      }
    }
  }

  return startAddr;
}

void findINTERPPath(char interpPath[],const uint8_t phdrNumbers, FILE* fptr) {

  for (uint8_t i = 0; i < phdrNumbers; i++) {
    Elf64_Program_Hdr currentPHdr;

    fread(&currentPHdr,sizeof(currentPHdr),1,fptr);

    if (currentPHdr.p_type == 0x3) {
      long savedPos = ftell(fptr);
      fseek(fptr, currentPHdr.p_offset, SEEK_SET);
      fread(interpPath,512,1,fptr);
      fseek(fptr,savedPos,SEEK_SET);
      break;
    } 
  }
}

Elf64_Addr loadDYN(const uint8_t phdrNumbers, FILE* fptr) {
  uint64_t totalMem = 0;

  Elf64_Program_Hdr segments[phdrNumbers];
  uint8_t segIndex = 0;

  Elf64_Addr startAddr = NULL;

  for (uint8_t i = 0; i < phdrNumbers; i++) {
    Elf64_Program_Hdr currentPHdr;

    fread(&currentPHdr,sizeof(currentPHdr),1,fptr);

    if (currentPHdr.p_type == 0x1) {

      segments[segIndex] = currentPHdr;
      segIndex++;

      totalMem += currentPHdr.p_memsz;
    }
  }

  startAddr = mmap(NULL,2*totalMem,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE,fileno(fptr),0);

  for (int i = 0; i < segIndex;i++) {
    void *dest = (char *)startAddr + segments[i].p_vaddr;
    fseek(fptr,segments[i].p_offset,SEEK_SET);
    fread(dest,segments[i].p_filesz,1,fptr);
    // Zero out BSS (p_memsz > p_filesz)
    memset((char *)dest + segments[i].p_filesz, 0, segments[i].p_memsz - segments[i].p_filesz);
  }

  return startAddr;
}

struct {
  Elf64_Ehdr e;
  Elf64_Addr p;
};

Elf64_Addr readInterp(const char* filename) {
  printf("%s\n",filename);
  FILE *fptr = fopen(filename,"rb");
  Elf64_Elf_Hdr ehdr;

  fread(&ehdr,sizeof(Elf64_Elf_Hdr),1,fptr); //add err check
  fseek(fptr, ehdr.e_phoff, SEEK_SET);
  bool success = validateElfHeader(&ehdr);
  bool sizeSuccess = validateAllHdrSizes(&ehdr);
  bool dyn = isDyn(&ehdr);

  uint8_t phdrNumbers = getPHdrNum(&ehdr);

  fseek(fptr,ehdr.e_phoff,SEEK_SET);
  printf("%s\n","erm1");
  Elf64_Addr startAddr = loadDYN(phdrNumbers,fptr);
  printf("%s\n","erm2");

  fclose(fptr);

  printf("%lx entry!!!!!!!\n",ehdr.e_entry);
  printf("%lx startAddr \n",startAddr);

  return startAddr;
}

void loadSeg(Elf64_Program_Hdr segments[],const uint8_t phdrNumbers, FILE* fptr) {
  uint8_t segIndex = 0;

  for (uint8_t i = 0; i < phdrNumbers; i++) {
    fread(segments+i,sizeof(Elf64_Program_Hdr),1,fptr);
  }
}

Elf64_Addr readBinary(const char* file_name, Elf64_Elf_Hdr* ehdr) {
  FILE *fptr = fopen(file_name,"rb");

  fread(ehdr,sizeof(Elf64_Elf_Hdr),1,fptr); //add err check
  fseek(fptr, ehdr->e_phoff, SEEK_SET);
  bool success = validateElfHeader(ehdr);
  bool sizeSuccess = validateAllHdrSizes(ehdr);
  bool dyn = isDyn(ehdr);

  uint8_t phdrNumbers = getPHdrNum(ehdr);

  Elf64_Addr startAddr = 0;
  
  if (dyn) {
    char interpPath[512] = {0};
    findINTERPPath(interpPath,phdrNumbers,fptr);
    startAddr = readInterp(interpPath);
  } else {
    startAddr = loadPhdr(phdrNumbers,fptr);
  }

  fclose(fptr);

  return startAddr;
}

void executeProgram(uint64_t* sp,uint64_t entryptr) {
  printf("rsp mod 16 = %lu\n", ((uintptr_t)sp%16));
  asm volatile(
    "mov %0, %%rsp;"
    "jmp *%1\n\t"
    :
    : "r"(sp), "r"(entryptr)
    : "memory"
  );

  __builtin_unreachable();
}

int main(int argc, char **argv, char** envp) {
  if (argc < 2) {
    return 0;
  }

  Elf64_Elf_Hdr elfHdrBuffer;

  Elf64_Addr phdr = readBinary(argv[1],&elfHdrBuffer);

  FILE* file = fopen(argv[1],"rb");
  Elf64_Program_Hdr segments[elfHdrBuffer.e_phnum];
  Elf64_Addr segphdr= mmap(NULL,elfHdrBuffer.e_phnum*sizeof(Elf64_Program_Hdr),PROT_READ,MAP_PRIVATE,fileno(file),elfHdrBuffer.e_phoff);
  fclose(file);


  uint64_t spVal;
  asm( "mov %%rsp, %0" : "=rm" ( spVal ));

  Elf64_auxv_t auxv[] = {
    {AT_NULL,0},
    {AT_PAGESZ,sysconf(_SC_PAGE_SIZE)},
    {AT_RANDOM,spVal},
    {AT_EXECFN,argv[1]},
    {AT_ENTRY,elfHdrBuffer.e_entry},
    {AT_UID,getuid()},
    {AT_EUID,geteuid()},
    {AT_GID,getgid()},
    {AT_EGID,getegid()},
    {AT_HWCAP,getauxval(AT_HWCAP)},
    {AT_HWCAP2,getauxval(AT_HWCAP2)},
    {AT_SECURE,0},
    {AT_MINSIGSTKSZ,0x4000},
    {AT_BASE,phdr},
    {AT_PHNUM,elfHdrBuffer.e_phnum},
    {AT_PHENT,elfHdrBuffer.e_phentsize},
    {AT_PHDR, segphdr},
  };

  //argv[0] = 
  uint64_t startUpValues[6] = {};
  uint64_t* sp = allocateStack(auxv,argc,argv,envp,startUpValues);

  printf("%lx sp\n",sp);
  printf("%lx entry\n",phdr+0x1f540);
  printf("%lx original entry offset\n",elfHdrBuffer.e_entry);
  //executeProgram(sp,elfHdrBuffer.e_entry); //correct for static
  executeProgram(sp,phdr+0x1f540);

  return 0;
}