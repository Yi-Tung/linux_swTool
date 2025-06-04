#ifndef __H_SWDAEMON__
#define __H_SWDAEMON__

int be_swDaemon(const char*);

void enable_swDaemon_pid_file(int);

int get_swDaemon_pid_file_path(char*, size_t);
int set_swDaemon_pid_file_path(const char*, size_t);

#endif
