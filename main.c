#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#define LOG_FILE "a.txt"
#define FIFO_NAME "/tmp/exo-server"
#define BUF_SIZE 1024
#define COUNT_SECONDS 10

volatile sig_atomic_t sig_term, sig_int, sig_alrm, sig_usr1, sig_hup;

typedef struct {
    uint64_t count_messages;
    uint64_t len_messages;
    uint64_t count_alarms;
} Stats;

Stats stats = {0, 0, 0};
FILE *f = NULL;
int fifo = -1;


void sig_term_handler(int signo) { (void)(signo); sig_term = 1; }
void sig_int_handler(int signo)  { (void)(signo); sig_int = 1; }
void sig_alrm_handler(int signo) { (void)(signo); sig_alrm = 1; }
void sig_usr1_handler(int signo) { (void)(signo); sig_usr1 = 1; }
void sig_hup_handler(int signo)  { (void)(signo); sig_hup = 1; }

void set_signals() {
    struct sigaction sa;

    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = sig_term_handler;
    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        perror("sigaction(SIGTERM)");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sig_int_handler;
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction(SIGINT)");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sig_alrm_handler;
    if (sigaction(SIGALRM, &sa, NULL) < 0) {
        perror("sigaction(SIGALRM)");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGQUIT, &sa, NULL) < 0) {
        perror("sigaction(SIGQUIT)");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sig_hup_handler;
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        perror("sigaction(SIGHUP)");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sig_usr1_handler;
    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("sigaction(SIGUSR1)");
        exit(EXIT_FAILURE);
    }
}


void print_stats() {
    fprintf(f, "Stats: %lu messages, %lu bytes, %lu alarms\n",
            stats.count_messages, stats.len_messages, stats.count_alarms);
    fflush(f);
}

void clean_up() {
    if (fifo != -1) {
        if (close(fifo) == -1) {  
            perror("close fifo failed");
        }
        fifo = -1;
    }

    if (f && f != stdout) {
        fclose(f);
    }

    if (unlink(FIFO_NAME) == -1) {
        perror("unlink failed");
    }

    exit(EXIT_SUCCESS);
}

void demonize_process(bool immediately) {
    if (f && f != stdout) {  
        fclose(f);
        f = NULL;
    }
    if (immediately) {
        daemon(1, 0);
    } else {
        if (fork() > 0) exit(EXIT_SUCCESS);
        if (setsid() == -1) {
            perror("setsid failed");
            exit(EXIT_FAILURE);
        }
        if (fork() > 0) exit(EXIT_SUCCESS);

        umask(0);
        chdir("/");
    }

    f = fopen(LOG_FILE, "w");
    if (!f) {
        perror("fopen failed");
        exit(EXIT_FAILURE);
    }

    setvbuf(f, NULL, _IOLBF, 0);

    int fd = fileno(f);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    f = stdout;
}

void eintr_error() {
    if (sig_term) {
        fprintf(f, "Received SIGTERM: terminating immediately upon user request\n");
        print_stats();
        clean_up();
    }
    if (sig_int) {
        fprintf(f, "Received SIGINT: finishing current read, then exiting\n");
        print_stats();
    }
    if (sig_usr1) {
        sig_usr1 = 0;
        print_stats();
    }
    if (sig_alrm) {
        sig_alrm = 0;
        stats.count_alarms++;
        fprintf(f, "Received SIGALRM: still waiting for data...\n");
        alarm(COUNT_SECONDS);
    }
    if (sig_hup) {
        sig_hup = 0;
        if (f && f != stdout) { 
            fclose(f);
            f = NULL;
        }
        demonize_process(false);
        fprintf(f, "Received SIGHUP: switched to daemon mode\n");
        print_stats();
        alarm(COUNT_SECONDS);
    }
}


int main(int argc, char **argv) {
    if (argc != 1 && argc != 2) {
        fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    bool is_demon = (argc == 2);

    f = stdout;
    if (is_demon) {
        demonize_process(true);
    }

    set_signals();

    if (mkfifo(FIFO_NAME, S_IRUSR | S_IWUSR) == -1) {
        if (errno == EEXIST) {
            struct stat st;
            if (stat(FIFO_NAME, &st) == -1 || !S_ISFIFO(st.st_mode)) {
                perror("FIFO check failed");
                exit(EXIT_FAILURE);
            }
            fprintf(f, "FIFO already exists.\n");
        } else {
            perror("mkfifo failed");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(f, "FIFO created.\n");
    }

    alarm(COUNT_SECONDS);

    bool is_sig_int = false;

    while (true) {
        fifo = open(FIFO_NAME, O_RDONLY);
        if (fifo < 0) {
            if (errno == EINTR) {
                eintr_error();
                if (sig_int) {
                    sig_int = 0;
                    clean_up();
                }
                continue;
            }
            perror("open failed");
            if (fifo != -1)  close(fifo);
            exit(EXIT_FAILURE);
        }

        char buffer[BUF_SIZE + 1];
        ssize_t n;

        while (true) {
            n = read(fifo, buffer, BUF_SIZE);
            if (n < 0) {
                if (errno == EINTR) {
                    eintr_error();
                    if (sig_int) {
                        sig_int = 0;
                        is_sig_int = true;
                    }
                    continue;
                }
                perror("read failed");
                break;
            } else if (n == 0) {
                if (is_sig_int) {
                    clean_up();
                }
                break;
            } else {
                buffer[n] = '\0';
                buffer[n - 1] = '\n';
                fputs(buffer, f);
                fflush(f);
                stats.len_messages += n;
            }  stats.count_messages++;
        }

        close(fifo);
        fifo = -1;
    }

    clean_up();
}
