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

#define SWTOOL_VERSION "Version 1.0"
#define SWTOOL_LOG_FILE_NAME "./swTool.log"

void show_usage(const char *tool_name) {
  printf("Usage: %s [option]\n%s\n"
         , tool_name
         , "\noption:\n"
           "\t-h\tShow this message\n"
           "\t-v\tShow this tool version\n"
           "\t-p\tShow the parameter of this option\n"
           "\t-d\tExecute the daemon to do something\n"
           "\t-n\tShow the information of a network interface\n");
}

int main(int argc, char *argv[]) {

  char *cp_argv0 = strdup(argv[0]);
  char *cp_basename = basename(cp_argv0);
  char *tool_name = strdup(cp_basename);
  char *tool_pwd = getcwd(NULL, 0);

  const char *all_opt = "hvdp:n:";
  int opt_code;

  pid_t pid;
  int status;

  char *iface_name = NULL;
  uint8_t mac[6] = {0};
  char ipv4[16] = {0};


  free(cp_argv0);
  cp_argv0 = NULL;

#ifdef log_switch
  printf("[%s] mode=%s\n", tool_name, mode);
  printf("[%s] log_switch=%d\n", tool_name, log_switch);

#if log_switch

  char buf[512] = {0};

  umask(0);
  if(access(SWTOOL_LOG_FILE_NAME, F_OK) == -1) {
    int fd = open(SWTOOL_LOG_FILE_NAME, O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if(fd != -1) {
      close(fd);
    }
  }

  set_swLog_level(SWLOG_LEVEL_HIDE);
  set_swLog_output_fd(STDOUT_FILENO);
  enable_swLog_auto_lock_file_path(0);
  set_swLog_file_name(SWTOOL_LOG_FILE_NAME, sizeof(SWTOOL_LOG_FILE_NAME));
  set_swLog_lock_file_path(".", sizeof("."));
  set_swLog_store_switch(1);
  set_swLog_pr_switch(1);

  pr_swLog(SWLOG_LEVEL_HIDE, "%s: The message is never printed!", tool_name);
  set_swLog_store_switch(0);

  set_swLog_level(SWLOG_LEVEL_ERROR);
  pr_swLog(SWLOG_LEVEL_WARNING, "%s: The message is a warning!", tool_name);

  pr_swLog(SWLOG_LEVEL_INFO, "%s: Current swLog level is %d", tool_name, get_swLog_level());

  get_swLog_file_name(buf, sizeof(buf));
  pr_swLog(SWLOG_LEVEL_INFO, "%s: Log File Name is %s", tool_name, buf);
  pr_swLog(SWLOG_LEVEL_HIDE, "%s: The message is hidden", tool_name, buf);

  get_swLog_lock_file_path(buf, sizeof(buf));
  pr_swLog(SWLOG_LEVEL_INFO, "%s: Lock File Path is %s", tool_name, buf);
#endif

#endif


  opterr = 0;  //disable the default error message for getopt
  while( (opt_code = getopt(argc, argv, all_opt)) != -1 ) {
    switch(opt_code) {
      case 'h':
        show_usage(tool_name);
        break;
      case 'v':
        printf("%s: %s\n", tool_name, SWTOOL_VERSION);
        break;
      case 'p':
        printf("parameter: %s\n", optarg);
        break;
      case 'd':
        enable_swDaemon_pid_file(1);
        set_swDaemon_pid_file_path("/tmp", sizeof("/tmp"));
#ifdef log_switch
#if log_switch
        get_swDaemon_pid_file_path(buf, sizeof(buf));
        pr_swLog(SWLOG_LEVEL_INFO, "%s: PID File Path is %s", tool_name, buf);
#endif
#endif
        pid = fork();
        if(pid == -1) {
          printf("[%s]: Failed to create a child process \n", tool_name);
        }
        else if(pid > 0) {
          sleep(10);
          status = swDaemon_is_alive(tool_name);
          if(status == 1) {
            printf("[%s]: Nice! My child is still alive \n", tool_name);
          }
          else if(status == 0) {
            printf("[%s]: Oh! No... My child has passed way... \n", tool_name);
          }
          else {
            printf("[%s]: Some errors occurred in the function 'swDaemon_is_alive' \n", tool_name);
          }
        }
        else {
          if(be_swDaemon(tool_name) == -1) {
            printf("[%s]: Daemon is running or some errors have occurred... \n", tool_name);
            break;
          }
          else if(chdir(tool_pwd) == -1) {
            printf("[%s]: Daemon cannot change a directory... \n", tool_name);
            break;
          }

          for(int index=1; index<=60; index++) {
#ifdef log_switch
#if log_switch
            pr_swLog(SWLOG_LEVEL_INFO, "%s: write %dth log by the daemon", tool_name, index);
#endif
#endif
            sleep(1);
          }
        }
        break;
      case 'n':
        iface_name = strdup(optarg);
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
        free(iface_name);
        break;
      case '?':
        if(optopt=='p') {
          fprintf(stderr, "%s: the option 'p' need to a parameter\n", tool_name);
        }
        else {
          fprintf(stderr, "%s: not support the option \'-%c\'\n", tool_name, optopt);
        }
        break;
      default:
        break;
    }
  }

  free(tool_name);
  free(tool_pwd);
  return 0;
}
