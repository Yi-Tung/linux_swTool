#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/stat.h>

#include "swDaemon.h"


static pthread_rwlock_t g_swDaemon_pid_file_lock = PTHREAD_RWLOCK_INITIALIZER;

static char g_swDaemon_pid_file_name[512] = "swDaemon.pid";
static char g_swDaemon_pid_file_path[512] = ".";
static _Atomic int g_swDaemon_pid_file_switch = 1;
static int g_swDaemon_pid_fd = -1;


static int _write_swDaemon_pid_file(const char *mName) {
  char pid_num[32] = {0};
  struct flock lock;

  lock.l_pid = getpid();
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;

  g_swDaemon_pid_fd = open(mName, O_CREAT | O_RDWR | O_TRUNC | O_CLOEXEC, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP);
  if(g_swDaemon_pid_fd == -1) {
    return -1;
  }
  else if(fcntl(g_swDaemon_pid_fd, F_SETLK, &lock) == -1) {
    close(g_swDaemon_pid_fd);
    g_swDaemon_pid_fd = -1;
    return -1;
  }

  snprintf(pid_num, sizeof(pid_num), "%d\n", getpid());

  if(write(g_swDaemon_pid_fd, pid_num, strlen(pid_num)) == -1) {
    close(g_swDaemon_pid_fd);
    g_swDaemon_pid_fd = -1;
    return -1;
  }

  return 0;
}

static void _swDaemon_cleanup(void) {
  char pid_file[512];

  if(g_swDaemon_pid_fd != -1) {
    close(g_swDaemon_pid_fd);
  }
  snprintf(pid_file, sizeof(pid_file), "%s/%s", g_swDaemon_pid_file_path, g_swDaemon_pid_file_name);
  unlink(pid_file);
}

int be_swDaemon(const char *mName) {
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

  if(atomic_load(&g_swDaemon_pid_file_switch)) {
    char pid_file[512] = {0};

    pthread_rwlock_wrlock(&g_swDaemon_pid_file_lock);
    snprintf(g_swDaemon_pid_file_name, sizeof(g_swDaemon_pid_file_name), "%s.pid", mName);
    snprintf(pid_file, sizeof(pid_file), "%s/%s", g_swDaemon_pid_file_path, g_swDaemon_pid_file_name);
    pthread_rwlock_unlock(&g_swDaemon_pid_file_lock);

    if(_write_swDaemon_pid_file(pid_file) == -1) {
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
  atomic_store(&g_swDaemon_pid_file_switch, mSwitch>0?1:0);
}

int get_swDaemon_pid_file_path(char *mPath, size_t mSize) {
  pthread_rwlock_rdlock(&g_swDaemon_pid_file_lock);

  size_t len = strlen(g_swDaemon_pid_file_path) + 1;
  int ret = -1;

  if(mSize > len) {
    snprintf(mPath, mSize, "%s", g_swDaemon_pid_file_path);
    ret = 0;
  }

  pthread_rwlock_unlock(&g_swDaemon_pid_file_lock);
  return ret;
}

int set_swDaemon_pid_file_path(const char *mPath, size_t mSize) {
  if( (mPath == NULL) || (mSize <= 0) || (mSize >= sizeof(g_swDaemon_pid_file_path)) ) {
    return -1;
  }

  char *cp_mPath = strdup(mPath);
  int last_index = strlen(cp_mPath) - 1;
  int fd = -1;

  if(cp_mPath[last_index] == '/') {
    cp_mPath[last_index] = '\0';
  }

  fd = open(cp_mPath, O_CLOEXEC | O_DIRECTORY, S_IXUSR | S_IWUSR | S_IXGRP | S_IWGRP);
  if(fd != -1) {
    close(fd);
  }
  else {
    free(cp_mPath);
    return -1;
  }

  pthread_rwlock_wrlock(&g_swDaemon_pid_file_lock);
  snprintf(g_swDaemon_pid_file_path, sizeof(g_swDaemon_pid_file_path), "%s", cp_mPath);
  pthread_rwlock_unlock(&g_swDaemon_pid_file_lock);

  free(cp_mPath);
  return 0;
}

int swDaemon_is_alive(const char *mName) {
  char pid_file[512];
  char pid_buf[32];
  FILE *fp = NULL;
  pid_t pid;

  pthread_rwlock_rdlock(&g_swDaemon_pid_file_lock);
  snprintf(pid_file, sizeof(pid_file), "%s/%s.pid", g_swDaemon_pid_file_path, mName);
  pthread_rwlock_unlock(&g_swDaemon_pid_file_lock);

  fp = fopen(pid_file, "r");
  if(fp == NULL) {
    return 0;
  }

  if(fgets(pid_buf, sizeof(pid_buf), fp) == NULL) {
    fclose(fp);
    return -1;
  }
  else {
    fclose(fp);
  }

  pid = strtol(pid_buf, NULL, 10);
  if(pid <= 1) {
    return -1;
  }

  if(kill(pid, 0) == -1) {
    if(errno == EPERM) {
      return 1;
    }
    else if(errno == ESRCH) {
      return 0;
    }
    else {
      return -1;
    }
  }

  return 1;
}
