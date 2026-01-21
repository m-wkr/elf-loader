#include "elf.h"

#define ET_EXEC 0x2
#define ET_DYN 0x3

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

Elf64_Section_Hdr* getSection(char* sectionName, void *elfStartPtr) {
  Elf64_Elf_Hdr* ehdr = (Elf64_Elf_Hdr*)elfStartPtr;
  Elf64_Section_Hdr* shdr = (Elf64_Section_Hdr*)(elfStartPtr + ehdr->e_shoff);

  Elf64_Section_Hdr* shdr_strtab = &shdr[ehdr->e_shstrndx];
  char* sh_strtab_p = elfStartPtr + shdr_strtab->sh_offset;

  for (int i = 0; i < ehdr->e_shnum; i++) {
    if (!strcmp(sectionName,sh_strtab_p+shdr[i].sh_name)) {
      return &shdr[i];
    }
  }

  return NULL;
}