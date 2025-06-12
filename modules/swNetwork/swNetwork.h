#ifndef __H_SWNETWORK__
#define __H_SWNETWORK__

#include <stdint.h>

int swNetwork_is_iface_up(const char*);

int get_swNetwork_mac_addr(const char*, uint8_t*);

#endif
