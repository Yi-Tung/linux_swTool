#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>


#if defined(__linux__)
#include <linux/if.h>

#elif defined(__APPLE__)
#include <ifaddrs.h>
#include <net/if_dl.h>

#else
#endif


#include "swNetwork.h"


int swNetwork_is_iface_up(const char *mName) {
  if( (mName == NULL) || (strlen(mName) >= IFNAMSIZ) ) {
    return -1;
  }

  struct ifreq iface_req;
  int fd;

  strncpy(iface_req.ifr_name, mName, IFNAMSIZ - 1);

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

int get_swNetwork_mac_addr(const char *mName, uint8_t *mMac) {
  if( (mName == NULL) || (mMac == NULL) || (strlen(mName) >= IFNAMSIZ) ) {
    return -1;
  }

#if defined(__linux__)
  struct ifreq iface_req;
  int fd;

  strncpy(iface_req.ifr_name, mName, IFNAMSIZ - 1);

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if(fd == -1) {
    return -1;
  }

  if(ioctl(fd, SIOCGIFHWADDR, &iface_req) == -1) {
    close(fd);
    return -1;
  }
  else {
    close(fd);
  }

  memcpy(mMac, iface_req.ifr_hwaddr.sa_data, 6);
  return 0;

#elif defined(__APPLE__)
  struct ifaddrs *iface_addr_head, *iface_addr_ptr;
  struct sockaddr_dl *sa_dl;

  if(getifaddrs(&iface_addr_head) == -1) {
    return -1;
  }

  for(iface_addr_ptr = iface_addr_head; iface_addr_ptr != NULL; iface_addr_ptr = iface_addr_ptr->ifa_next) {
    if( (iface_addr_ptr->ifa_addr->sa_family  == AF_LINK)
      && (strcmp(iface_addr_ptr->ifa_name, mName) == 0) ) {
      sa_dl = (struct sockaddr_dl*)(iface_addr_ptr->ifa_addr);
      memcpy(mMac, LLADDR(sa_dl), 6);
      break;
    }
  }

  freeifaddrs(iface_addr_head);
  return 0;

#else
  return -1;

#endif
}

int get_swNetwork_ipv4_addr(const char *mName, char *mIp, size_t mSize) {
  if( (mName == NULL) || (mIp == NULL) || (mSize < 8) ) {
    return -1;
  }

#if defined(__linux__)
  struct sockaddr_in *sa_in;
  struct ifreq iface_req;
  int fd;

  strncpy(iface_req.ifr_name, mName, IFNAMSIZ - 1);

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if(fd == -1) {
    return -1;
  }

  if(ioctl(fd, SIOCGIFADDR, &iface_req) == -1) {
    close(fd);
    return -1;
  }
  else {
    close(fd);
  }

  sa_in = (struct sockaddr_in*)&iface_req.ifr_addr;
  inet_ntop(AF_INET, &(sa_in->sin_addr), mIp, mSize);
  return 0;

#elif defined(__APPLE__)
  struct ifaddrs *iface_addr_head, *iface_addr_ptr;
  struct sockaddr_in *sa_in;

  if(getifaddrs(&iface_addr_head) == -1) {
    return -1;
  }

  for(iface_addr_ptr = iface_addr_head; iface_addr_ptr != NULL; iface_addr_ptr = iface_addr_ptr->ifa_next) {
    if( (iface_addr_ptr->ifa_addr->sa_family  == AF_INET)
      && (strcmp(iface_addr_ptr->ifa_name, mName) == 0) ) {
      sa_in = (struct sockaddr_in*)(iface_addr_ptr->ifa_addr);
      inet_ntop(AF_INET, &(sa_in->sin_addr), mIp, mSize);
      break;
    }
  }

  freeifaddrs(iface_addr_head);
  return 0;

#else
  return -1;

#endif
}
