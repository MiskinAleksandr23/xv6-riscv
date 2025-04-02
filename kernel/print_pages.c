#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

#define PRINT 0
#define CLEAR 1

void print_page(int level, uint64 index, uint64 pt_id, pagetable_t pt, int mask) {
    char flags[8] = {
      (pt_id & PTE_R) ? 'R' : '_',
      (pt_id & PTE_W) ? 'W' : '_',
      (pt_id & PTE_X) ? 'X' : '_',
      (pt_id & PTE_U) ? 'U' : '_',
      (pt_id & PTE_G) ? 'G' : '_',
      (pt_id & PTE_A) ? 'A' : '_',
      (pt_id & PTE_D) ? 'D' : '_',
      '\0'
  };

    int should_print = 0;
    if (mask == 0) {
        should_print = 1;
    } else if ((mask == 1 || mask == 3) && flags[5] == 'A') {
        should_print = 1;
    } else if ((mask == 2 || mask == 3) && flags[6] == 'D') {
        should_print = 1;
    }

    if (!should_print && level == 3) {
        return;
    }

    const char *extra = "";
    if (level == 1) { extra = ""; }
    else if (level == 2) { extra = "........."; }
    else if (level == 3) { extra = "..................."; }
    char index_str[5];
    if (index < 10) {
        index_str[0] = '0';
        index_str[1] = '0';
        index_str[2] = '0' + index;
        index_str[3] = '\0';
    } else if (index < 100) {
        index_str[0] = '0';
        index_str[1] = '0' + index / 10;
        index_str[2] = '0' + index % 10;
        index_str[3] = '\0';
    } else {
        index_str[0] = '0' + index / 100;
        index_str[1] = '0' + (index / 10) % 10;
        index_str[2] = '0' + index % 10;
        index_str[3] = '\0';
    }
    printf("%s0x%s -> %p %s\n", extra, index_str, pt, flags);
}

uint64 handle_pages(uint64 buf, uint64 len, int mask, int action) {
    if (mask > 3 || mask < 0 || len < 0)
        return -1;

    struct proc *p = myproc();
    acquire(&p->lock);
    pagetable_t pt0 = p->pagetable;
    release(&p->lock);

    uint64 start_ind = (buf && len) ? buf: 0;
    uint64 end_ind = (buf && len) ? buf + len: MAXVA - 1;

    if(action == PRINT) printf("PAGETABLE %p\n", pt0);

    for (int l1 = 0; l1 < 512; ++l1) {
        if (!(pt0[l1] & PTE_V)) continue;

        uint64 pt1_va = (uint64)l1 << PXSHIFT(2);
        uint64 pt1_va_end = pt1_va + (1UL << PXSHIFT(2));
        if (pt1_va >= end_ind || pt1_va_end <= start_ind)
            continue;

        pagetable_t pt1 = (pagetable_t)PTE2PA(pt0[l1]);
        if (action == PRINT) print_page(1, l1, pt0[l1], pt1, mask);

        for (int l2 = 0; l2 < 512; ++l2) {
            if (!(pt1[l2] & PTE_V)) continue;

            uint64 pt2_va = pt1_va | (uint64)l2 << PXSHIFT(1);
            uint64 pt2_va_end = pt2_va + (1UL << PXSHIFT(1));

            if (pt2_va >= end_ind || pt2_va_end <= start_ind)
                continue;

            pagetable_t pt2 = (pagetable_t)PTE2PA(pt1[l2]);
            if (action == PRINT) print_page(2, l2, pt1[l2], pt2, mask);

            for (int l3 = 0; l3 < 512; ++l3) {
                if (!(pt2[l3] & PTE_V)) continue;

                uint64 pt3_va = pt2_va | (uint64)l3 << PXSHIFT(0);
                uint64 pt3_va_end = pt3_va + (1UL << PXSHIFT(0));

                if (pt3_va >= end_ind || pt3_va_end <= start_ind)
                    continue;

                if (action == PRINT) {
                  print_page(3, l3, pt2[l3], (pagetable_t)PTE2PA(pt2[l3]), mask);
                } else {
                  if ((mask == 1 || mask == 3) && (pt2[l3] & (uint64)PTE_A))
                      pt2[l3] &= ~(uint64)PTE_A;
                  if ((mask == 2 || mask == 3) && (pt2[l3] & (uint64)PTE_D))
                      pt2[l3] &= ~(uint64)PTE_D;
                }

            }
        }
    }
    return 0;
}

uint64 sys_print_pages(void) {
    uint64 buf, len;
    int mask;

    argaddr(0, &buf);
    argaddr(1, &len);
    argint(2, &mask);
    return handle_pages(buf, len, mask, PRINT);
}

uint64 sys_clear_flags(void) {
    uint64 buf, len;
    int mask;

    argaddr(0, &buf);
    argaddr(1, &len);
    argint(2, &mask);
    return handle_pages(buf, len, mask, CLEAR);
}
