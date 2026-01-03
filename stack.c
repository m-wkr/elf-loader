#include "stack.h"

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