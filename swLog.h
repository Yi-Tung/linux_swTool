#ifndef __H_SWLOG__
#define __H_SWLOG__

typedef enum swLog_level {
  SWLOG_LEVEL_HIDE    = 0b00000000,
  SWLOG_LEVEL_ERROR   = 0b00000001,
  SWLOG_LEVEL_WARNING = 0b00000011,
  SWLOG_LEVEL_INFO    = 0b00000111
} swLog_level_t;


void pr_swLog(swLog_level_t, char*, ...);

void set_swLog_level(swLog_level_t);
swLog_level_t get_swLog_level(void);

void set_swLog_file_name(char*, size_t);
int get_swLog_file_name(char*, size_t);

void set_swLog_store_switch(int);
int get_swLog_store_switch(void);

void set_swLog_output_fd(int);
int get_swLog_output_fd(void);

void set_swLog_pr_switch(int);
int get_swLog_pr_switch(void);


#endif
