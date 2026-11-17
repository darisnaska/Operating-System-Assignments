
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>

static void print_error(const char *syscall_name)
{
    int saved_errno = errno;
    fprintf(stderr, "%s: %s (errno %d)\n",
            syscall_name, strerror(saved_errno), saved_errno);
}

int main(void)
{
    int fd;
    const char *path = "/tmp/p11_testfile";
    int tmp = open(path, O_CREAT | O_WRONLY, 0644);
    if (tmp >= 0)
        close(tmp); /* make sure the file exists on disk */

    fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd == -1)
        print_error("open");
    else
        close(fd); /* shouldn't happen, but stay tidy */

    if (close(-1) == -1)
        print_error("close");

    void * volatile bogus_buf = NULL;
    if (write(STDOUT_FILENO, bogus_buf, 10) == -1)
        print_error("write");

    if (getuid() == 0) {
        if (setgid(65534) == -1 || setuid(65534) == -1) {
            fprintf(stderr, "warning: could not drop privileges, "
                             "kill demo may not show EPERM\n");
        }
    }
    if (kill(1, 0) == -1)
        print_error("kill");

    unlink(path); /* clean up */
    return 0;
}