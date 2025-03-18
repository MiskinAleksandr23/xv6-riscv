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
    printf("Колво процессов: %d\n", count);

    // недостаточный размер буфера
    if(count > 0){
        struct procinfo *buf = malloc(sizeof(struct procinfo) * (count - 1));
        ret = ps_listinfo(buf, count - 1);
        if(ret < 0)
            printf("Буфер слишком маленький: %d\n", ret);
        else
            printf("Ожидалась ошибка, но получено %d\n", ret);
        free(buf);
    }

    // недопустимый адрес, просто очень большой
    ret = ps_listinfo((struct procinfo *)0xFFFFFFFF, count);
    if(ret < 0)
        printf("Неверный адресс %d\n", ret);
    else
        printf("Ожидалась ошибка, но получено %d\n", ret);
    exit(0);
}