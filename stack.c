#include "stack.h"

uint64_t* allocateStack(Elf64_auxv_t* auxv, int argc, char** argv, char**envp) {
  const size_t STACK_SIZE = 1024*1024*4;
  void* stack_bottom = mmap(NULL,STACK_SIZE,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
  uint64_t* sp = stack_bottom+STACK_SIZE; //stack_top /bottom rename these, confusing

  uint64_t args[1024]; //hold ptrs to the strings
  uint64_t envs[1024];
  int argsIndex = 0;
  int envsIndex = 0;

  for (char** env = envp; *env != 0; env++) {
    int envSize = strlen(*env)+1; //get string length
    sp-=envSize; //move sp down for strncpy
    strcpy(sp,*env); //copy string into mem
    envs[envsIndex] = sp; //record ptr location
    envsIndex++;
  }

  for (int i = 0; i < argc;i++) {
    int envSize = strlen(argv[i])+1; //get string length
    sp-=envSize; //move sp down for strncpy
    strcpy(sp,argv[i]); //copy string into mem
    args[argsIndex] = sp; //record ptr location
    argsIndex++;
  }


  sp-=2;

  sp[0] = AT_NULL; //thing
  sp[1] = 0;


  //Get aux size
  int auxc = 0;
  Elf64_auxv_t* p = auxv;
  while (p->a_type != AT_NULL) {
    auxc++;
    p++;
  }

  for (int i = auxc-1; i >=0; i--) {
    sp-=2;
    sp[0] = auxv[i].a_type;
    sp[1] = auxv[i].a_un.a_val;
  }

  sp--;
  *sp = 0;

  for (int i = envsIndex-1; i >= 0;i--) {
    sp--;
    sp[0] = envs[i];
  }

  //argv 
  sp--;
  *sp=0;

  for (int i = argc-1;i>=0;i--) {
    sp--;
    sp[0] = args[i];
  }

  //argc
  sp--;
  *sp = argc;

  return sp;
}