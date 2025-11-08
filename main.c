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

int getBit(const enum endianness endian_type,const int offset,const int size,const unsigned char *value) {
  unsigned int binary_buffer = 0;

  if (endian_type == E_LOW) {

    printf("%x test\n",*value);

    for (int i = 0; i < size*8; i++) {

      int array_index = i/8;
      int bit_index = i%8;

      printf("%d ",array_index);
      printf("%d\n",bit_index);

      binary_buffer *= 2;
      binary_buffer |= (value[array_index] >> (8 - bit_index - 1)) & 1;

      printf("%x\n",value[array_index]);
      printf("%x\n",binary_buffer);
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

  fread(buffer,sizeof(buffer),1,fptr);

  int testNum = 0x457f464c;

  //printf("%x\n",testNum);
  printf("%x\n",getBit(E_LOW,0,4,buffer));

  fclose(fptr);
}



int main(int argc, char **argv) {
  if (argc > 1) {
    readBinary(argv[1]);
  }
  return 0;
}