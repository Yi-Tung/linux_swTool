#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "swDaemon.h"


int be_swDaemon(void) {
  pid_t pid;
  int fd;

  pid = fork();
  if(pid == -1) {
    return -1;
  }
  else if(pid > 0) {
    exit(EXIT_SUCCESS);
  }

  pid = setsid();
  if(pid == -1) {
    return -1;
  }

  pid = fork();
  if(pid == -1) {
    return -1;
  }
  else if(pid > 0) {
    exit(EXIT_SUCCESS);
  }

  chdir("/");
  umask(0);

  fd = open("/dev/null", O_RDWR);
  if(fd == -1) {
    return -1;
  }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  dup2(fd, STDIN_FILENO);
  dup2(fd, STDOUT_FILENO);
  dup2(fd, STDERR_FILENO);
  close(fd);

  return 0;
}
