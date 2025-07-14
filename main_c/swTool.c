#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "swLog.h"
#include "swDaemon.h"
#include "swNetwork.h"

#define PROPERTY_VALID 0x01
#define PROPERTY_WRITE 0x02
#define PROPERTY_DAEMON 0x04


#define SWTOOL_MAGIC_NUM "tung"
#define SWTOOL_VERSION "Version 1.1"
#define SWTOOL_LOG_FILE_NAME "./swTool.log"


typedef struct cmd_args {
  int argc;
  void **argv;
} cmd_args_t;

typedef struct proc_info {
  pid_t exec_pid;
  char *exec_name;
  char *exec_pwd;

  int (*action) (const char*, const cmd_args_t);
  uint8_t property_flag;
  cmd_args_t action_args;
} proc_info_t;


int show_usage(const char *tool_name, const cmd_args_t args) {
  printf("Usage: %s [option]\n%s\n"
         , tool_name
         , "\noption:\n"
           "\t-h\tShow this message\n"
           "\t-v\tShow this tool version\n"
           "\t-n\tShow the information of a network interface\n"
           "\t-p\tAdd the special permission\n"
           "\t-d\tTurn into a daemon\n");
  return 0;
}

int show_version(const char *tool_name, const cmd_args_t args) {
  printf("%s: %s\n", tool_name, SWTOOL_VERSION);
  return 0;
}

int show_network_info(const char *tool_name, const cmd_args_t args) {
  char *iface_name = NULL;
  uint8_t mac[6] = {0};
  char ipv4[16] = {0};
  int status;
  int ret = 0;

  iface_name = args.argv[0];

  status = swNetwork_is_iface_up(iface_name);
  if(status == -1) {
    printf("[%s]: some error occurred to get the network status\n", tool_name);
    ret = -1;
  }
  else if(status == 0) {
    printf("[%s]: '%s' network interface is down\n", tool_name, iface_name);
  }
  else if(status == 1) {
    printf("[%s]: '%s' network interface is up\n", tool_name, iface_name);

    if(get_swNetwork_mac_addr(iface_name, mac) == 0) {
      printf("[%s]: Mac address of '%s' network interface is '%02x:%02x:%02x:%02x:%02x:%02x'\n", tool_name, iface_name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    else {
      printf("[%s]: some errors occurred to get the mac address\n", tool_name);
      ret = -1;
    }

    if(get_swNetwork_ipv4_addr(iface_name, ipv4, sizeof(ipv4)) == 0) {
      printf("[%s]: IPv4 address of '%s' network interface is '%s'\n", tool_name, iface_name, ipv4);
    }
    else {
      printf("[%s]: some errors occurred to get the IPv4 address\n", tool_name);
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

  proc_info_t *my_proc_info = malloc(sizeof(proc_info_t));
  char buf[512] = {0};
  int index;


  my_proc_info->exec_pid = getpid();
  my_proc_info->exec_name = strdup(tool_name);
  my_proc_info->exec_pwd = getcwd(NULL, 0);
  my_proc_info->property_flag = 0;
  my_proc_info->action_args.argc = 0;
  my_proc_info->action_args.argv = NULL;

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
    pr_swLog(SWLOG_LEVEL_INFO, "%s: swLog File Name is %s", my_proc_info->exec_name, buf);

    get_swLog_lock_file_path(buf, sizeof(buf));
    pr_swLog(SWLOG_LEVEL_INFO, "%s: swLog Lock File Path is %s", my_proc_info->exec_name, buf);
  }
#endif

#endif

  while( (opt_code = getopt(argc, argv, all_opt)) != -1 ) {
    switch(opt_code) {
      case 'h':
        my_proc_info->property_flag |= PROPERTY_VALID;
        my_proc_info->action = show_usage;
        my_proc_info->action_args.argc = 0;
        my_proc_info->action_args.argv = NULL;
        break;
      case 'v':
        my_proc_info->property_flag |= PROPERTY_VALID;
        my_proc_info->action = show_version;
        my_proc_info->action_args.argc = 0;
        my_proc_info->action_args.argv = NULL;
        break;
      case 'p':
        if(!strcmp(optarg, SWTOOL_MAGIC_NUM)) {
          my_proc_info->property_flag |= PROPERTY_WRITE;
        }
        else {
          my_proc_info->property_flag &= ~PROPERTY_VALID;
        }
        break;
      case 'd':
        my_proc_info->property_flag |= PROPERTY_DAEMON;
        enable_swDaemon_pid_file(1);
        set_swDaemon_pid_file_path("/tmp", sizeof("/tmp"));
#if defined(mode)
        if(!strcmp(mode, "debug")) {
          get_swDaemon_pid_file_path(buf, sizeof(buf));
          pr_swLog(SWLOG_LEVEL_INFO, "%s: PID File Path is %s", my_proc_info->exec_name, buf);
        }
#endif
        break;
      case 'n':
        my_proc_info->property_flag |= PROPERTY_VALID;
        my_proc_info->action = show_network_info;
        my_proc_info->action_args.argc = 1;
        my_proc_info->action_args.argv = malloc(my_proc_info->action_args.argc * sizeof(void*));
        my_proc_info->action_args.argv[0] = strdup(optarg);
        break;
      case '?':
      default:
        my_proc_info->property_flag &= ~PROPERTY_VALID;
        break;
    }
  }


  if(my_proc_info->property_flag & PROPERTY_VALID) {
    if(my_proc_info->property_flag & PROPERTY_DAEMON) {
      if(be_swDaemon(my_proc_info->exec_name) == -1) {
#if defined(log_switch) && log_switch
        pr_swLog(SWLOG_LEVEL_ERROR, "[%s]: Daemon is running or some errors have occurred... \n", my_proc_info->exec_name);
#endif
      }
      else if(chdir(my_proc_info->exec_pwd) == -1) {
#if defined(log_switch) && log_switch
        pr_swLog(SWLOG_LEVEL_WARNING , "[%s]: Daemon cannot change a directory... \n", my_proc_info->exec_name);
#endif
      }
    }

    if(my_proc_info->action != NULL) {
      (my_proc_info->action)(my_proc_info->exec_name, my_proc_info->action_args);
    }
  }


  if(my_proc_info->action_args.argv != NULL) {
    for(index=0; index<my_proc_info->action_args.argc; index++) {
      if(my_proc_info->action_args.argv[index] != NULL) {
        free(my_proc_info->action_args.argv[index]);
      }
    }
    free(my_proc_info->action_args.argv);
  }
  free(my_proc_info->exec_name);
  free(my_proc_info->exec_pwd);
  free(my_proc_info);
  return 0;
}
