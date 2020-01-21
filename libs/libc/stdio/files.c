#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define STDIO_OWNED 0x800000

FILE *stdout;
FILE *stdin;
FILE *stderr;

static FILE files[3];

static int parse_flags(const char *mode) {
    int flags = 0;
    switch (mode[0]) {
        case 'r':
            switch (mode[1]) {
                case '+':
                    flags = O_RDWR;
                    break;
                default:
                    flags = O_RDONLY;
                    break;
            }
            break;
        case 'w':
            switch (mode[1]) {
                case '+':
                    flags = O_RDWR | O_CREAT | O_TRUNC;
                    break;
                default:
                    flags = O_WRONLY | O_CREAT | O_TRUNC;
                    break;
            }
            break;
        case 'a':
            switch (mode[1]) {
                case '+':
                    flags = O_RDWR | O_CREAT | O_APPEND;
                    break;
                default:
                    flags = O_WRONLY | O_CREAT | O_APPEND;
                    break;
            }
            break;
    }

    return flags;
}

static void update_file(FILE *file, int fd, int flags) {
    file->pos = 0;
    file->buffer = malloc(BUFSIZ);
    file->fd = fd;
    file->length = (flags & O_RDONLY) ? 0 : BUFSIZ;
    file->flags = flags | STDIO_OWNED;
    file->buf_type = _IOFBF;
    file->eof = 0;
    file->error = 0;
    file->pushed_back_char = '\0';
}

FILE *fopen(const char *__restrict path, const char *__restrict mode) {
    int flags = parse_flags(mode);

    mode_t __mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int fd = open(path, flags, __mode);

    /* Check If Open Failed */
    if (fd == -1) {
        return NULL;
    }

    FILE *file = malloc(sizeof(FILE));
    update_file(file, fd, flags);

    return file;
}

FILE *freopen(const char *pathname, const char *mode, FILE *stream) {
    if (!mode) {
        errno = EINVAL;
        return NULL;
    }

    int flags = parse_flags(mode);

    if (!stream) {
        return fopen(pathname, mode);
    }

    if (!pathname) {
        fcntl(stream->fd, F_SETFL, flags);
        stream->flags = flags;
        return stream;
    }

    if (close(stream->fd) != 0) {
        return NULL;
    }

    fflush(stream);
    free(stream->buffer);
    int fd = open(pathname, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (fd == -1) {
        return NULL;
    }

    update_file(stream, fd, flags);
    return stream;
}

FILE *fdopen(int fd, const char *__restrict mode) {
    int flags = parse_flags(mode);

    FILE *file = malloc(sizeof(FILE));
    update_file(file, fd, flags);

    return file;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    // Actually should be doing this when we try to read from any _IOLBF stream,
    // and it should flush all other _IOLBF streams, not just stdout
    if (stream->fd == STDIN_FILENO) {
        fflush(stdout);
    }

    if (stream->buf_type == _IONBF) {
        ssize_t ret = read(stream->fd, ptr, nmemb * size);

        if (ret == 0) {
            stream->eof = 1;
        } else if (ret < 0) {
            stream->error = 1;
        }

        if (ret < 0) {
            return 0;
        }

        return (size_t) ret;
    }

    size_t to_read = size * nmemb;
    if (stream->length - stream->pos > 0) {
        size_t can_copy = MIN(to_read, (size_t)(stream->length - stream->pos));
        memcpy(ptr, stream->buffer + stream->pos, can_copy);
        stream->pos += can_copy;
        to_read -= can_copy;
        if (to_read == 0) {
            return can_copy;
        }
    }

    if (to_read > BUFSIZ) {
        ssize_t ret = read(stream->fd, ptr, to_read);

        if (ret < 0) {
            return 0;
        }

        return (size_t) ret;
    }

    stream->pos = 0;
    stream->length = read(stream->fd, stream->buffer, BUFSIZ);

    // Read failed
    if (stream->length <= 0) {
        stream->error = stream->length < 0 ? 1 : 0;
        stream->eof = stream->length < 0 ? 0 : 1;
        return size * nmemb - to_read;
    }

    memcpy(ptr, stream->buffer, to_read);
    stream->pos += to_read;
    return nmemb * size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    char *data = (char *) ptr;

    if (stream->buf_type == _IONBF) {
        ssize_t ret = write(stream->fd, ptr, nmemb * size);

        if (ret < 0) {
            return 0;
        }

        return (size_t) ret;
    } else if (stream->buf_type == _IOLBF) {
        size_t len = size * nmemb;
        fpos_t start = stream->pos;
        size_t i = 0;

        while (i < len) {
            while (i < len && stream->pos < stream->length && data[i] != '\n') {
                stream->buffer[stream->pos++] = data[i++];
            }

            if (i >= len) {
                return len;
            }

            stream->buffer[stream->pos++] = data[i++];
            ssize_t check = write(stream->fd, stream->buffer, stream->pos);

            if (check < 0) {
                return i - stream->pos + start;
            }

            stream->pos = 0;
            start = 0;
        }

        return i;
    } else {
        size_t len = size * nmemb;
        fpos_t start = stream->pos;
        size_t i = 0;

        while (i < len) {
            while (i < len && stream->pos < stream->length) {
                stream->buffer[stream->pos++] = data[i++];
            }

            if (i >= len) {
                return len;
            }

            ssize_t check = write(stream->fd, stream->buffer, stream->pos);

            if (check < 0) {
                return i - stream->pos + start;
            }

            stream->pos = 0;
            start = 0;
        }

        return i;
    }
}

int fclose(FILE *stream) {
    if (fflush(stream) == EOF) {
        return EOF;
    }

    int ret = close(stream->fd);
    if (ret < 0) {
        return EOF;
    }

    if (stream->buffer != NULL && stream->flags & STDIO_OWNED) {
        free(stream->buffer);
    }

    if (stream != stdin && stream != stdout && stream != stderr) {
        free(stream);
    }
    return 0;
}

int fflush(FILE *stream) {
    if (stream == NULL) {
        /* Should Flush All Open Streams */
        assert(false);
    }

    if (stream->pos != 0 && stream->buf_type != _IONBF && (stream->flags & O_WRONLY || stream->flags & O_RDWR)) {
        ssize_t check = write(stream->fd, stream->buffer, stream->pos);
        stream->pos = 0;

        if (check < 0) {
            return EOF;
        }

        return 0;
    }

    return 0;
}

int fputs(const char *s, FILE *f) {
    return fprintf(f, "%s", s);
}

int getc(FILE *stream) {
    return fgetc(stream);
}

int fgetc(FILE *stream) {
    if (stream->pushed_back_char != '\0') {
        int ret = (int) stream->pushed_back_char;
        stream->pushed_back_char = '\0';
        return ret;
    }

    char c;
    ssize_t ret = fread(&c, sizeof(char), 1, stream);
    if (ret <= 0) {
        return EOF;
    }

    return (int) c;
}

int ungetc(int c, FILE *stream) {
    stream->pushed_back_char = (unsigned char) c;
    return c;
}

int getchar() {
    return fgetc(stdin);
}

int putc(int c, FILE *stream) {
    return fputc(c, stream);
}

int fputc(int c, FILE *stream) {
    int ret = fprintf(stream, "%c", (char) c);
    if (ret < 0) {
        return EOF;
    }

    return (unsigned char) c;
}

int putchar(int c) {
    return fputc(c, stdout);
}

char *fgets(char *__restrict buf, int size, FILE *__restrict stream) {
    int i = 0;
    while (i < size - 1) {
        errno = 0;
        int c = fgetc(stream);
        if (c == EOF) {
            if (errno || i == 0) {
                return NULL;
            }
            break;
        }

        if (c == '\n') {
            buf[i++] = (char) c;
            break;
        }

        buf[i++] = (char) c;
    }

    buf[i] = '\0';
    return buf;
}

int fgetpos(FILE *stream, fpos_t *pos) {
    /* I doubt this is the intended behavior */
    *pos = stream->pos;
    return 0;
}

int fseek(FILE *stream, long offset, int whence) {
    off_t ret = lseek(stream->fd, offset, whence);
    assert(ret <= INT_MAX);

    if (ret < 0) {
        return -1;
    }

    stream->eof = 0;
    return 0;
}

int fsetpos(FILE *stream, const fpos_t *pos) {
    /* I doubt this is the intended behavior */
    stream->pos = *pos;
    return 0;
}

long ftell(FILE *stream) {
    return lseek(stream->fd, 0, SEEK_CUR);
}

void rewind(FILE *stream) {
    fseek(stream, 0L, SEEK_SET);
    clearerr(stream);
}

void clearerr(FILE *stream) {
    stream->error = 0;
    stream->eof = 0;
}

int feof(FILE *stream) {
    return stream->eof;
}

int ferror(FILE *stream) {
    return stream->error;
}

int fileno(FILE *stream) {
    return stream->fd;
}

void setbuf(FILE *stream, char *buf) {
    setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}

int setvbuf(FILE *stream, char *buf, int mode, size_t size) {
    if (stream->flags & STDIO_OWNED) {
        free(stream->buffer);
    }

    if (buf == NULL) {
        buf = malloc(size);
    }
    stream->buffer = buf;
    stream->buf_type = mode;
    stream->length = size;
    stream->flags &= STDIO_OWNED;
    return 0;
}

static char tmp_name_buffer[L_tmpnam] = { 0 };
static bool is_tmp_name_buffer_initialzied = false;

char *tmpnam(char *s) {
    if (!is_tmp_name_buffer_initialzied) {
        pid_t pid = getpid();

        strcpy(tmp_name_buffer, P_tmpdir);
        strcat(tmp_name_buffer, "/");
        for (size_t i = strlen(P_tmpdir) + 1; i < strlen(P_tmpdir) + 1 + 10; i++) {
            tmp_name_buffer[i] = ((pid + i) % 26) + 'a';
        }

        is_tmp_name_buffer_initialzied = true;
    }

    for (size_t i = strlen(P_tmpdir) + 1; i < strlen(P_tmpdir) + 1 + 10; i++) {
        tmp_name_buffer[i] = (((tmp_name_buffer[i] + 1) - 'a') % 26) + 'a';
    }

    if (s) {
        memcpy(s, tmp_name_buffer, L_tmpnam);
    }

    return s ? s : tmp_name_buffer;
}

FILE *tmpfile(void) {
    size_t failed_attempts = 0;
    while (failed_attempts < TMP_MAX) {
        char name[L_tmpnam];
        tmpnam(name);

        if (access(name, F_OK) != -1) {
            failed_attempts++;
            continue;
        }

        return fopen(name, "w+");
    }

    // Give up
    return NULL;
}

#define DEFAULT_LINE_BUFFER_SIZE 100

ssize_t getline(char **__restrict line_ptr, size_t *__restrict n, FILE *__restrict stream) {
    if (*line_ptr == NULL) {
        if (*n == 0) {
            *n = DEFAULT_LINE_BUFFER_SIZE;
        }

        *line_ptr = malloc(*n);
    }

    size_t pos = 0;
    for (;;) {
        errno = 0;
        int c = fgetc(stream);

        /* Indicate IO error or out of lines */
        if (c == EOF && (errno || pos == 0)) {
            return -1;
        }

        if (c == EOF) {
            (*line_ptr)[pos] = '\0';
            break;
        }

        if (c == '\n' || c == '\r') {
            (*line_ptr)[pos++] = c;
            (*line_ptr)[pos] = '\0';
            break;
        }

        (*line_ptr)[pos++] = c;

        if (pos + 1 >= *n) {
            *n *= 2;
            *line_ptr = realloc(*line_ptr, *n);
        }
    }

    return (ssize_t) pos;
}

static pid_t p_child_pid = 0;

FILE *popen(const char *command, const char *mode) {
    if (!command || !mode) {
        errno = EINVAL;
        return NULL;
    }

    int fds[2];
    if (pipe(fds)) {
        return NULL;
    }

    FILE *f;

    if (mode[0] == 'r') {
        f = fdopen(fds[0], mode);
    } else if (mode[0] == 'w') {
        f = fdopen(fds[1], mode);
    } else {
        errno = EINVAL;
        return NULL;
    }

    if (!f) {
        return NULL;
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (mode[0] == 'r') {
            dup2(fds[1], STDOUT_FILENO);
            close(fds[1]);
        } else {
            dup2(fds[0], STDIN_FILENO);
            close(fds[0]);
        }

        char *const args[] = { "sh", "-c", (char *) command, NULL };

        execve("/bin/sh", args, environ);
        _exit(127);
    } else if (pid == -1) {
        fclose(f);
        return NULL;
    }

    p_child_pid = pid;
    if (mode[0] == 'r') {
        close(fds[1]);
    } else {
        close(fds[0]);
    }

    return f;
}

int pclose(FILE *stream) {
    if (p_child_pid == 0) {
        errno = EINVAL;
        return -1;
    }

    int wstatus;
    while (!waitpid(p_child_pid, &wstatus, 0))
        ;
    p_child_pid = 0;

    if (fclose(stream) || wstatus == -1) {
        return -1;
    }

    return WEXITSTATUS(wstatus);
}

void perror(const char *s) {
    assert(s != NULL);

    fprintf(stderr, "%s: %s\n", s, strerror(errno));
}

int remove(const char *path) {
    int ret = unlink(path);
    if (ret == -1 && errno == EISDIR) {
        return rmdir(path);
    }

    return ret;
}

void init_files() {
    /* stdin */
    files[0].fd = 0;
    files[0].pos = 0;
    files[0].buf_type = isatty(STDIN_FILENO) ? _IONBF : _IOFBF;
    files[0].buffer = malloc(BUFSIZ);
    files[0].length = 0;
    files[0].eof = 0;
    files[0].error = 0;
    files[0].flags = O_RDWR | STDIO_OWNED;
    files[0].pushed_back_char = '\0';
    stdin = files + 0;

    /* stdout */
    files[1].fd = 1;
    files[1].buf_type = isatty(STDOUT_FILENO) ? _IOLBF : _IOFBF;
    files[1].buffer = malloc(BUFSIZ);
    files[1].length = BUFSIZ;
    files[1].eof = 0;
    files[1].error = 0;
    files[1].flags = O_RDWR | STDIO_OWNED;
    files[1].pos = 0;
    files[1].pushed_back_char = '\0';
    stdout = files + 1;

    /* stderr */
    files[2].fd = 2;
    files[2].pos = 0;
    files[2].buf_type = _IONBF;
    files[2].buffer = NULL;
    files[2].length = 0;
    files[2].eof = 0;
    files[2].error = 0;
    files[2].flags = O_RDWR | STDIO_OWNED;
    files[2].pushed_back_char = '\0';
    stderr = files + 2;
}