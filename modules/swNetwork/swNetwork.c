#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "swNetwork.h"


int swNetwork_is_iface_up(const char *mName) {
  if(mName == NULL) {
    return -1;
  }

  struct ifreq iface_req;
  int fd;

  snprintf(iface_req.ifr_name, IFNAMSIZ, "%s", mName);

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if(fd == -1) {
    return -1;
  }

  if(ioctl(fd, SIOCGIFFLAGS, &iface_req) == -1) {
    close(fd);
    return -1;
  }
  else {
    close(fd);
  }

  if(iface_req.ifr_flags & IFF_UP) {
    return 1;
  }
  else {
    return 0;
  }
}
