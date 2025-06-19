#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <libgen.h>

#include "swLog.h"


static pthread_rwlock_t g_swLog_log_level_lock = PTHREAD_RWLOCK_INITIALIZER;
static swLog_level_t g_swLog_log_level = SWLOG_LEVEL_HIDE;

static pthread_rwlock_t g_swLog_file_name_lock = PTHREAD_RWLOCK_INITIALIZER;
static char g_swLog_lock_file_path[SW_FILE_PATH_MAX_LEN] = ".";
static char g_swLog_lock_file_name[SW_FILE_NAME_MAX_LEN] = ".swLog.log.lock";
static char g_swLog_log_file_name[SW_FILE_PATH_NAME_MAX_LEN] = "./swLog.log";
static _Atomic int g_swLog_auto_lock_file_path_switch = 1;
static _Atomic int g_swLog_store_switch = 0;

static _Atomic int g_swLog_output_fd = STDOUT_FILENO;
static _Atomic int g_swLog_pr_switch = 0;


static void _store_swLog(const char *mMsg) {
  static pthread_mutex_t inner_lock = PTHREAD_MUTEX_INITIALIZER;
  char lock_file_name[SW_FILE_PATH_NAME_MAX_LEN] = {0};
  char log_file_name[SW_FILE_PATH_NAME_MAX_LEN] = {0};

  pthread_rwlock_rdlock(&g_swLog_file_name_lock);
  snprintf(lock_file_name, sizeof(lock_file_name), "%s/%s", g_swLog_lock_file_path, g_swLog_lock_file_name);
  snprintf(log_file_name, sizeof(log_file_name), "%s", g_swLog_log_file_name);
  pthread_rwlock_unlock(&g_swLog_file_name_lock);

  if(!access(log_file_name, F_OK | W_OK)) {
    int lock_fd = open(lock_file_name, O_CREAT | O_RDWR | O_CLOEXEC, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP);
    if(lock_fd == -1) {
      fprintf(stderr, "%s[%d]: Failed to open swLog_lock_file", __func__, __LINE__);
      return;
    }

    struct flock outer_lock;
    outer_lock.l_type = F_WRLCK;
    outer_lock.l_whence = SEEK_SET;
    outer_lock.l_start = 0;
    outer_lock.l_len = 0;
    if(fcntl(lock_fd, F_SETLKW, &outer_lock) == -1) {
      fprintf(stderr, "%s[%d]: Failed to lock swLog_lock", __func__, __LINE__);
      close(lock_fd);
      return;
    }

    pthread_mutex_lock(&inner_lock);
    int fd = open(log_file_name, O_WRONLY | O_APPEND | O_CLOEXEC, S_IWUSR | S_IWGRP);
    if(fd != -1) {
      dprintf(fd, "%s", mMsg);
      close(fd);
    }
    pthread_mutex_unlock(&inner_lock);

    outer_lock.l_type = F_UNLCK;
    if(fcntl(lock_fd, F_SETLK, &outer_lock) == -1) {
      fprintf(stderr, "%s[%d]: Failed to unlock swLog_lock", __func__, __LINE__);
    }

    close(lock_fd);
  }
}

void pr_swLog(swLog_level_t mLevel, char *mMsg, ...) {
  static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
  swLog_level_t log_level;

  pthread_rwlock_rdlock(&g_swLog_log_level_lock);
  log_level = g_swLog_log_level;
  pthread_rwlock_unlock(&g_swLog_log_level_lock);

  if( (log_level & mLevel) && (mMsg != NULL) ) {
    int output_fd = atomic_load(&g_swLog_output_fd);
    time_t log_time = time(NULL);
    struct tm log_time_info;
    char msg_buf[2048] = {0};
    char log_buf[4096] = {0};
    va_list ap;

    va_start(ap, mMsg);
    vsnprintf(msg_buf, sizeof(msg_buf), mMsg, ap);
    va_end(ap);

    if(localtime_r(&log_time, &log_time_info) == NULL) {
      fprintf(stderr, "%s[%d]: Failed to translate local time", __func__, __LINE__);
      return;
    }

    snprintf(log_buf, sizeof(log_buf), "[%04d-%02d-%02d %02d:%02d:%02d] %s\n"
      , log_time_info.tm_year + 1900
      , log_time_info.tm_mon  + 1
      , log_time_info.tm_mday
      , log_time_info.tm_hour
      , log_time_info.tm_min
      , log_time_info.tm_sec
      , msg_buf);

    if(atomic_load(&g_swLog_pr_switch)) {
      pthread_mutex_lock(&lock);
      if(write(output_fd, log_buf, strlen(log_buf)) == -1) {
        fprintf(stderr, "%s[%d]: Failed to write a log to the output", __func__, __LINE__);
        return;
      }
      pthread_mutex_unlock(&lock);
    }

    if(atomic_load(&g_swLog_store_switch)) {
      _store_swLog(log_buf);
    }
  }
}

void set_swLog_level(swLog_level_t mLevel) {
  switch(mLevel) {
    case SWLOG_LEVEL_HIDE:
    case SWLOG_LEVEL_ERROR:
    case SWLOG_LEVEL_WARNING:
    case SWLOG_LEVEL_INFO:
      pthread_rwlock_wrlock(&g_swLog_log_level_lock);
      g_swLog_log_level = mLevel;
      pthread_rwlock_unlock(&g_swLog_log_level_lock);
      break;
    default:
      break;
  }
}

swLog_level_t get_swLog_level(void) {
  swLog_level_t log_level;

  pthread_rwlock_rdlock(&g_swLog_log_level_lock);
  log_level = g_swLog_log_level;
  pthread_rwlock_unlock(&g_swLog_log_level_lock);

  return log_level;
}

void set_swLog_file_name(char *mName, size_t mSize) {
  pthread_rwlock_wrlock(&g_swLog_file_name_lock);

  static const size_t lock_path_len = sizeof(g_swLog_lock_file_path);
  static const size_t lock_len = sizeof(g_swLog_lock_file_name);
  static const size_t log_len = sizeof(g_swLog_log_file_name);
  char *cp_mName, *dir_name, *base_name, *tmp;

  if( (mName != NULL) && (mSize > 1) && (mSize <= SW_FILE_NAME_MAX_LEN) ) {
    if(!access(mName, F_OK | W_OK)) {
      cp_mName = strdup(mName);
      tmp = dirname(cp_mName);
      dir_name = strdup(tmp);
      free(cp_mName);

      cp_mName = strdup(mName);
      tmp = basename(cp_mName);
      base_name = strdup(tmp);
      free(cp_mName);

      if(atomic_load(&g_swLog_auto_lock_file_path_switch)) {
        snprintf(g_swLog_lock_file_path, lock_path_len, "%s", dir_name);
      }
      snprintf(g_swLog_lock_file_name, lock_len, ".%s.lock", base_name);
      snprintf(g_swLog_log_file_name, log_len, "%s", mName);
      free(dir_name);
      free(base_name);
    }
  }

  pthread_rwlock_unlock(&g_swLog_file_name_lock);
}

int get_swLog_file_name(char *mName, size_t mSize) {
  pthread_rwlock_rdlock(&g_swLog_file_name_lock);

  size_t len = strlen(g_swLog_log_file_name) + 1;
  int ret = 0;
  if(mSize >= len) {
    snprintf(mName, mSize, "%s", g_swLog_log_file_name);
    ret = 1;
  }

  pthread_rwlock_unlock(&g_swLog_file_name_lock);
  return ret;
}

void set_swLog_store_switch(int mSwitch) {
  atomic_store(&g_swLog_store_switch, mSwitch>0?1:0);
}

int get_swLog_store_switch(void) {
  return atomic_load(&g_swLog_store_switch);
}

void set_swLog_output_fd(int mFd) {
  if(fcntl(mFd, F_GETFD) != -1) {
    atomic_store(&g_swLog_output_fd, mFd);
  }
}

int get_swLog_output_fd(void) {
  return atomic_load(&g_swLog_output_fd);
}

void set_swLog_pr_switch(int mSwitch) {
  atomic_store(&g_swLog_pr_switch, mSwitch>0?1:0);
}

int get_swLog_pr_switch(void) {
  return atomic_load(&g_swLog_pr_switch);
}

void enable_swLog_auto_lock_file_path(int mSwitch) {
  atomic_store(&g_swLog_auto_lock_file_path_switch, mSwitch>0?1:0);
}

void set_swLog_lock_file_path(char *mPath, size_t mSize) {
  if( (mPath == NULL) || (mSize < 1) ||  (mSize >= SW_FILE_PATH_MAX_LEN) ) {
    return;
  }
  pthread_rwlock_wrlock(&g_swLog_file_name_lock);

  char *cp_mPath = strdup(mPath);
  int last_index = strlen(cp_mPath) - 1;
  int fd = -1;

  if( cp_mPath[last_index] == '/' ) {
    cp_mPath[last_index] = '\0';
  }

  fd = open(cp_mPath, O_CLOEXEC | O_DIRECTORY, S_IXUSR | S_IWUSR | S_IXGRP | S_IWGRP);
  if(fd != -1) {
    snprintf(g_swLog_lock_file_path, SW_FILE_PATH_MAX_LEN, "%s", cp_mPath);
    close(fd);
  }

  free(cp_mPath);
  pthread_rwlock_unlock(&g_swLog_file_name_lock);
}

int get_swLog_lock_file_path(char *mPath, size_t mSize) {
  pthread_rwlock_rdlock(&g_swLog_file_name_lock);

  size_t len = strlen(g_swLog_lock_file_path) + 1;
  int ret = 0;
  if(mSize >= len) {
    snprintf(mPath, mSize, "%s", g_swLog_lock_file_path);
    ret = 1;
  }

  pthread_rwlock_unlock(&g_swLog_file_name_lock);
  return ret;
}
