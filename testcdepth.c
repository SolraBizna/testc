#include <fcntl.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <stdio.h>

#define QUERY_STACK_DEPTH _IOR('S',0,int)

int main() {
  int fd_bitmask;
  int fd;
  char c;
  int result;
  fd = open("/dev/testc0", O_RDONLY);
  if(fd < 0) {
    perror("/dev/testc0");
    return 1;
  }
  if(ioctl(fd, QUERY_STACK_DEPTH, &result)) {
    perror("QUERY_STACK_DEPTH");
    return 1;
  }
  printf("%d\n", result);
  return 0;
}
