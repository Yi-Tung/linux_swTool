#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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

  mInfo->action = NULL;

  if(mInfo->magic_num != NULL) {
    free(mInfo->magic_num);
    mInfo->magic_num = NULL;
  }

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
