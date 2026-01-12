#include "elf.h"
#include "stack.h"

#include <unistd.h>
#include <string.h>


int getMemoryFlags(int elfFlags) {
  int flags = 0;

  if (elfFlags&PF_R) {
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

Elf64_Addr loadPhdr(const bool isDynamic, const uint8_t phdrNumbers, FILE* fptr) {
  uint64_t totalMem = 0;

  Elf64_Program_Hdr segments[phdrNumbers];
  uint8_t segIndex = 0;

  Elf64_Addr startAddr = NULL;

  for (uint8_t i = 0; i < phdrNumbers; i++) {
    Elf64_Program_Hdr currentPHdr;

    fread(&currentPHdr,sizeof(currentPHdr),1,fptr);

    if (currentPHdr.p_type == 0x1) {

      if (!isDynamic) {
        void* addr = mmap(currentPHdr.p_vaddr,currentPHdr.p_memsz,getMemoryFlags(currentPHdr.p_flags),MAP_PRIVATE|MAP_FIXED,fileno(fptr),currentPHdr.p_offset);
        memset((void*)(addr + currentPHdr.p_filesz),0,currentPHdr.p_memsz - currentPHdr.p_filesz);

      if (startAddr == 0x0) {
        startAddr = addr;
      }

      }

      segments[segIndex] = currentPHdr;
      segIndex++;

      totalMem += currentPHdr.p_memsz;
    }

    if (isDynamic) {
      startAddr = mmap(NULL,totalMem,0,MAP_PRIVATE,fileno(fptr),0);
      int success = munmap(startAddr,totalMem);
      
      for (uint8_t i = 0; i < segIndex;i++) {
        mmap(startAddr+segments[i].p_vaddr,segments[i].p_memsz,getMemoryFlags(segments[i].p_flags),MAP_PRIVATE|MAP_FIXED,fileno(fptr),segments[i].p_offset);
      }
    } 

  }

  return startAddr;
}

struct {
  Elf64_Ehdr e;
  Elf64_Addr p;
};

Elf64_Addr readBinary(const char* file_name, Elf64_Elf_Hdr* ehdr) {
  FILE *fptr = fopen(file_name,"rb");
  //Elf64_Elf_Hdr elfIdentification;

  fread(ehdr,sizeof(Elf64_Elf_Hdr),1,fptr); //add err check

  bool success = validateElfHeader(ehdr);
  bool sizeSuccess = validateAllHdrSizes(ehdr);
  bool dyn = isDyn(ehdr);

  uint8_t phdrNumbers = getPHdrNum(ehdr);

  Elf64_Addr phdrAddr = loadPhdr(dyn,phdrNumbers,fptr);

  fclose(fptr);

  return phdrAddr;
}

void executeProgram(uint64_t* sp, uint64_t entryptr) {
  void (*entry)() = (void*)entryptr;
  asm volatile(
    "mov %0, %%rsp;"
    :
    :"r" (sp)
    :
  );
  entry();
}

int main(int argc, char **argv, char** envp) {
  if (argc < 2) {
    return 0;
  }

  Elf64_Elf_Hdr elfHdrBuffer;

  Elf64_Addr phdr = readBinary(argv[1],&elfHdrBuffer);

  Elf64_auxv_t auxv[] = {
    {AT_PAGESZ,sysconf(_SC_PAGE_SIZE)},
    {AT_PHDR, phdr},
    {AT_PHENT,elfHdrBuffer.e_phentsize},
    {AT_PHNUM,elfHdrBuffer.e_phnum},
    {AT_ENTRY,elfHdrBuffer.e_entry},
    {AT_HWCAP,getauxval(AT_HWCAP)},
    {AT_HWCAP2,getauxval(AT_HWCAP2)},
    {AT_SYSINFO_EHDR,getauxval(AT_SYSINFO_EHDR)},
    {AT_NULL,0}
  };

  uint64_t* sp = allocateStack(auxv,argc-1,argv+1,envp);

  printf("%lx\n",sp);
  printf("%lx\n",phdr);
  executeProgram(sp,elfHdrBuffer.e_entry);

  return 0;
}