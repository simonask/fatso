#include "fatso.h"
#include "internal.h"

#include <string.h> // strdup
#include <stdio.h>
#include <unistd.h> // fork
#include <stdlib.h> // exit

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
  for (char* const* a = argv; *a; ++a) {
    size_t idx = a - argv;
    p->argv = fatso_reallocf(p->argv, (idx+1) * sizeof(char*));
    p->argv[idx] = strdup(*a);
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

  p->pid = fork();
  if (p->pid == 0) {
    // Child process:
    dup2(fileno(stdin), in[0]);
    dup2(fileno(stdout), out[1]);
    dup2(fileno(stderr), err[1]);
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
  }
}

int
fatso_process_wait(struct fatso_process* p) {
  if (p->pid == 0) {
    return -1;
  }

  int status;
  int r = waitpid(p->pid, &status, 0);
  if (r < 0) {
    perror("waitpid");
    return -1;
  }

  // TODO: Do this asynchronously.
  if (p->callbacks) {
    static const size_t BUFLEN = 1024;
    char* buffer = alloca(BUFLEN);
    if (p->callbacks->on_stdout) {
      while (true) {
        ssize_t n = read(p->out, buffer, BUFLEN);
        p->callbacks->on_stdout(p, buffer, n);
        if (n < BUFLEN)
          break;
      }
    }
    if (p->callbacks->on_stderr) {
      while (true) {
        ssize_t n = read(p->err, buffer, BUFLEN);
        p->callbacks->on_stderr(p, buffer, n);
        if (n < BUFLEN)
          break;
      }
    }
  }

  int exitstatus = WEXITSTATUS(status);
  return exitstatus;
}

int
fatso_process_kill(struct fatso_process* p, pid_t sig) {
  return kill(p->pid, sig);
}

