#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/uio.h>
#include <sys/user.h>

#define MAX_STACK_DEPTH 16
#define QUERY_STACK_DEPTH _IOR('S',0,int)

static char stack[MAX_STACK_DEPTH];
static int stack_depth;

/* Used to wake up someone doing a read-oriented select on us */
static struct proc* rsel_wake_proc;
/* Used to wake up someone doing a write-oriented select on us */
static struct proc* wsel_wake_proc;

/* Outputs a character to the "debug port" of a QFB card installed in slot C */
static void outc(c) char c; {
  *(unsigned int*)0xFC00003C = c;
}

static void outhex(d) {
  if(d >= 0 && d <= 9) outc('0' + d);
  else outc('A' + d - 10);
}

static void outs(s) char* s; {
  while(*s) outc(*s++);
}

static void outptr(p) char* p; {
  unsigned int i = (unsigned int)p;
  int shift;
  for(shift = 28; shift >= 0; shift -= 4) {
    outhex((i >> shift) & 15);
  }
}

static void outi(i) unsigned int i; {
  char buf[10];
  int digits;
  if(i & 0x80000000) {
    outc('-');
    i = ~i + 1;
  }
  digits = 0;
  do {
    buf[digits++] = '0' + (i % 10);
    i /= 10;
  } while(i);
  while(--digits >= 0) {
    outc(buf[digits]);
  }
}

int testc_init() {
  outs("testc_init\n");
  stack_depth = 3;
  stack[0] = '6';
  stack[1] = '5';
  stack[2] = '4';
  return 0;
}

int testc_open(dev) dev_t dev; {
  outs("testc_open(");
  outi(dev);
  outs(")\n");
  if(minor(dev) != 0) return EINVAL;
  return 0;
}

int testc_close(dev) dev_t dev; {
  outs("testc_close(");
  outi(dev);
  outs(")\n");
  return 0;
}

int testc_read(dev, uio) dev_t dev; struct uio* uio; {
  int ret;
  outs("testc_read(");
  outi(dev);
  outs(")\n");
  if(stack_depth > 0) {
    ret = uiomove(stack + --stack_depth, 1, UIO_READ, uio);
    if(ret == 0) {
      if(wsel_wake_proc) {
        selwakeup(wsel_wake_proc, 0);
        wsel_wake_proc = 0;
      }
    }
    return ret;
  }
  else {
    return 0;
  }
}

int testc_write(dev, uio) dev_t dev; struct uio* uio; {
  int ret;
  outs("testc_write(");
  outi(dev);
  outs(")\n");
  if(stack_depth < MAX_STACK_DEPTH) {
    ret = uiomove(stack + stack_depth++, 1, UIO_WRITE, uio);
    if(ret == 0) {
      if(rsel_wake_proc) {
        selwakeup(rsel_wake_proc, 0);
        rsel_wake_proc = 0;
      }
    }
    return ret;
  }
  else {
    return EWOULDBLOCK;
  }
}

int testc_ioctl(dev, cmd, addr, arg)
  dev_t dev; int cmd; caddr_t* addr; int arg;
{
  outs("testc_ioctl(");
  outi(dev);
  outs(",");
  outi(cmd);
  outs(",");
  outptr(addr);
  outs(",");
  outi(arg);
  outs(")\n");
  switch(cmd) {
  case QUERY_STACK_DEPTH:
    *(int*)addr = stack_depth;
    return 0;
  default:
    return EINVAL;
  }
}

int testc_select(dev, rw) dev_t dev; int rw; {
  outs("testc_select(");
  outi(dev);
  outs(",");
  outi(rw);
  outs(")\n");
  switch(rw) {
  case 0:
    /* exceptional condition */
    return 0;
  case 1: /* FREAD? */
    /* readable */
    if(stack_depth > 0) {
      return 1;
    }
    else {
      rsel_wake_proc = u.u_procp;
      return 0;
    }
  case 2: /* FWRITE? */
    /* writable */
    if(stack_depth < MAX_STACK_DEPTH) {
      return 1;
    }
    else {
      wsel_wake_proc = u.u_procp;
      return 0;
    }
  }
}

