#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "swLog.h"

#define SWTOOL_VERSION "Version 1.0"
#define SWTOOL_LOG_FILE_NAME "./swTool.log"

void show_usage(const char *tool_name) {
  printf("Usage: %s [option]\n%s\n"
         , tool_name
         , "\noption:\n"
           "\t-h\tShow this message\n"
           "\t-v\tShow this tool version\n"
           "\t-p\tShow the parameter of this option\n");
}

int main(int argc, char *argv[]) {

  const char *all_opt = "hvp:";
  const char *tool_name = basename(argv[0]);

  int opt_code;


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
  set_swLog_file_name(SWTOOL_LOG_FILE_NAME, sizeof(SWTOOL_LOG_FILE_NAME));
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

  return 0;
}
