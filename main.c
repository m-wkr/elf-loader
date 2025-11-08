#include <stdio.h>

enum error {
  NO_ERROR,
  FILE_ISSUE
};

void error_handler(const enum error error_code) {
  if (error_code != NO_ERROR) {
    printf("File could not be opened, please ensure that it exists\n");
  }
}

void getBit(const int offset) {
}


enum error readBinary(const char* file_name) {
  FILE *fptr = fopen(file_name,"rb");

  int buffer[1024];

  fread(buffer,sizeof(buffer),1,fptr);

  for (int i = 0; i < 1024; i++) {
    printf("%x ",buffer[i]);
  }

  fclose(fptr);
}



int main(int argc, char **argv) {
  if (argc > 1) {
    printf("%s\n",argv[1]);

    readBinary(argv[1]);
  }
  return 0;
}