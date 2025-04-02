#include "kernel/types.h"
#include "user/user.h"

#define SIZE 2048


void perror(const char *str) {
    write(2, str, strlen(str));
    exit(1);
}

int main(int argc, char *argv[]) {
    printf("Таблица страниц до выполнения любых операций:\n");
    print_pages(0, 0, 0);

    printf("\nТаблица страниц после работы со стековой переменной (var):\n");
    char var1 = 50;
    print_pages(&var1, 1, 2);
    char var2 = 100;
    print_pages(&var2, 1, 2);

    printf("\nТаблица страниц после работы с массивом на стеке (stack_array):\n");
    char stack_array[SIZE];
    stack_array[0] = 'a';
    stack_array[2000] = 'b';
    print_pages(stack_array, SIZE, 1);

    printf("\nТаблица страниц после выделения памяти на куче (malloc):\n");
    char *arr = malloc(SIZE);
    if ((uint64)arr == -1) {
        perror("Ошибка выделения памяти\n");
    }
    print_pages(arr, SIZE, 3);

    printf("\nТаблица страниц после сброса флагов A и D (clear_flags):\n");
    clear_flags(arr, SIZE, 3);
    print_pages(arr, SIZE, 3);

    printf("\nТаблица страниц после чтения данных из массива:\n");
    char val = arr[0];
    (void)val;
    print_pages(arr, SIZE, 3);

    printf("\nТаблица страниц после изменения данных в массиве:\n");
    arr[0] = 'X';
    arr[2000] = 'Y';
    print_pages(arr, SIZE, 3);

    printf("\nТаблица страниц после освобождения массива (free):\n");
    free(arr);
    print_pages(arr, SIZE, 3);

    printf("\nТаблица страниц с флагом A (Активирован):\n");
    print_pages(0, 0, 1);

    printf("\nТаблица страниц с флагом D (Изменен):\n");
    print_pages(0, 0, 2);

    printf("\nТаблица страниц с флагами A или D (Активирован или изменен):\n");
    print_pages(0, 0, 3);
}
