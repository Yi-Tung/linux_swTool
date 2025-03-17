#ifndef __C_SWLOG__
#define __C_SWLOG__

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

#include "swLog.h"


static time_t g_log_time;

static int g_swLog_output_fd = STDOUT_FILENO;
static swLog_level_t g_log_level = SWLOG_LEVEL_HIDE;
static char g_swLog_file_name[512] = "./swLog.log";
static int g_swLog_store_switch = 0;
static int g_swLog_pr_switch = 0;


static void _store_swLog(char *mMsg, va_list mAp) {
  if(!access(g_swLog_file_name, F_OK | W_OK)) {
    int fd = open(g_swLog_file_name, O_WRONLY | O_APPEND | O_CLOEXEC | O_EXLOCK, S_IWUSR | S_IWGRP);
    if(fd != -1) {
      vdprintf(fd, mMsg, mAp);
      close(fd);
    }
  }
}

void _pr_swLog(swLog_level_t mLevel, char *mMsg, ...) {
  if(g_log_level & mLevel) {
    char buf[512] = {0};
    va_list ap;

    va_start(ap, mMsg);
    g_log_time = time(NULL);
    snprintf(buf, sizeof(buf), "[%s] %s\n", strtok(ctime(&g_log_time),"\n"), mMsg);

    if(g_swLog_pr_switch) {
      vdprintf(g_swLog_output_fd, buf, ap);
    }

    if(g_swLog_store_switch) {
      _store_swLog(buf, ap);
    }
    va_end(ap);
  }
}

void _set_swLog_level(swLog_level_t mLevel) {
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
}

swLog_level_t _get_swLog_level(void) {
  return g_log_level;
}

void _set_swLog_file_name(char *mName, size_t mSize) {
  if(mName != NULL && mSize > 1) {
    if(!access(mName, F_OK | W_OK)) {
      strncpy(g_swLog_file_name, mName,
        sizeof(g_swLog_file_name)<=mSize?sizeof(g_swLog_file_name):mSize);
    }
  }
}

char* _get_swLog_file_name(void) {
  return g_swLog_file_name;
}

void _set_swLog_store_switch(int mSwitch) {
  g_swLog_store_switch = (mSwitch>0?1:0);
}

int _get_swLog_store_switch(void) {
  return g_swLog_store_switch;
}

void _set_swLog_output_fd(int mFd) {
  if(fcntl(mFd, F_GETFD) != -1) {
    g_swLog_output_fd = mFd;
  }
}

int _get_swLog_output_fd(void) {
  return g_swLog_output_fd;
}

void _set_swLog_pr_switch(int mSwitch) {
  g_swLog_pr_switch = (mSwitch>0?1:0);
}

int _get_swLog_pr_switch(void) {
  return g_swLog_pr_switch;
}

#endif
