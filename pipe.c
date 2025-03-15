#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define BUFSIZE 4096

int main(int argc, char *argv[]) {
  int pipefd[2];
  if (pipe(pipefd) < 0) {
    perror("Не удалось создать pipe");
    exit(1);
  }
  pid_t pid = fork();
  if (pid < 0) {
    perror("Не удалось создать fork");
    exit(1);
  }
  if (pid == 0) {
    if (close(pipefd[1]) < 0) {
      perror("Ошибка закрытия pipefd[1] в дочернем процессе");
      exit(1);
    }
    char buffer[BUFSIZE];
    int read_b = 0;
    while ((read_b = read(pipefd[0], buffer, BUFSIZE)) > 0) {
      int total_wr = 0;
      while (total_wr < read_b) {
        ssize_t written_b = write(1, buffer + total_wr, read_b - total_wr);
        if (written_b < 0) {
          perror("Ошибка записи");
          close(pipefd[0]);
          exit(1);
        }
        total_wr += written_b;
      }
    }
    if (read_b < 0) {
      perror("Ошибка чтения");
      close(pipefd[0]);
      exit(1);
    }
    if (close(pipefd[0]) < 0) {
      perror("Ошибка закрытия");
      exit(1);
    }
    fflush(stdout);
    exit(0);
  } else {
    close(pipefd[0]);
    for (int i = 0; i < argc; i++) {
      int len = strlen(argv[i]);
      int total_wr = 0;
      while (total_wr < len) {
        ssize_t written_b = write(pipefd[1], argv[i] + total_wr, len - total_wr);
        if (written_b < 0) {
          perror("Ошибка записи");
          close(pipefd[1]);
          exit(1);
        }
        total_wr += written_b;
      }
      while (total_wr < 1) {
        ssize_t written_b = write(pipefd[1], "\n", 1);
        if (written_b < 0) {
          perror("Ошибка записи новой строки");
          close(pipefd[1]);
          exit(1);
        }
        total_wr += written_b;
      }
    }
    if (close(pipefd[1]) < 0) {
      perror("Ошибка закрытия");
      exit(1);
    }
    int status;
    if (wait(&status) < 0) {
      perror("Ошибка wait");
      exit(1);
    }
    if (WIFEXITED(status)) {
      int exit_code = WEXITSTATUS(status);
      if (exit_code != 0) {
        fprintf(stderr, "Дочерний процесс завершился с кодом %d\n", exit_code);
        exit(1);
      }
    }
  }
}