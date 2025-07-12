/* Copyright (c) Geoffrey P. Messer, 2025.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE JIM TCL PROJECT ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * JIM TCL PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the Jim Tcl Project.
 */

#include <jim.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <stdio.h>

#define BUF_SZ 128
#define ERR_BUF_SZ 512

int SetNonBlocking(Jim_Interp *interp, const char *cmd, int fd) {
  int flags;
  if ((flags = fcntl(fd, F_GETFL)) == -1) {
    Jim_SetResultFormatted(interp, "%s: fnctl get fail", cmd);
    return 0;	
  }
  if (fcntl(fd, F_SETFL, flags|O_NONBLOCK) == -1) {
    Jim_SetResultFormatted(interp, "%s: fnctl set fail", cmd);
    return 0;	
  }
  return 1;
}

static int ApexCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  static int to_bc[2] = {0};
  static int from_bc[2] = {0};
  static int err_bc[2] = {0};
  static int bc_active = 0;
  static char output[BUF_SZ];
  static char error[ERR_BUF_SZ];
  static pid_t pid;
  static int retcode;
  if (argc != 2) {
    Jim_WrongNumArgs(interp, 1, argv, "expression|close");
    return JIM_ERR;
  }
  const char *cmd = Jim_String(argv[0]);
  int len;
  const char *expression = Jim_GetString(argv[1], &len);
  const char *endExpr = "\nprint \"@\"\n";
  int extraLen = strlen(endExpr);
  char commandExpr[len + extraLen];
  memcpy(commandExpr, expression, len);
  memcpy(commandExpr + len, endExpr, extraLen);
  if (! bc_active) {
    retcode = JIM_ERR;
    if (pipe(to_bc) == -1
        || pipe(from_bc) == -1
        || pipe(err_bc) == -1) {
      Jim_SetResultFormatted(interp, "%s: pipe fail", cmd);
      return JIM_ERR;	
    }
    pid_t pid = fork();
    if (pid == -1) {
      Jim_SetResultFormatted(interp, "%s: fork fail", cmd);
      goto MATH_CLOSE;	
    }
    if (pid == 0) {
      /* child process */
      dup2(to_bc[0], STDIN_FILENO);
      dup2(from_bc[1], STDOUT_FILENO);
      dup2(err_bc[1], STDERR_FILENO);
      close(to_bc[0]); close(to_bc[1]);
      close(from_bc[0]); close(from_bc[1]);
      close(err_bc[0]); close(err_bc[1]);
      execlp("bc", "bc", "-lLq", NULL);
      /* should not get here */
      exit(1);
    }
    /* parent process */
    close(to_bc[0]);
    close(from_bc[1]);
    close(err_bc[1]);
    SetNonBlocking(interp, cmd, to_bc[1]);
    SetNonBlocking(interp, cmd, from_bc[0]);
    SetNonBlocking(interp, cmd, err_bc[0]);
    bc_active = 1;
  }
  if (strcmp(expression, "close") == 0) {
    retcode = JIM_OK;
    goto MATH_CLOSE;
  }
  ssize_t n;
  size_t written = 0, total = len + extraLen;
  while (written < total) {
    n = write(to_bc[1], commandExpr + written, total - written);
    if (n > 0) {
      written += n;
    } else if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      continue;
    } else {
      Jim_SetResultFormatted(interp, "%s: write fail", cmd);
      retcode = JIM_ERR;
      goto MATH_CLOSE;
    }
  }
  Jim_Obj *result = Jim_NewEmptyStringObj(interp);
  size_t done = 0;
  while (!done) {
    n = read(from_bc[0], output, BUF_SZ - 1);
    if (n > 0) {
      output[n] = '\0';
      char *at = strchr(output, '@');
      if (at) {
	done = 1;
	Jim_AppendString(interp, result, output, at - output);
      } else {
	Jim_AppendString(interp, result, output, n);
      }
    } else if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      continue;
    } else if (n == -1) {
      Jim_SetResultFormatted(interp, "%s: read fail", cmd);
      retcode = JIM_ERR;
      goto MATH_CLOSE;
    } else {
      break; /* EOF */
    }
  }
  if (done) {
    Jim_Obj *argk[4];
    argk[0] = Jim_NewStringObj(interp, "string", -1);
    argk[1] = Jim_NewStringObj(interp, "trimright", -1);
    argk[2] = result;
    argk[3] = Jim_NewStringObj(interp, "\n@", -1);
    Jim_EvalObjVector(interp, 4, argk);
    Jim_SetResult(interp, result);
    return JIM_OK;
  }
  ssize_t e = read(err_bc[0], error, ERR_BUF_SZ-1);
  if (e > 0) {
    error[e] = '\0';
    Jim_SetResultString(interp, error, -1);
    goto MATH_CLOSE;
  } else if (e == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
    Jim_SetResultFormatted(interp, "%s: error read error", cmd);
    goto MATH_CLOSE;
  }
MATH_CLOSE:
  close(to_bc[1]);
  close(from_bc[0]);
  close(err_bc[0]);
  waitpid(pid, NULL, 0);
  pid = 0;
  bc_active = 0;
  return retcode;
}

int Jim_apexInit(Jim_Interp *interp) {
  /* apex = Arbitrary Precision EXpression */
  Jim_CreateCommand(interp, "apex", ApexCmd, NULL, NULL);
  return JIM_OK;
}
