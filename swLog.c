#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

#include "swLog.h"

#define SWLOG_LOCK_FILE_NAME "./swLog.lock"


static pthread_rwlock_t g_log_level_lock = PTHREAD_RWLOCK_INITIALIZER;
static swLog_level_t g_log_level = SWLOG_LEVEL_HIDE;

static pthread_rwlock_t g_swLog_file_name_lock = PTHREAD_RWLOCK_INITIALIZER;
static char g_swLog_file_name[512] = "./swLog.log";
static _Atomic int g_swLog_store_switch = 0;

static _Atomic int g_swLog_output_fd = STDOUT_FILENO;
static _Atomic int g_swLog_pr_switch = 0;


static void _store_swLog(const char *mMsg) {
  static pthread_mutex_t inner_lock = PTHREAD_MUTEX_INITIALIZER;
  char log_file_name[512] = {0};

  pthread_rwlock_rdlock(&g_swLog_file_name_lock);
  snprintf(log_file_name, sizeof(log_file_name), "%s", g_swLog_file_name);
  pthread_rwlock_unlock(&g_swLog_file_name_lock);

  if(!access(log_file_name, F_OK | W_OK)) {
    int lock_fd = open(SWLOG_LOCK_FILE_NAME, O_CREAT | O_RDWR | O_CLOEXEC, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP);
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

  pthread_rwlock_rdlock(&g_log_level_lock);
  log_level = g_log_level;
  pthread_rwlock_unlock(&g_log_level_lock);

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
      write(output_fd, log_buf, strlen(log_buf));
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
      pthread_rwlock_wrlock(&g_log_level_lock);
      g_log_level = mLevel;
      pthread_rwlock_unlock(&g_log_level_lock);
      break;
    default:
      break;
  }
}

swLog_level_t get_swLog_level(void) {
  swLog_level_t log_level;

  pthread_rwlock_rdlock(&g_log_level_lock);
  log_level = g_log_level;
  pthread_rwlock_unlock(&g_log_level_lock);

  return log_level;
}

void set_swLog_file_name(char *mName, size_t mSize) {
  pthread_rwlock_wrlock(&g_swLog_file_name_lock);

  static const size_t len = sizeof(g_swLog_file_name);
  if(mName != NULL && mSize > 1) {
    if(!access(mName, F_OK | W_OK)) {
      snprintf(g_swLog_file_name, len, "%s", mName);
    }
  }

  pthread_rwlock_unlock(&g_swLog_file_name_lock);
}

int get_swLog_file_name(char *mName, size_t mSize) {
  pthread_rwlock_rdlock(&g_swLog_file_name_lock);

  size_t len = strlen(g_swLog_file_name) + 1;
  int ret = 0;
  if(mSize > len) {
    snprintf(mName, mSize, "%s", g_swLog_file_name);
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

