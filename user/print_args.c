#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void print_chars(int argc, char *argv[], int use_mutex, int mutex_id) {
    char ch[2] = {0, 0};

    for (int i = 1; i < argc; i++) {
        for (int j = 0; argv[i][j]; j++) {
            if (use_mutex) mutex_lock(mutex_id);

            ch[0] = argv[i][j];
            printf("%d: arg %d, char '%s'\n", getpid(), i, ch);

            if (use_mutex) mutex_unlock(mutex_id);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(2, "Usage: %s <args...>\n", argv[0]);
        exit(1);
    }

    int mx = mutex();
    if (mx < 0) {
        fprintf(2, "Mutex init failed\n");
        exit(1);
    }

    printf("Unsynchronized:\n");
    if (fork() == 0) {
        print_chars(argc, argv, 0, 0);
        exit(0);
    } else {
        print_chars(argc, argv, 0, 0);
        wait(0);
    }

    printf("\nSynchronized:\n");
    if (fork() == 0) {
        print_chars(argc, argv, 1, mx);
        exit(0);
    } else {
        print_chars(argc, argv, 1, mx);
        wait(0);
    }

    close(mx);
    exit(0);
}
