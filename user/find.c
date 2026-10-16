#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "kernel/stat.h"
#include "user/user.h"

char filename[DIRSIZ];
int is_exec = 0;
int args_num = 0;
char *args[MAXARG];

void find_exec();

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
        if (is_exec) {
          if (args_num + 2 > MAXARG) {
            fprintf(2, "find-exec: args is too many");
            continue;
          }
          args[args_num] = buf;
          args[args_num + 1] = 0;
          find_exec();
        } else
          printf("%s\n", buf);
      }

      if (st.type == T_DIR)
        find(buf);
    }
  }
  close(fd);
}

void find_exec() {
  int pid;

  if ((pid = fork()) < 0) {
    fprintf(2, "find-exec: cant fork\n");
  }

  if (pid == 0) {
    exec(args[0], args);
    fprintf(2, "find-exec: exec %s failed\n", args[0]);
    exit(1);
  } else if (pid > 0) {
    wait((int *)0);
  }
}

int main(int argc, char *argv[]) {
  if (argc == 2) {
    strcpy(filename, argv[1]);
    find(".");
    exit(0);
  } else if (argc == 1) {
    fprintf(2, "usage: find [path] <filename> [-exec args]");
    exit(0);
  } else if (argc > 3) {
    if (strcmp("-exec", argv[3]) == 0) {
      is_exec = 1;
      args_num = argc - 4;

      for (int i = 0; i < args_num; i++)
        args[i] = argv[4 + i];
    }
  }
  strcpy(filename, argv[2]);
  find(argv[1]);
  exit(0);
}
