#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "user/user.h"

char filename[DIRSIZ];

void find(char *path) {
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  if (st.type == T_DIR) {
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
      fprintf(2, "find: path too long\n");
      close(fd);
      return;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;

      if (stat(buf, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", buf);
        continue;
      }

      if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;

      if (strcmp(de.name, filename) == 0) {
        printf("%s\n", buf);
      }

      if (st.type == T_DIR)
        find(buf);
    }
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc == 2) {
    strcpy(filename, argv[1]);
    find(".");
    exit(0);
  } else if (argc == 1) {
    fprintf(2, "usage: find [path] <filename>");
    exit(0);
  }

  strcpy(filename, argv[2]);
  find(argv[1]);
  exit(0);
}
