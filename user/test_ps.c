#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/procinfo.h"

int
main(void)
{
    int count, ret;

    // plist = NULL
    count = ps_listinfo(0, 0);
    printf("Test 1: Number of processes: %d\n", count);

    // недостаточный размер буфера
    if(count > 0){
        struct procinfo *buf = malloc(sizeof(struct procinfo) * (count - 1));
        ret = ps_listinfo(buf, count - 1);
        if(ret < 0)
            printf("Test 2: Buffer too small, error code: %d\n", ret);
        else
            printf("Test 2: Expected error, but got %d records\n", ret);
        free(buf);
    }

    // недопустимый адрес, просто очень большой
    ret = ps_listinfo((struct procinfo *)0xFFFFFFFF, count);
    if(ret < 0)
        printf("Test 3: Invalid address, error code: %d\n", ret);
    else
        printf("Test 3: Expected error, but got %d records\n", ret);
    exit(0);
}