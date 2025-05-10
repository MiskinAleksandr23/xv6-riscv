#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define BUFSIZE 32

void test_io_operations() {
    int fd;
    char buf[BUFSIZE];

    printf("IO operations test\n");
    fd = mutex();
    if (fd < 0) {
        fprintf(2, "Mutex create error\n");
        exit(1);
    }

    printf(read(fd, buf, sizeof(buf)) >= 0 ? "Read unexpected\n" : "Read blocked\n");
    printf(write(fd, buf, sizeof(buf)) >= 0 ? "Write unexpected\n" : "Write blocked\n");

    close(fd);
}

void test_owner_close() {
    int fd = mutex();
    if (fd < 0) {
        fprintf(2, "Mutex create error\n");
        exit(1);
    }
    if (mutex_lock(fd) < 0) {
        fprintf(2, "Lock error\n");
        exit(1);
    }
    printf(close(fd) < 0 ? "Close failed\n" : "Closed locked\n");
}

void test_foreign_close() {
    int fd = mutex();
    if (fd < 0) {
        fprintf(2, "Mutex create error\n");
        exit(1);
    }
    if (mutex_lock(fd) < 0) {
        fprintf(2, "Lock error\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        fprintf(2, "Fork error\n");
        exit(1);
    }
    if (pid == 0) {
        printf(close(fd) < 0 ? "Foreign close blocked\n" : "Foreign close bug\n");
        exit(0);
    }

    sleep(2);
    if (mutex_unlock(fd) < 0) {
        fprintf(2, "Unlock failed\n");
    }
    printf(close(fd) < 0 ? "Parent close failed\n" : "Parent closed ok\n");
    wait(0);
}

void test_exit_lock() {
    int pid = fork();
    if (pid < 0) {
        fprintf(2, "Fork error\n");
        exit(1);
    }
    if (pid == 0) {
        int fd = mutex();
        if (mutex_lock(fd) < 0) {
            fprintf(2, "Child lock failed\n");
            exit(1);
        }
        printf("Child exit locked\n");
        exit(0);
    }
    wait(0);
    printf("Parent saw exit\n");
}

void test_foreign_unlock() {
    int fd = mutex();
    if (mutex_lock(fd) < 0) {
        fprintf(2, "Parent lock failed\n");
        exit(1);
    }
    int pid = fork();

    if (pid == 0) {
        printf(mutex_unlock(fd) < 0 ? "Unlock denied\n" : "Unlock bug\n");
        exit(0);
    }

    sleep(2);
    if (mutex_unlock(fd) < 0) {
        fprintf(2, "Unlock failed\n");
    }
    close(fd);
    wait(0);
}

int main() {
    test_io_operations();
    test_owner_close();
    test_foreign_close();
    test_exit_lock();
    test_foreign_unlock();
    exit(0);
}
