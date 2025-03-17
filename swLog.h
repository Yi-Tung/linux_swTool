#ifndef __H_SWLOG__
#define __H_SWLOG__

typedef enum swLog_level {
  SWLOG_LEVEL_HIDE    = 0b00000000,
  SWLOG_LEVEL_ERROR   = 0b00000001,
  SWLOG_LEVEL_WARNING = 0b00000011,
  SWLOG_LEVEL_INFO    = 0b00000111
} swLog_level_t;

extern void _pr_swLog(swLog_level_t, char*, ...);
extern void _set_swLog_level(swLog_level_t);
extern swLog_level_t _get_swLog_level(void);
extern void _set_swLog_file_name(char*,size_t);
extern char* _get_swLog_file_name(void);
extern void _set_swLog_store_switch(int);
extern int _get_swLog_store_switch(void);
extern void _set_swLog_output_fd(int);
extern int _get_swLog_output_fd(void);
extern void _set_swLog_pr_switch(int);
extern int _get_swLog_pr_switch(void);

#define pr_swLog(mLevel, mMsg, ...) do{ _pr_swLog(mLevel, mMsg, __VA_ARGS__); }while(0)
#define set_swLog_level(mLevel) do{ _set_swLog_level(mLevel); }while(0)
#define get_swLog_level(void) _get_swLog_level(void)
#define set_swLog_file_name(mName, mSize) do{ _set_swLog_file_name(mName, mSize); }while(0)
#define get_swLog_file_name(void) _get_swLog_file_name(void)
#define set_swLog_store_switch(mSwitch) do{ _set_swLog_store_switch(mSwitch); }while(0)
#define get_swLog_store_switch(void) _get_swLog_store_switch(void)
#define set_swLog_output_fd(mFd) do{ _set_swLog_output_fd(mFd); }while(0)
#define get_swLog_output_fd(void) _get_swLog_output_fd(void)
#define set_swLog_pr_switch(mSwitch) do{ _set_swLog_pr_switch(mSwitch); }while(0)
#define get_swLog_pr_switch(void) _get_swLog_pr_switch(void)

#endif
