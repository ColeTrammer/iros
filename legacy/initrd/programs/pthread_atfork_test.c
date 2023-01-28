/* CELEBP60 */
/* Example using SUSv3 pthread_atfork() interface */

#define _UNIX03_THREADS 1

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define __errno2() errno

char fn_c[] = "childq.out";
char fn_p[] = "parentside.out";
int fd_c;
int fd_p;

void prep1(void) {
    char buff[80] = "prep1\n";
    assert(write(4, buff, sizeof(buff)));
}

void prep2(void) {
    char buff[80] = "prep2\n";
    assert(write(4, buff, sizeof(buff)));
}

void prep3(void) {
    char buff[80] = "prep3\n";
    assert(write(4, buff, sizeof(buff)));
}

void parent1(void) {
    char buff[80] = "parent1\n";
    assert(write(4, buff, sizeof(buff)));
}

void parent2(void) {
    char buff[80] = "parent2\n";
    assert(write(4, buff, sizeof(buff)));
}

void parent3(void) {
    char buff[80] = "parent3\n";
    assert(write(4, buff, sizeof(buff)));
}

void child1(void) {
    char buff[80] = "child1\n";
    assert(write(3, buff, sizeof(buff)));
}

void child2(void) {
    char buff[80] = "child2\n";
    assert(write(3, buff, sizeof(buff)));
}

void child3(void) {
    char buff[80] = "child3\n";
    assert(write(3, buff, sizeof(buff)));
}

void *thread1(void *arg) {
    (void) arg;
    printf("Thread1: Hello from the thread.\n");
    return NULL;
}

int main(void) {
    pthread_t thid;
    int rc, ret;
    pid_t pid;
    int status;
    char header[30] = "Called Child Handlers\n";

    if (pthread_create(&thid, NULL, thread1, NULL) != 0) {
        perror("pthread_create() error");
        exit(3);
    }

    if (pthread_join(thid, NULL) != 0) {
        perror("pthread_join() error");
        exit(5);
    } else {
        printf("IPT: pthread_join success!  Thread 1 should be finished now.\n");
        printf("IPT: Prepare to fork!!!\n");
    }

    /*-----------------------------------------*/
    /*|  Start atfork handler calls in parent  */
    /*-----------------------------------------*/
    /* Register call 1 */
    rc = pthread_atfork(&prep1, &parent2, &child3);
    if (rc != 0) {
        perror("IPT: pthread_atfork() error [Call #1]");
        printf("  rc= %d, errno: %d, ejr: %08x\n", rc, errno, __errno2());
    }

    /* Register call 2 */
    rc = pthread_atfork(&prep2, &parent3, &child1);
    if (rc != 0) {
        perror("IPT: pthread_atfork() error [Call #2]");
        printf("  rc= %d, errno: %d, ejr: %08x\n", rc, errno, __errno2());
    }

    /* Register call 3 */
    rc = pthread_atfork(&prep3, &parent1, NULL);
    if (rc != 0) {
        perror("IPT: pthread_atfork() error [Call #3]");
        printf("  rc= %d, errno: %d, ejr: %08x\n", rc, errno, __errno2());
    }

    /* Create output files to expose the execution of fork handlers. */
    if ((fd_c = creat(fn_c, S_IWUSR)) < 0)
        perror("creat() error");
    else
        printf("Created %s and assigned fd= %d\n", fn_c, fd_c);
    if ((ret = write(fd_c, header, 30)) == -1)
        perror("write() error");
    else
        printf("Write() wrote %d bytes in %s\n", ret, fn_c);

    if ((fd_p = creat(fn_p, S_IWUSR)) < 0)
        perror("creat() error");
    else
        printf("Created %s and assigned fd= %d\n", fn_p, fd_p);
    if ((ret = write(fd_p, header, 30)) == -1)
        perror("write() error");
    else
        printf("Write() wrote %d bytes in %s\n", ret, fn_p);

    pid = fork();

    if (pid < 0)
        perror("IPT: fork() error");
    else {
        if (pid == 0) {
            printf("Child: I am the child!\n");
            printf("Child: My PID= %d, parent= %d\n", (int) getpid(), (int) getppid());
            exit(0);

        } else {
            printf("Parent: I am the parent!\n");
            printf("Parent: My PID= %d, child PID= %d\n", (int) getpid(), (int) pid);

            if (wait(&status) == -1)
                perror("Parent: wait() error");
            else if (WIFEXITED(status))
                printf("Child exited with status: %d\n", WEXITSTATUS(status));
            else
                printf("Child did not exit successfully\n");

            close(fd_c);
            close(fd_p);
        }
    }
}
