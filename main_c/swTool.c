#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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


typedef struct proc_info {
  pid_t exec_pid;
  char *exec_name;
  char *exec_pwd;

  int (*action) (const char*, const int, ...);
  uint8_t property_flag;
} proc_info_t;


int show_usage(const char *tool_name, const int argc, ...) {
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

int show_version(const char *tool_name, const int argc, ...) {
  printf("%s: %s\n", tool_name, SWTOOL_VERSION);
  return 0;
}

int show_network_info(const char *tool_name, const int argc, ...) {
  char *iface_name = NULL;
  uint8_t mac[6] = {0};
  char ipv4[16] = {0};
  int status;
  va_list ap;

  va_start(ap, argc);
  iface_name = va_arg(ap, char*);

  status = swNetwork_is_iface_up(iface_name);
  if(status == -1) {
    printf("[%s]: some error occurred to get the network status\n", tool_name);
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
    }

    if(get_swNetwork_ipv4_addr(iface_name, ipv4, sizeof(ipv4)) == 0) {
      printf("[%s]: IPv4 address of '%s' network interface is '%s'\n", tool_name, iface_name, ipv4);
    }
    else {
      printf("[%s]: some errors occurred to get the IPv4 address\n", tool_name);
    }
  }

  va_end(ap);
  return 0;
}

int main(int argc, char *argv[]) {

  const char *all_opt = "hvn:p:d";
  int opt_code;

  char *cp_argv0 = strdup(argv[0]);
  char *tool_name = basename(cp_argv0);

  proc_info_t *my_proc_info = malloc(sizeof(proc_info_t));
  char *iface_name = NULL;
  char buf[512] = {0};


  my_proc_info->exec_pid = getpid();
  my_proc_info->exec_name = strdup(tool_name);
  my_proc_info->exec_pwd = getcwd(NULL, 0);
  my_proc_info->property_flag = 0;

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
        break;
      case 'v':
        my_proc_info->property_flag |= PROPERTY_VALID;
        my_proc_info->action = show_version;
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
        iface_name = strdup(optarg);
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

    if(my_proc_info->action == NULL) {

    }
    else if( (my_proc_info->action == show_usage)
        || (my_proc_info->action == show_version) ) {
      (my_proc_info->action)(my_proc_info->exec_name, 0);
    }
    else if(my_proc_info->action == show_network_info) {
      (my_proc_info->action)(my_proc_info->exec_name, 1, iface_name);
    }
  }


  if(iface_name != NULL) {
    free(iface_name);
  }
  free(my_proc_info->exec_name);
  free(my_proc_info->exec_pwd);
  free(my_proc_info);
  return 0;
}
