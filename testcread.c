#include <fcntl.h>
#include <sys/time.h> /* for select() ?! */
#include <sys/errno.h>
#include <stdio.h>

int main() {
  int fd_bitmask;
  int fd;
  char c;
  fd = open("/dev/testc0", O_RDONLY);
  if(fd < 0) {
    perror("/dev/testc0");
    return 1;
  }
  if(fd > 30) {
    fprintf(stderr, "File descriptor way too big for our simple hack\n");
    return 1;
  }
  while(1) {
    fd_bitmask = 1 << fd; // :|
    select(fd+1, &fd_bitmask, 0, 0, 0);
    if(fd_bitmask & (1 << fd)) {
      if(read(fd, &c, 1) != 1) {
        perror("read");
        break;
      }
      putchar(c);
      fflush(stdout);
    }
  }
  return 0;
}
