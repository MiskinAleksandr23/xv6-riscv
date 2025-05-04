#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define SIZE 128


void test_null() {
    int fd, r;
    char buf[SIZE];
    printf("Tests /dev/null\n");
    fd = open("/dev/null", O_RDWR);
    r = write(fd, "hi", 2);

    printf("Write to null, returned(expected 2) %d\n", r);

    r = read(fd, buf, 1);
    printf("Read from null, returned(expected 0) %d\n\n", r);
    close(fd);
}


void test_zero() {
    int fd, r;
    char buf[SIZE];

    printf("Tests /dev/zero\n");
    fd = open("/dev/zero", O_RDWR);
    r = read(fd, buf, 3);
    printf("read three bytes: %d %d %d, returned(expected 3) %d\n", buf[0], buf[1], buf[2], r);

    r = write(fd, "message", 7);
    printf("writing to zero, expecting -1, get %d\n\n", r);
    close(fd);
}


void test_urandom() {
    int fd, r;
    char buf[SIZE];

    printf("Tests /dev/urandom\n");

    fd = open("/dev/urandom", O_RDWR);
    r = read(fd, buf, 3);

    printf("read three bytes: %d %d %d, returned(expected 3) %d\n", buf[0], buf[1], buf[2], r);

    uint64 seed = 22223333;
    r = write(fd, &seed, sizeof(seed));
    printf("write to urandom returned(expected 8) %d\n", r);

    r = read(fd, buf, 3);

    printf("read three bytes: %d %d %d, returned(expected 3) %d\n\n", buf[0], buf[1], buf[2], r);
    close(fd);
}

void test_nullstat() {
    int fd, r;
    printf("Tests /dev/nullstat\n");

    fd = open("/dev/nullstat", O_RDWR);
    r = write(fd, "xyz", 3);

    printf("Write to nullstat, returned(expected 3) %d\n", r);
    r = write(fd, "abcd", 4);
    printf("Write to nullstat, returned(expected 4) %d\n", r);


    uint64 count;
    r = read(fd, &count, sizeof(count));
    if (r == sizeof(count)) {
        printf("Read from nullstat, returned(expected 3 + 4 = 7) %d\n", (int) count);
    } else {
        printf("error while reading nullstat\n");
    }
    r = read(fd, &count, 16);
    printf("Read incorrect size, returned(expected -1) %d\n\n", r);
    close(fd);
}

int main(int argc, char** argv) {
    test_null();
    test_zero();
    test_urandom();
    test_nullstat();
}