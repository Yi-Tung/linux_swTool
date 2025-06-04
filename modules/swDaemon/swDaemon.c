#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <sys/stat.h>

#include "swDaemon.h"

#define SWDAEMON_PID_FILE_NAME "/tmp/swDaemon.pid"


static _Atomic int g_pid_file_switch = 1;
static int g_pid_fd = -1;


static int _write_swDaemon_pid_file(const char *mName) {
  char pid_num[32] = {0};
  struct flock lock;

  lock.l_pid = getpid();
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;

  g_pid_fd = open(mName, O_CREAT | O_RDWR | O_TRUNC | O_CLOEXEC, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP);
  if(g_pid_fd == -1) {
    return -1;
  }
  else if(fcntl(g_pid_fd, F_SETLK, &lock) == -1) {
    close(g_pid_fd);
    g_pid_fd = -1;
    return -1;
  }

  snprintf(pid_num, sizeof(pid_num), "%d\n", getpid());

  if(write(g_pid_fd, pid_num, strlen(pid_num)) == -1) {
    close(g_pid_fd);
    g_pid_fd = -1;
    return -1;
  }

  return 0;
}

static void _swDaemon_cleanup(void) {
  if(g_pid_fd != -1) {
    close(g_pid_fd);
  }
  unlink(SWDAEMON_PID_FILE_NAME);
}

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

  if(atomic_load(&g_pid_file_switch)) {
    if(_write_swDaemon_pid_file(SWDAEMON_PID_FILE_NAME) == -1) {
      return -1;
    }
  }

  if(atexit(_swDaemon_cleanup) == -1) {
    return -1;
  }

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

void enable_swDaemon_pid_file(int mSwitch) {
  atomic_store(&g_pid_file_switch, mSwitch>0?1:0);
}
