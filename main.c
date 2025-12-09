#include <stdio.h>
#include <stdint.h>

#define EI_NIDENT 16

typedef uint64_t    Elf64_Addr;
typedef uint16_t    Elf64_Half;
typedef uint64_t    Elf64_Off;
typedef uint32_t    Elf64_Sword;
typedef uint32_t    Elf64_Word;


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


typedef struct {
  Elf64_Word      sh_name;

  Elf64_Word      sh_type;
  Elf64_Word      sh_flags;
  Elf64_Addr      sh_addr;
  Elf64_Off       sh_offset;
  Elf64_Word      sh_size;
  Elf64_Word      sh_link;
  Elf64_Word      sh_info;
  Elf64_Word      sh_addralign;
  Elf64_Word      sh_entsize;
} Elf64_Section_Hdr;

typedef struct {
  Elf64_Word      p_type;
  Elf64_Off       p_offset;
  Elf64_Addr      p_vaddr;
  Elf64_Addr      p_paddr;
  Elf64_Word      p_filesz;
  Elf64_Word      p_memsz;
  Elf64_Word      p_flags;
  Elf64_Word      p_aligns;
} Elf64_Program_Hdr;

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

// offset is a BYTE offset, size is of a BYTE value, and value is the file buffer
unsigned int getBit(const enum endianness endian_type,const int offset,const int size,const unsigned char *value) {
  unsigned int binary_buffer = 0;

  if (endian_type == E_LOW) {

    for (int i = 0; i < size*8; i++) {

      int array_index = i/8;
      int bit_index = i%8;

      //printf("%d ",array_index);
      //printf("%d\n",bit_index);

      binary_buffer *= 2;
      binary_buffer |= (value[offset+array_index] >> (8 - bit_index - 1)) & 1;

      //printf("%x\n",value[offset+array_index]);
      //printf("%x\n",binary_buffer);
    }

  } else {

    for (int i = 0; i < 64; i++) {
      binary_buffer *=2;
      binary_buffer |= (*value >> i) & 1;
    }

  }

  return binary_buffer;
}


enum error readBinary(const char* file_name) {
  FILE *fptr = fopen(file_name,"rb");

  unsigned char buffer[1024];

  fread(buffer,1,1024,fptr);

  printf("%x\n",getBit(E_LOW,18,8,buffer));

  fclose(fptr);
}



int main(int argc, char **argv) {
  if (argc > 1) {
    readBinary(argv[1]);
  }
  return 0;
}