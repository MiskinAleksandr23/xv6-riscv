#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

#define BUF_SIZE 512


int main(int argc, char *argv[]) {
  int pipefd[2];
  if (pipe(pipefd) < 0) {
    fprintf(2, "Не удалось создать pipe\n");
    exit(1);
  }
  int pid = fork();
  if (pid < 0) {
    fprintf(2, "Не удалось создать fork\n");
    exit(1);
  }
  if (pid == 0) {
    close(pipefd[1]);
    if (close(0) < 0) {
      fprintf(2, "Ошибка закрытия стандартного ввода в дочернем процессе\n");
      exit(1);
    }
    if (dup(pipefd[0]) < 0) {
      fprintf(2, "Ошибка dup\n");
      exit(1);
    }
    if (close(pipefd[0]) < 0) {
      fprintf(2, "Ошибка закрытия pipefd[0] в дочернем процессе\n");
      exit(1);
    }
    char *wc_args[] = {"wc", 0};
    exec("wc", wc_args);
    fprintf(2, "Ошибка exec\n");
    exit(1);
  } else {
    close(pipefd[0]);
    char buf[BUF_SIZE];
    int len = 0;
    for (int i = 0; i < argc; i++) {
      const int a_len = strlen(argv[i]);
      if (len + a_len + 1 >= BUF_SIZE) {
        int total_written = 0;
        while (total_written < len) {
          int written = write(pipefd[1], buf + total_written, len - total_written);
          if (written < 0) {
            fprintf(2, "Ошибка записи в pipe при переполнении буфера\n");
            close(pipefd[1]);
            exit(1);
          }
          total_written += written;
        }
        len = 0;
      }
      memcpy(buf + len, argv[i], a_len);
      len += a_len;
      buf[len] = '\n';
      len++;
    }
    if (len > 0) {
      int total_written = 0;
      while (total_written < len) {
        int written = write(pipefd[1], buf + total_written, len - total_written);
        if (written < 0) {
          fprintf(2, "Ошибка записи в pipe при завершении\n");
          close(pipefd[1]);
          exit(1);
        }
        total_written += written;
      }
    }
    close(pipefd[1]);
    int status;
    if (wait(&status) < 0) {
      fprintf(2, "Ошибка wait\n");
      exit(1);
    }
    exit(0);
  }
}