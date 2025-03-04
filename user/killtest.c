#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int main(int argc, char *argv[]) {
  // для пункта a) аргумент "-a", для пункта "-b"
  if (argc != 2) {
    fprintf(2, "Неверное число аргументов\n");
    exit(1);
  }
  int point_a = 0;
  if (strcmp(argv[1], "-a") == 0) {
    point_a = 1;
  } else if (strcmp(argv[1], "-b") != 0) {
    fprintf(2, "Аргумент должен быть -a или -b\n");
    exit(1);
  }
  int pid = fork();
  if (pid < 0) {
    fprintf(2, "fork не удался\n");
    exit(1);
  } else if (pid == 0) {
    sleep(60);
    exit(1);
  } else {
    fprintf(1, "Родительский процесс (PID: %d)\nДочерний процесс (PID: %d)\n", getpid(), pid);
    if (!point_a && kill(pid) < 0) {
      fprintf(2, "Сигнал kill не отправлен %d\n", pid);
    }
    int status;
    int finished = wait(&status);
    if (finished < 0) {
      fprintf(2, "Ошибка wait\n");
      exit(1);
    }
    fprintf(1, "Дочерний процесс (PID: %d) завершился с кодом %d\n", finished, status);
    exit(0);
  }
}
