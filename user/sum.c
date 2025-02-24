#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"

#define MAX_INPUT_SIZE 1024

int space_index(char *buf) {
  for (int i = 0; buf[i] != '\0'; i++) {
    if (buf[i] == ' ') {
      return i;
    }
  }
  return -1;
}

int is_valid_number(char *str) {
  if (*str == '\0') {
    return 0;
  }
  int i = 0;
  if (str[i] == '-') {
    ++i;
    if (str[i] == '\0') {
      return 0;
    }
  }
  for (; str[i] != '\0'; i++) {
    if (str[i] != ' ' && (str[i] < '0' || str[i] > '9')) {
      return 0;
    }
  }
  return 1;
}

int main() {
  char buf[MAX_INPUT_SIZE];
  char ch;
  int n = 0;
  int fd = 0;

  while (1) {
    const int ret = read(fd, &ch, 1);
    if (ret < 0) {
      fprintf(2,"Ошибка: Некорректный формат\n");
      exit(1);
    } else if (ret == 0 || ch == '\n') {
      break;
    }
    if (n < MAX_INPUT_SIZE - 1) {
      buf[n++] = ch;
    } else {
      fprintf(2,"Ошибка: Переполнение буфера\n");
      exit(1);
    }
  }
  buf[n] = '\0';

  if (n == 0) {
    fprintf(2,"Ошибка: введено 0 чисел\n");
    exit(1);
  }

  fprintf(2,"|%s|\n", buf);

  const int space_ind = space_index(buf);
  if (space_ind == -1) {
    fprintf(2,"Ошибка: не найдено пробела между числами\n");
    exit(1);
  }

  for (int i = space_ind + 1; buf[i] != '\0'; i++) {
    if (buf[i] == ' ') {
      fprintf(2,"Ошибка: более двух чисел введено\n");
      exit(1);
    }
  }


  buf[space_ind] = '\0';
  char *num1_str = buf;
  char *num2_str = buf + space_ind + 1;

  if (!is_valid_number(num1_str)) {
    fprintf(2, "Ошибка: первое значение не является числом\n");
    exit(1);
  }
  if (!is_valid_number(num2_str)) {
    fprintf(2, "Ошибка: первое значение не является числом\n");
    exit(1);
  }

  int negative1 = *num1_str == '-';
  int negative2 = *num2_str == '-';

  if (negative1) num1_str++;
  if (negative2) num2_str++;

  int num1 = atoi(num1_str);
  int num2 = atoi(num2_str);

  if (negative1) num1 = -num1;
  if (negative2) num2 = -num2;

  int result = sum(num1, num2);
  printf("Сумма = %d\n", result);
  exit(0);
}