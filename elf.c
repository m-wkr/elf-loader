#include "elf.h"

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