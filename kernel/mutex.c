#include "types.h"
#include "param.h"
#include "sleeplock.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

static const int LOG_ENABLED = 1;

struct file *
mutexalloc(void)
{
    struct file *f = filealloc();
    if (!f) {
        if (LOG_ENABLED)
            printf("filealloc() failed for process pid %d\n", myproc()->pid);
        return 0;
    }

    if (LOG_ENABLED)
        printf("Process pid %d: file allocated at 0x%lx\n", myproc()->pid, (uint64)f);

    f->type = FD_MUTEX;

    f->mutex = (struct sleeplock *)kalloc();
    if (!f->mutex) {
        fileclose(f);
        if (LOG_ENABLED)
            printf("kalloc() failed for mutex in file at 0x%lx\n", (uint64)f);
        return 0;
    }

    if (LOG_ENABLED)
        printf("Process pid %d: kalloced mutex at 0x%lx for file at 0x%lx\n",
               myproc()->pid, (uint64)f->mutex, (uint64)f);

    initsleeplock(f->mutex, "mutex");
    return f;
}

void
mutexclose(struct file *f)
{
    if (!f || f->type != FD_MUTEX) {
        if (LOG_ENABLED)
            printf("Attempted to close non-mutex file at 0x%lx\n", (uint64)f);
        return;
    }

    if (f->mutex->locked && f->mutex->pid == myproc()->pid)
        releasesleep(f->mutex);

    if (LOG_ENABLED)
        printf("Process pid %d: releasing mutex at 0x%lx for file at 0x%lx\n",
               myproc()->pid, (uint64)f->mutex, (uint64)f);

    kfree((char *)f->mutex);
    if (LOG_ENABLED)
        printf("Process pid %d: freed mutex memory for file at 0x%lx\n",
              myproc()->pid, (uint64)f);

    f->mutex = 0;
}
