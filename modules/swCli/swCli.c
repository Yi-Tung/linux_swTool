#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "swCli.h"


int swCli_init(swCli_info_t *mInfo, const char *mExec_name, const char *mMagic_num) {
  if(mInfo == NULL) {
    return -1;
  }

  if(mExec_name != NULL) {
    mInfo->exec_name = strdup(mExec_name);
  }
  else {
    mInfo->exec_name = NULL;
  }

  if(mMagic_num != NULL) {
    mInfo->magic_num = strdup(mMagic_num);
  }
  else {
    mInfo->magic_num = NULL;
  }

  mInfo->exec_pid = getpid();
  mInfo->exec_pwd = strdup(getcwd(NULL, 0));
  mInfo->property_flags = 0;
  mInfo->action_args.argc = 0;
  mInfo->action_args.argv = NULL;

  return 0;
}

int swCli_destroy(swCli_info_t *mInfo) {
  if(mInfo == NULL) {
    return -1;
  }

  if(mInfo->exec_name != NULL) {
    free(mInfo->exec_name);
    mInfo->exec_name = NULL;
  }

  if(mInfo->exec_pwd != NULL) {
    free(mInfo->exec_pwd);
    mInfo->exec_pwd = NULL;
  }

  if(mInfo->magic_num != NULL) {
    free(mInfo->magic_num);
    mInfo->magic_num = NULL;
  }

  mInfo->action = NULL;
  mInfo->action_args.argc = 0;

  if(mInfo->action_args.argv != NULL) {
    int index;

    for(index=0; index<mInfo->action_args.argc; index++) {
      if(mInfo->action_args.argv[index] != NULL) {
        free(mInfo->action_args.argv[index]);
        mInfo->action_args.argv[index] = NULL;
      }
    }

    free(mInfo->action_args.argv);
    mInfo->action_args.argv = NULL;
  }

  return 0;
}

int set_swCli_action_args(swCli_info_t *mInfo, const int mArgc, ...) {
  va_list ap;
  int index;
  char *tmp;

  if( (mInfo == NULL) || (mArgc < 0) ) {
    return -1;
  }

  if(mInfo->action_args.argv != NULL) {
    for(index=0; index<mInfo->action_args.argc; index++) {
      if(mInfo->action_args.argv[index] != NULL) {
        free(mInfo->action_args.argv[index]);
      }
    }
    free(mInfo->action_args.argv);
  }

  if(mArgc != 0) {
    mInfo->action_args.argv = malloc(mArgc * sizeof(char*));
    mInfo->action_args.argc = mArgc;

    va_start(ap, mArgc);
    for(index=0; index<mArgc; index++) {
      tmp = va_arg(ap, char*);

      if(tmp == NULL) {
        mInfo->action_args.argv[index] = NULL;
      }
      else {
        mInfo->action_args.argv[index] = strdup(tmp);
      }
    }
    va_end(ap);
  }
  else {
    mInfo->action_args.argv = NULL;
    mInfo->action_args.argc = 0;
  }

  return 0;
}
