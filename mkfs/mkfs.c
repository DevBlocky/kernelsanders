#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  const char *filename = argc > 1 ? argv[1] : "fs.img";
  int fsfd = open(filename, O_CREAT | O_WRONLY, 0777);
  if (fsfd < 0)
    exit(-1);

  // just copy from stdin for now
  int srcfd = STDIN_FILENO;
  char buf[4096];
  ssize_t n;
  while ((n = read(srcfd, buf, sizeof(buf))) > 0)
    write(fsfd, buf, n);

  close(fsfd);
  return 0;
}
