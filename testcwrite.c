#include <fcntl.h>
#include <sys/time.h> /* for select() ?! */
#include <sys/errno.h>
#include <stdio.h>

int main() {
  int fd_bitmask;
  int fd;
  char c;
  int result;
  fd = open("/dev/testc0", O_WRONLY);
  if(fd < 0) {
    perror("/dev/testc0");
    return 1;
  }
  if(fd > 30) {
    fprintf(stderr, "File descriptor way too big for our simple hack\n");
    return 1;
  }
  while(1) {
    result = read(0, &c, 1);
    if(result < 1) {
      if(result == 0) break;
      else {
        perror("stdin");
        return 1;
      }
    }
    fd_bitmask = 1 << fd; // :|
    select(fd+1, 0, &fd_bitmask, 0, 0);
    if(fd_bitmask & (1 << fd)) {
      if(write(fd, &c, 1) != 1) {
        perror("write");
        break;
      }
      // Echo the outputted character
      putchar(c);
      fflush(stdout);
    }
  }
  return 0;
}
