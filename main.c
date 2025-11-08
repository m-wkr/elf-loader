#include <stdio.h>

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

  printf("%x\n",getBit(E_LOW,0,4,buffer));

  fclose(fptr);
}



int main(int argc, char **argv) {
  if (argc > 1) {
    readBinary(argv[1]);
  }
  return 0;
}