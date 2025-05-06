#ifndef __C_SWLOG__
#define __C_SWLOG__

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


static _Atomic int g_swLog_output_fd = STDOUT_FILENO;
static swLog_level_t g_log_level = SWLOG_LEVEL_HIDE;
static char g_swLog_file_name[512] = "./swLog.log";
static _Atomic int g_swLog_store_switch = 0;
static _Atomic int g_swLog_pr_switch = 0;


static void _store_swLog(char *mMsg) {
  if(!access(g_swLog_file_name, F_OK | W_OK)) {
    int lock_fd = open(SWLOG_LOCK_FILE_NAME, O_CREAT | O_RDWR, 0666);
    if(lock_fd == -1) {
      fprintf(stderr, "%s[%d]: Failed to open swLog_lock_file", __func__, __LINE__);
      return;
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if(fcntl(lock_fd, F_SETLKW, &lock) == -1) {
      fprintf(stderr, "%s[%d]: Failed to lock swLog_lock", __func__, __LINE__);
      close(lock_fd);
      return;
    }

    int fd = open(g_swLog_file_name, O_WRONLY | O_APPEND | O_CLOEXEC, S_IWUSR | S_IWGRP);
    if(fd != -1) {
      dprintf(fd, "%s", mMsg);
      close(fd);
    }

    lock.l_type = F_UNLCK;
    if(fcntl(lock_fd, F_SETLK, &lock) == -1) {
      fprintf(stderr, "%s[%d]: Failed to unlock swLog_lock", __func__, __LINE__);
    }

    close(lock_fd);
  }
}

void pr_swLog(swLog_level_t mLevel, char *mMsg, ...) {
  static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
  static struct tm log_time_info;

  if( (g_log_level & mLevel) && (mMsg != NULL) ) {
    time_t log_time = time(NULL);
    char msg_buf[2048] = {0};
    char log_buf[4096] = {0};
    va_list ap;

    va_start(ap, mMsg);
    if(localtime_r(&log_time, &log_time_info) == NULL) {
      fprintf(stderr, "%s[%d]: Failed to translate local time", __func__, __LINE__);
      va_end(ap);
      return;
    }
    vsnprintf(msg_buf, sizeof(msg_buf), mMsg, ap);
    snprintf(log_buf, sizeof(log_buf), "[%04d-%02d-%02d %02d:%02d:%02d] %s\n"
      , log_time_info.tm_year + 1900
      , log_time_info.tm_mon  + 1
      , log_time_info.tm_mday
      , log_time_info.tm_hour
      , log_time_info.tm_min
      , log_time_info.tm_sec
      , msg_buf);
    va_end(ap);

    if(atomic_load(&g_swLog_pr_switch)) {
      pthread_mutex_lock(&lock);
      dprintf(get_swLog_output_fd(), "%s", log_buf);
      pthread_mutex_unlock(&lock);
    }

    if(atomic_load(&g_swLog_store_switch)) {
      _store_swLog(log_buf);
    }
  }
}

void set_swLog_level(swLog_level_t mLevel) {
  static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

  pthread_mutex_lock(&lock);
  switch(mLevel) {
    case SWLOG_LEVEL_HIDE:
    case SWLOG_LEVEL_ERROR:
    case SWLOG_LEVEL_WARNING:
    case SWLOG_LEVEL_INFO:
      g_log_level = mLevel;
      break;
    default:
      break;
  }
  pthread_mutex_unlock(&lock);
}

swLog_level_t get_swLog_level(void) {
  return g_log_level;
}

void set_swLog_file_name(char *mName, size_t mSize) {
  static const size_t len = sizeof(g_swLog_file_name);

  if(mName != NULL && mSize > 1) {
    if(!access(mName, F_OK | W_OK)) {
      snprintf(g_swLog_file_name, len, "%s", mName);
    }
  }
}

char* get_swLog_file_name(void) {
  return g_swLog_file_name;
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

#endif
