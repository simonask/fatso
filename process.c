#include "fatso.h"
#include "internal.h"

#include <string.h> // strdup
#include <stdio.h>
#include <unistd.h> // fork
#include <stdlib.h> // exit
#include <sys/select.h> // select etc.
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

struct fatso_process {
  char* path;
  FATSO_ARRAY(char*) args;
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
  const char* const argv[],
  const struct fatso_process_callbacks* callbacks,
  void* userdata
) {
  struct fatso_process* p = fatso_alloc(sizeof(struct fatso_process));
  p->path = strdup(path);

  for (const char* const* a = argv; *a; ++a) {
    char* str = strdup(*a);
    fatso_push_back_v(&p->args, &str);
  }
  char* null = NULL;
  fatso_push_back_v(&p->args, &null);

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
  for (size_t i = 0; i < p->args.size; ++i) {
    fatso_free(p->args.data[i]);
  }
  fatso_free(p->args.data);
  fatso_free(p);
}

void*
fatso_process_userdata(struct fatso_process* p) {
  return p->userdata;
}

struct signal_backup {
  struct sigaction intr;
  struct sigaction quit;
  sigset_t omask;
};

static void
restore_signals(struct signal_backup* sigback) {
  sigaction(SIGINT, &sigback->intr, NULL);
  sigaction(SIGQUIT, &sigback->quit, NULL);
  sigprocmask(SIG_SETMASK, &sigback->omask, NULL);
}

static void
process_start(struct fatso_process* p, struct signal_backup* sigback) {
  if (p->pid != 0) {
    return;
  }

  // Block some signals:
  struct sigaction sa;
  if (sigback) {
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, &sigback->intr);
    sigaction(SIGQUIT, &sa, &sigback->quit);
    sigaddset(&sa.sa_mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &sa.sa_mask, &sigback->omask);
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

    if (sigback) {
      // Restore the signals:
      restore_signals(sigback);
    }

    // Replace standard I/O:
    close(in[1]);
    close(out[0]);
    close(err[0]);
    dup2(in[0], fileno(stdin));
    dup2(out[1], fileno(stdout));
    dup2(err[1], fileno(stderr));

    // Take off:
    r = execvp(p->path, p->args.data);
    perror("execvp");
    exit(127);
  } else if (p->pid < 0) {
    // Error:
    perror("fork");
    return;
  } else {
    // Owner process:
    close(out[1]);
    close(err[1]);
    close(in[0]);
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

void
fatso_process_start(struct fatso_process* p) {
  process_start(p, NULL);
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
    if (maxfd < 0)
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
            } else {
              // EOF (socket was probably closed)
              break;
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
        if (r != 0) {
          if (r < 0) {
            perror("waitpid");
          }
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
  return fatso_system_with_callbacks(command, &standard_callbacks);
}

int
fatso_system_with_callbacks(const char* command, const struct fatso_process_callbacks* callbacks) {
  const char* new_argv[] = {
    "sh",
    "-c",
    command,
    NULL
  };

  struct fatso_process* process = fatso_process_new("/bin/sh", new_argv, callbacks, NULL);
  struct signal_backup sigback;
  process_start(process, &sigback);
  int r = fatso_process_wait(process);
  restore_signals(&sigback);
  fatso_process_free(process);
  return r;
}

