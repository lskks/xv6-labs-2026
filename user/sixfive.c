#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "user/user.h"

#define MAXSIZE 128
char *delim = " -\r\t\n./,";

typedef enum status { A, B, C } status;

int isnum(char ch) { return ch <= '9' && ch >= '0'; }

int six_or_five(int num) { return num % 5 == 0 || num % 6 == 0; }

void six_five(int fd) {
  char ch[1];
  char digits[MAXSIZE];
  int len = 0;
  status s = A;

  while (read(fd, ch, 1) > 0) {
    // status A
    if (strchr(delim, *ch) != 0) {
      if (s == B) {
        digits[len] = '\0';
        int num = atoi(digits);
        if (six_or_five(num)) {
          printf("%d\n", num);
        }
      }
      len = 0;
      s = A;
    } else if (isnum(*ch)) {  // status B
      if (s == A) {
        digits[len++] = *ch;
        s = B;
      } else if (s == B) {
        digits[len++] = *ch;
      } else {
        continue;
      }
    } else { // status C
      s = C;
      len = 0;
      continue;
    }
  }

  // EOF
  if (s == B) {
    digits[len] = '\0';
    int num = atoi(digits);
    if (six_or_five(num)) {
      printf("%d\n", num);
    }
  }
}

int main(int argc, char *argv[]) {
  int fd;
  if (argc < 2) {
    fd = 0;
    six_five(fd);
  } else {
    for (int i = 1; i < argc; ++i) {
      if ((fd = open(argv[i], O_RDONLY)) < 0) {
        fprintf(2, "sixfive: cannot open %s\n", argv[i]);
        continue;
      }
      six_five(fd);
      close(fd);
    }
  }

  exit(0);
}