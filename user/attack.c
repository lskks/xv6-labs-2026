#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"


#define DATASIZE (40 * 4096)
#define STR "Here it is: "


int
main(int argc, char *argv[])
{
  // Your code here.
  int len = strlen(STR);
  char* data = sbrk(DATASIZE);
  for (char* i = data; i < data + DATASIZE - len;++i) {
    char ch = *(i + len);
    *(i + len) = '\0';
    if (strcmp(i, STR) == 0){
      *(i + len) = ch;
      i += len;
      printf("%s\n", i);
      exit(0);
    }
    *(i + len) = ch;
  }
  exit(1);
}
