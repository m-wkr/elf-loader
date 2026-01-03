#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/mman.h>
#include <string.h>

#define EI_NIDENT 16

#define ELFMAG0   0x7F
#define ELFMAG1   0x45
#define ELFMAG2   0x4c
#define ELFMAG3   0x46
#define ELFENDIAN 1   //little endian flag
#define ELFARCH64 2   //64 bit architecture flag

#define ELFMACHINE 0x3e //AMD x86-64

typedef uint64_t    Elf64_Addr;
typedef uint16_t    Elf64_Half;
typedef uint64_t    Elf64_Off;
//typedef uint32_t    Elf64_Sword;
typedef uint32_t    Elf64_Word;
typedef uint64_t    Elf64_Lword;

typedef struct {
  unsigned char e_ident[EI_NIDENT];
  Elf64_Half      e_type;
  Elf64_Half      e_machine;
  Elf64_Word      e_version;
  Elf64_Addr      e_entry;
  Elf64_Off       e_phoff;
  Elf64_Off       e_shoff;
  Elf64_Word      e_flags;
  Elf64_Half      e_ehsize;
  Elf64_Half      e_phentsize;
  Elf64_Half      e_phnum;
  Elf64_Half      e_shentsize;
  Elf64_Half      e_shnum;
  Elf64_Half      e_shstrndx;
} Elf64_Elf_Hdr;

enum ELF_IDEN {
  EI_MAG0 = 0,
  EI_MAG1,
  EI_MAG2,
  EI_MAG3,
  EI_ARCH,
  EI_ENDIAN,
  EI_VER,
  EI_OSABI,
  EI_ABIVER,
  EI_PAD
};


typedef struct {
  Elf64_Word      sh_name;

  Elf64_Word      sh_type;
  Elf64_Lword      sh_flags;
  Elf64_Addr      sh_addr;
  Elf64_Off       sh_offset;
  Elf64_Lword      sh_size;
  Elf64_Word      sh_link;
  Elf64_Word      sh_info;
  Elf64_Lword      sh_addralign;
  Elf64_Lword      sh_entsize;
} Elf64_Section_Hdr;

typedef struct {
  Elf64_Word      p_type;
  Elf64_Word      p_flags;
  Elf64_Off       p_offset;
  Elf64_Addr      p_vaddr;
  Elf64_Addr      p_paddr;
  Elf64_Lword      p_filesz;
  Elf64_Lword      p_memsz;
  Elf64_Lword      p_aligns;
} Elf64_Program_Hdr;

typedef enum error {
  NO_ERROR,
  FILE_ISSUE
};


void error_handler(const enum error error_code);

//write struct for errMsg, bool return type for now
bool validateElfHeader(const Elf64_Elf_Hdr *elf_hdr);

bool validateAllHdrSizes(const Elf64_Elf_Hdr *elf_hdr);

bool hasPHdr(const Elf64_Elf_Hdr *elf_hdr);

uint8_t getPHdrNum(const Elf64_Elf_Hdr *elf_hdr);

uint8_t getSHdrNum(const Elf64_Elf_Hdr *elf_hdr);