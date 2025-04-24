#include "types.h"
#include "riscv.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "defs.h"
#include "file.h"

#define NULL 0
#define ZERO 1
#define URANDOM 2
#define NULLSTAT 3

#define A 1337
#define B 11


struct pseudo {
  struct spinlock lock;
  uint64 seed;
  uint64 count;
};

static struct pseudo pseudo_dev;

uint8 gen_u8() {
  pseudo_dev.seed = pseudo_dev.seed * A + B;
  return (uint8) (pseudo_dev.seed >> 16);
}

int pseudo_read(int user_dst, uint64 dst, int len, short minor) {
  acquire(&pseudo_dev.lock);

  int result = -1;

  if (minor == NULL) {
    result = 0;
  }
  else if (minor == ZERO) {
    int has_error = 0;
    for (int i = 0; i < len; i++) {
      int value = 0;
      if (either_copyout(user_dst, dst + i, &value, 1) < 0) {
        has_error = 1;
        break;
      }
    } if (!has_error) {
      result = len;
    }
  }
  else if (minor == URANDOM) {
    int has_error = 0;
    for (int i = 0; i < len; i++) {
      uint8 value = gen_u8();
      if (either_copyout(user_dst, dst + i, &value, 1) < 0) { 
        has_error = 1;
        break;
      }
    }
    if (!has_error) {
      result = len;
    }
  }
  else if (minor == NULLSTAT) {
    if (len != sizeof(uint64) || either_copyout(user_dst, dst, &pseudo_dev.count, len) < 0) {
      result = -1;
    } else {
      result = len;
    }
  }
  release(&pseudo_dev.lock);
  return result;
}
int pseudo_write(int user_src, uint64 src, int len, short minor) {
  int result = -1;
  acquire(&pseudo_dev.lock);


  if (minor == NULL) {
    result = len;
  }
  else if (minor == ZERO) {
    result = -1;
  }
  else if (minor == URANDOM) {
    if (len == sizeof(uint64)) {
      uint64 new_seed = 0;
      if (either_copyin(&new_seed, user_src, src, len) >= 0) {
        pseudo_dev.seed = new_seed;
        result = len;
      }
    }
  }
  else if (minor == NULLSTAT) {
    pseudo_dev.count += len;
    result = len;
  }

  release(&pseudo_dev.lock);
  return result;
}


void pseudo_init(void) {
  initlock(&pseudo_dev.lock, "dev");
  pseudo_dev.seed = A;
  pseudo_dev.count = 0;
  devsw[DEV_PSEUDO].read = pseudo_read;
  devsw[DEV_PSEUDO].write = pseudo_write;
}