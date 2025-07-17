#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "swCli.h"
#include "swLog.h"
#include "swDaemon.h"
#include "swNetwork.h"

#define SWTOOL_MAGIC_NUM "tung"
#define SWTOOL_VERSION "Version 1.1"
#define SWTOOL_LOG_FILE_NAME "./swTool.log"


int show_usage(const swCli_info_t my_cli_info) {
  printf("Usage: %s [option]\n%s\n"
         , my_cli_info.exec_name
         , "\noption:\n"
           "\t-h\tShow this message\n"
           "\t-v\tShow this tool version\n"
           "\t-n\tShow the information of a network interface\n"
           "\t-p\tAdd the special permission\n"
           "\t-d\tTurn into a daemon\n");
  return 0;
}

int show_version(const swCli_info_t my_cli_info) {
  printf("%s: %s\n", my_cli_info.exec_name, SWTOOL_VERSION);
  return 0;
}

int show_network_info(const swCli_info_t info) {
  char *iface_name = NULL;
  uint8_t mac[6] = {0};
  char ipv4[16] = {0};
  int status;
  int ret = 0;

  iface_name = info.action_args.argv[0];

  status = swNetwork_is_iface_up(iface_name);
  if(status == -1) {
    printf("[%s]: some error occurred to get the network status\n", info.exec_name);
    ret = -1;
  }
  else if(status == 0) {
    printf("[%s]: '%s' network interface is down\n", info.exec_name, iface_name);
  }
  else if(status == 1) {
    printf("[%s]: '%s' network interface is up\n", info.exec_name, iface_name);

    if(get_swNetwork_mac_addr(iface_name, mac) == 0) {
      printf("[%s]: Mac address of '%s' network interface is '%02x:%02x:%02x:%02x:%02x:%02x'\n", info.exec_name, iface_name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    else {
      printf("[%s]: some errors occurred to get the mac address\n", info.exec_name);
      ret = -1;
    }

    if(get_swNetwork_ipv4_addr(iface_name, ipv4, sizeof(ipv4)) == 0) {
      printf("[%s]: IPv4 address of '%s' network interface is '%s'\n", info.exec_name, iface_name, ipv4);
    }
    else {
      printf("[%s]: some errors occurred to get the IPv4 address\n", info.exec_name);
      ret = -1;
    }
  }

  return ret;
}

int main(int argc, char *argv[]) {

  const char *all_opt = "hvn:p:d";
  int opt_code;

  char *cp_argv0 = strdup(argv[0]);
  char *tool_name = basename(cp_argv0);

  swCli_info_t my_cli_info;
  char buf[512] = {0};
  int index;


  memset(&my_cli_info, 0, sizeof(swCli_info_t));
  swCli_init(&my_cli_info, tool_name, SWTOOL_MAGIC_NUM);

  free(cp_argv0);
  cp_argv0 = NULL;

  umask(0);

#if defined(log_switch) && log_switch
  if(access(SWTOOL_LOG_FILE_NAME, F_OK) == -1) {
    int fd = open(SWTOOL_LOG_FILE_NAME, O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if(fd != -1) {
      close(fd);
    }
  }

  set_swLog_level(SWLOG_LEVEL_INFO);
  set_swLog_output_fd(STDOUT_FILENO);
  enable_swLog_auto_lock_file_path(0);
  set_swLog_file_name(SWTOOL_LOG_FILE_NAME, sizeof(SWTOOL_LOG_FILE_NAME));
  set_swLog_lock_file_path(".", sizeof("."));
  set_swLog_store_switch(1);
  set_swLog_pr_switch(1);
#if defined(mode)
  if(!strcmp(mode, "debug")) {
    get_swLog_file_name(buf, sizeof(buf));
    pr_swLog(SWLOG_LEVEL_INFO, "%s: swLog File Name is %s", my_cli_info.exec_name, buf);

    get_swLog_lock_file_path(buf, sizeof(buf));
    pr_swLog(SWLOG_LEVEL_INFO, "%s: swLog Lock File Path is %s", my_cli_info.exec_name, buf);
  }
#endif

#endif

  while( (opt_code = getopt(argc, argv, all_opt)) != -1 ) {
    switch(opt_code) {
      case 'h':
        my_cli_info.property_flags |= SWCLI_PROPERTY_VALID;
        my_cli_info.action = show_usage;
        my_cli_info.action_args.argc = 0;
        my_cli_info.action_args.argv = NULL;
        break;
      case 'v':
        my_cli_info.property_flags |= SWCLI_PROPERTY_VALID;
        my_cli_info.action = show_version;
        my_cli_info.action_args.argc = 0;
        my_cli_info.action_args.argv = NULL;
        break;
      case 'p':
        if( (my_cli_info.magic_num != NULL)
            && (!strcmp(optarg, my_cli_info.magic_num)) ) {
          my_cli_info.property_flags |= SWCLI_PROPERTY_SPECIAL_PERMISSION;
        }
        else {
          my_cli_info.property_flags &= ~SWCLI_PROPERTY_VALID;
        }
        break;
      case 'd':
        my_cli_info.property_flags |= SWCLI_PROPERTY_DAEMON;
        enable_swDaemon_pid_file(1);
        set_swDaemon_pid_file_path("/tmp", sizeof("/tmp"));
#if defined(mode)
        if(!strcmp(mode, "debug")) {
          get_swDaemon_pid_file_path(buf, sizeof(buf));
          pr_swLog(SWLOG_LEVEL_INFO, "%s: PID File Path is %s", my_cli_info.exec_name, buf);
        }
#endif
        break;
      case 'n':
        my_cli_info.property_flags |= SWCLI_PROPERTY_VALID;
        my_cli_info.action = show_network_info;
        my_cli_info.action_args.argc = 1;
        my_cli_info.action_args.argv = malloc(my_cli_info.action_args.argc * sizeof(void*));
        my_cli_info.action_args.argv[0] = strdup(optarg);
        break;
      case '?':
      default:
        my_cli_info.property_flags &= ~SWCLI_PROPERTY_VALID;
        break;
    }
  }


  if(my_cli_info.property_flags & SWCLI_PROPERTY_VALID) {
    if(my_cli_info.property_flags & SWCLI_PROPERTY_DAEMON) {
      if(be_swDaemon(my_cli_info.exec_name) == -1) {
#if defined(log_switch) && log_switch
        pr_swLog(SWLOG_LEVEL_ERROR, "[%s]: Daemon is running or some errors have occurred... \n", my_cli_info.exec_name);
#endif
      }
      else if(chdir(my_cli_info.exec_pwd) == -1) {
#if defined(log_switch) && log_switch
        pr_swLog(SWLOG_LEVEL_WARNING , "[%s]: Daemon cannot change a directory... \n", my_cli_info.exec_name);
#endif
      }
    }

    if(my_cli_info.action != NULL) {
      (my_cli_info.action)(my_cli_info);
    }
  }


  swCli_destroy(&my_cli_info);
  return 0;
}
