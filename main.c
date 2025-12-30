#include "elf.h"

#include <unistd.h>
#include <sys/mman.h>
#include <sys/auxv.h>
#include <string.h>


enum error {
  NO_ERROR,
  FILE_ISSUE
};

enum endianness {
  E_LOW,
  E_HIGH
};

void error_handler(const enum error error_code) {
  if (error_code != NO_ERROR) {
    printf("File could not be opened, please ensure that it exists\n");
  }
}

//write struct for errMsg, bool return type for now
bool validateElfHeader(const Elf64_Elf_Hdr *elf_hdr) {
  if (elf_hdr->e_ident[EI_MAG0] != ELFMAG0) {
    return false;
  }

  if (elf_hdr->e_ident[EI_MAG1] != ELFMAG1 || elf_hdr->e_ident[EI_MAG2] != ELFMAG2 || 
    elf_hdr->e_ident[EI_MAG3] != ELFMAG3) {
      return false;
  }

  if (elf_hdr->e_ident[EI_ARCH] != ELFARCH64) {
    return false;
  }

  if (elf_hdr->e_ident[EI_ENDIAN] != ELFENDIAN) {
    return false;
  }


  if (elf_hdr->e_machine != ELFMACHINE) {
    return false;
  }

  return true;
}

bool validateAllHdrSizes(const Elf64_Elf_Hdr *elf_hdr) {
  if (elf_hdr->e_ehsize != sizeof(Elf64_Elf_Hdr)) {
    return false;
  }

  if (elf_hdr->e_phentsize != sizeof(Elf64_Program_Hdr)) {
    return false;
  }

  if (elf_hdr->e_shentsize != sizeof(Elf64_Section_Hdr)) {
    return false;
  }

  return true;
}

bool hasPHdr(const Elf64_Elf_Hdr *elf_hdr) { //Wrapper function
  return (elf_hdr->e_phnum);
}

uint8_t getPHdrNum(const Elf64_Elf_Hdr *elf_hdr) {
  return elf_hdr->e_phnum;
}

uint8_t getSHdrNum(const Elf64_Elf_Hdr *elf_hdr) {
  return elf_hdr->e_shnum;
}

int getMemoryFlags(int elfFlags) {
  int flags = 0;

  if (elfFlags&1) {
    flags |= PROT_EXEC;
  }

  if (elfFlags&2) {
    flags |= PROT_WRITE;
  }

  if (elfFlags&4) {
    flags |= PROT_READ;
  }

  return flags;
}

Elf64_Addr loadPhdr(const uint8_t phdrNumbers, FILE* fptr) {
  Elf64_Addr phdrAddr = 0x0;

  for (uint8_t i = 0; i < phdrNumbers; i++) {
    Elf64_Program_Hdr currentPHdr;

    fread(&currentPHdr,sizeof(currentPHdr),1,fptr);

    if (currentPHdr.p_type == 0x1) {
      void* addr = mmap(currentPHdr.p_vaddr,currentPHdr.p_memsz,getMemoryFlags(currentPHdr.p_flags),MAP_PRIVATE|MAP_FIXED,fileno(fptr),currentPHdr.p_offset);
      memset((void*)(addr + currentPHdr.p_filesz),0,currentPHdr.p_memsz - currentPHdr.p_filesz);

      if (phdrAddr == 0x0) {
        phdrAddr = addr;
      } 
    }

  }

  return phdrAddr;
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

  uint8_t phdrNumbers = getPHdrNum(ehdr);

  Elf64_Addr phdrAddr = loadPhdr(phdrNumbers,fptr);

  fclose(fptr);

  return phdrAddr;
}

uint64_t allocateStack(Elf64_auxv_t* auxv,int argc, char** argv) {//char**envp) {
  int auxc = sizeof(auxv);

  const size_t STACK_SIZE = 1024*1024;
  void* stack_bottom = mmap(NULL,STACK_SIZE,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
  uint64_t* sp = stack_bottom+STACK_SIZE; //stack_top /bottom rename these, confusing

  sp-=2;

  sp[0] = AT_NULL; //thing
  sp[1] = 0;

  //arg strings

  //auxv
  for (int i = auxc-1; i >=0; i--) {
    sp--;
    sp[0] = auxv[i].a_type;
    sp[1] = auxv[i].a_un.a_val;
  }

  sp--;
  *sp = 0;

  //argv 
  sp--;
  *sp=0;

  for (int i = argc-1;i>=0;i--) {
    sp--;
    *sp = (uint64_t)argv[i];
  }

  //argc
  sp--;
  *sp = argc;

  return sp;
}

void executeProgram(uint64_t sp, uint64_t entryptr) {
  asm volatile(
    "mov %0, %%rsp;"
    :
    :"r" (sp)
    :
  );

  void (*entry)() = (void*)entryptr;
  entry();
}


int main(int argc, char **argv) {
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

  uint64_t sp = allocateStack(auxv,argc-1,argv+1);

  printf("%lx\n",sp);
  printf("%lx\n",phdr);
  executeProgram(sp,elfHdrBuffer.e_entry);

  return 0;
}