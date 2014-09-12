#include "fatso.h"
#include "internal.h"

#include <string.h> // strdup
#include <stdio.h>
#include <unistd.h> // fork
#include <stdlib.h> // exit
#include <sys/select.h> // select etc.
#include <errno.h>
#include <fcntl.h>

struct fatso_process {
  char* path;
  char** argv;
  void* userdata;
  const struct fatso_process_callbacks* callbacks;
  pid_t pid;
  int out;
  int err;
  int in;
};

struct fatso_process*
fatso_process_new(
  const char* path,
  char* const argv[],
  const struct fatso_process_callbacks* callbacks,
  void* userdata
) {
  struct fatso_process* p = fatso_alloc(sizeof(struct fatso_process));
  p->path = strdup(path);

  size_t idx;
  p->argv = fatso_alloc(sizeof(char*));
  p->argv[0] = strdup(path);
  for (char* const* a = argv; *a; ++a) {
    idx = a - argv;
    p->argv = fatso_reallocf(p->argv, (idx+3) * sizeof(char*));
    p->argv[idx+1] = strdup(*a);
  }

  p->userdata = userdata;
  p->callbacks = callbacks;
  p->pid = 0;
  return p;
}

void
fatso_process_free(struct fatso_process* p) {
  if (p->out != 0)
    close(p->out);
  if (p->err != 0)
    close(p->err);
  if (p->in != 0)
    close(p->in);
  fatso_free(p->path);
  for (char** a = p->argv; *a; ++a) {
    fatso_free(*a);
  }
  fatso_free(p->argv);
  fatso_free(p);
}

void*
fatso_process_userdata(struct fatso_process* p) {
  return p->userdata;
}

void
fatso_process_start(struct fatso_process* p) {
  if (p->pid != 0) {
    return;
  }

  int out[2];
  int err[2];
  int in[2];

  int r;
  r = pipe(out) ? -1 : (pipe(err) ? -1 : pipe(in));
  if (r != 0) {
    perror("pipe");
  }

  // Flush standard output before forking, to avoid buffer issues.
  fflush(stdout);
  fflush(stderr);
  p->pid = fork();

  if (p->pid == 0) {
    // Child process:
    dup2(in[0], fileno(stdin));
    dup2(out[1], fileno(stdout));
    dup2(err[1], fileno(stderr));
    r = execvp(p->path, p->argv);
    perror("execvp");
    exit(1);
  } else if (p->pid < 0) {
    // Error:
    perror("fork");
    return;
  } else {
    // Owner process:
    p->out = out[0];
    p->err = err[0];
    p->in = in[1];
    r = fcntl(p->out, F_SETFL, O_NONBLOCK);
    if (r != 0)
      perror("fcntl");
    r = fcntl(p->err, F_SETFL, O_NONBLOCK);
    if (r != 0)
      perror("fcntl");
  }
}

int
fatso_process_wait(struct fatso_process* p) {
  int status;
  int r = fatso_process_wait_all(&p, &status, 1);
  if (r >= 0) {
    return status;
  } else {
    return r;
  }
}

int
fatso_process_wait_all(
  struct fatso_process** processes,
  int* out_statuses,
  size_t nprocesses
) {
  int r;
  size_t num_exited = 0;
  while (num_exited < nprocesses) {
    // Get output:
    fd_set fds;
    fd_set efds;
    FD_ZERO(&fds);
    FD_ZERO(&efds);
    int maxfd = -1;
    for (size_t i = 0; i < nprocesses; ++i) {
      struct fatso_process* p = processes[i];
      if (p->pid > 0) {
        FD_SET(p->out, &fds);
        FD_SET(p->out, &efds);
        FD_SET(p->err, &fds);
        if (p->out > maxfd) maxfd = p->out;
        if (p->err > maxfd) maxfd = p->err;
      }
    }
    if (maxfd <= 0)
      break;

    int numready = select(maxfd + 1, &fds, NULL, &efds, NULL);
    if (numready < 0) {
      if (errno == EINTR)
        continue;
      perror("select");
      return -1;
    }

    for (int fd = 1; fd < maxfd + 1; ++fd) {
      if (FD_ISSET(fd, &fds)) {
        // Find the corresponding process
        struct fatso_process* p;
        bool iserr = false;
        for (size_t i = 0; i < nprocesses; ++i) {
          p = processes[i];
          if (fd == p->out) { break; }
          if (fd == p->err) { iserr = true; break; }
        }

        // Find the appropriate callback:
        void(*callback)(struct fatso_process*, const void*, size_t) = NULL;
        if (p->callbacks) {
          if (iserr) {
            callback = p->callbacks->on_stderr;
          } else {
            callback = p->callbacks->on_stdout;
          }
        }

        // Read data from the pipe and notify the callback:
        if (callback) {
          static const size_t BUFLEN = 1024;
          char* buffer = alloca(BUFLEN);
          ssize_t n;
          while (true) {
            n = read(fd, buffer, BUFLEN);
            if (n > 0) {
              callback(p, buffer, n);
            } else if (n < 0) {
              if (errno == EAGAIN) {
                break;
              } else {
                perror("read");
                return -1;
              }
            }
          }
        }
      }
    }

    // Run through processes and check who exited:
    for (size_t i = 0; i < nprocesses; ++i) {
      struct fatso_process* p = processes[i];
      if (p->pid > 0) {
        int wstatus;
        r = waitpid(p->pid, &wstatus, WNOHANG);
        if (WIFEXITED(wstatus)) {
          p->pid = 0;
          close(p->out);
          close(p->err);
          close(p->in);
          p->out = 0;
          p->err = 0;
          p->in = 0;
          out_statuses[i] = WEXITSTATUS(wstatus);
          ++num_exited;
        }
      }
    }
  }

  return 0;
}

int
fatso_process_kill(struct fatso_process* p, pid_t sig) {
  return kill(p->pid, sig);
}

static void
forward_stdout(struct fatso_process* p, const void* buffer, size_t len) {
  write(fileno(stdout), buffer, len);
}

static void
forward_stderr(struct fatso_process* p, const void* buffer, size_t len) {
  write(fileno(stderr), buffer, len);
}

int
fatso_system(const char* command) {
  static const struct fatso_process_callbacks standard_callbacks = {
    .on_stdout = forward_stdout,
    .on_stderr = forward_stderr,
  };

  char* path = NULL;
  FATSO_ARRAY(char*) args = {0};

  const char* p0 = command;
  while (*p0) {
    const char* p1 = strchr(p0, ' ');
    size_t len;
    if (p1) {
      len = p1 - p0;
    } else {
      len = strlen(p0);
    }
    const char* s0 = p0;
    size_t slen = len;

    if (len > 2) {
      // Strip leading and trailing quotes.
      if ((s0[0] == '"' || s0[0] == '\'') && s0[len-1] == s0[0]) {
        ++s0;
        slen -= 2;
      }
    }

    char* component = strndup(s0, slen);
    fatso_push_back_v(&args, &component);
    p0 = p0 + len + 1;
  }
  char* null = NULL;
  fatso_push_back_v(&args, &null);

  path = strdup(args.data[0]);

  struct fatso_process* process = fatso_process_new(path, args.data + 1, &standard_callbacks, NULL);
  fatso_process_start(process);
  int r = fatso_process_wait(process);
  fatso_process_free(process);
  for (size_t i = 0; i < args.size; ++i) {
    fatso_free(args.data[i]);
  }
  fatso_free(args.data);
  fatso_free(path);
  return r;
}

