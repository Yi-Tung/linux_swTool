#ifndef __H_SWCLI__
#define __H_SWCLI__

#include <stdint.h>

#define SWCLI_PROPERTY_VALID 0x01
#define SWCLI_PROPERTY_SPECIAL_PERMISSION 0x02
#define SWCLI_PROPERTY_DAEMON 0x04

typedef struct swCli_cmd_args {
  int argc;
  void **argv;
} swCli_cmd_args_t;

typedef struct swCli_info {
  pid_t exec_pid;
  char *exec_name;
  char *exec_pwd;

  int (*action) (const struct swCli_info);
  swCli_cmd_args_t action_args;
  uint8_t property_flags;
  char *magic_num;
} swCli_info_t;


int swCli_init(swCli_info_t*, const char*, const char*);
int swCli_destroy(swCli_info_t*);

#endif
