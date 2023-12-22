#include "lua.h"
#include "lauxlib.h"

#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int tk_system_posix_err (lua_State *L, int err)
{
  lua_pushboolean(L, 0);
  lua_pushstring(L, strerror(errno));
  lua_pushinteger(L, err);
  return 3;
}

int tk_system_posix_close (lua_State *L)
{
  int n = luaL_checkinteger(L, -1);
  int rc = close(n);
  if (rc == -1)
    return tk_system_posix_err(L, errno);
  lua_pushboolean(L, 1);
  return 1;
}

// TODO: use lua allocator or buffer functions instead of malloc
int tk_system_posix_read (lua_State *L)
{
  int fd = luaL_checkinteger(L, -2);
  int size = luaL_checkinteger(L, -1);
  char *buf = malloc(size); // buf
  if (buf == NULL)
    return tk_system_posix_err(L, ENOMEM);
  int bytes = read(fd, buf, size);
  if (bytes == -1) {
    int err = errno;
    free(buf);
    return tk_system_posix_err(L, err);
  }
  lua_pushboolean(L, 1);
  lua_pushlstring(L, buf, bytes);
  free(buf);
  return 2;
}

int tk_system_posix_setenv (lua_State *L)
{
  const char *k = luaL_checkstring(L, 1);
  const char *v = luaL_checkstring(L, 2);
  if (setenv(k, v, 1) == -1)
    return tk_system_posix_err(L, errno);
  lua_pushboolean(L, 1);
  return 1;
}

int tk_system_posix_dup2 (lua_State *L)
{
  int new = luaL_checkinteger(L, -1);
  int old = luaL_checkinteger(L, -2);
  int rc = dup2(old, new);
  if (rc == -1)
    return tk_system_posix_err(L, errno);
  lua_pushboolean(L, 1);
  return 1;
}

int tk_system_posix_pipe (lua_State *L)
{
  int fds[2];
  int rc = pipe(fds);
  if (rc == -1)
    return tk_system_posix_err(L, errno);
  lua_pushboolean(L, 1);
  lua_pushinteger(L, fds[0]);
  lua_pushinteger(L, fds[1]);
  return 3;
}

int tk_system_posix_execp (lua_State *L)
{
	const char *path = luaL_checkstring(L, -2);
  luaL_checktype(L, -1, LUA_TTABLE);
  size_t n = lua_objlen(L, -1);
	char *argv[n + 2];
  argv[0] = (char *) path;
  argv[n + 1] = NULL;
  for (int i = 1; i <= n; i ++) { // prg argt
    lua_pushinteger(L, i); // prg argt i
    lua_gettable(L, -2); // prg argt arg
    argv[i] = (char *) luaL_checkstring(L, -1); // prg argt arg
    lua_pop(L, 1); // prg argt
  }
	execvp(path, argv);
  return tk_system_posix_err(L, errno);
}

int tk_system_posix_fork (lua_State *L)
{
  pid_t pid = fork();
  if (pid == -1)
    return tk_system_posix_err(L, errno);
  lua_pushboolean(L, 1);
  lua_pushinteger(L, pid);
  return 2;
}

int tk_system_posix_wait (lua_State *L)
{
  int pid0 = luaL_checkinteger(L, -1);
  int status;
  int pid1 = waitpid(pid0, &status, 0);
  if (pid1 == -1)
    return tk_system_posix_err(L, errno);
  lua_pushboolean(L, 1);
  lua_pushinteger(L, pid1);
  if (WIFEXITED(status)) {
    lua_pushstring(L, "exited");
    lua_pushinteger(L, WEXITSTATUS(status));
    return 4;
  } else if (WIFSIGNALED(status)) {
    lua_pushstring(L, "signaled");
    lua_pushinteger(L, WTERMSIG(status));
    return 4;
  } else if (WIFSTOPPED(status)) {
    lua_pushstring(L, "stopped");
    lua_pushinteger(L, WSTOPSIG(status));
    return 4;
  } else if (WIFCONTINUED(status)) {
    lua_pushstring(L, "continued");
    return 3;
  } else {
    lua_pushstring(L, "unknown");
    lua_pushinteger(L, status);
    return 3;
  }
}

luaL_Reg tk_system_posix_fns[] =
{
  { "close", tk_system_posix_close },
  { "dup2", tk_system_posix_dup2 },
  { "execp", tk_system_posix_execp },
  { "pipe", tk_system_posix_pipe },
  { "fork", tk_system_posix_fork },
  { "read", tk_system_posix_read },
  { "wait", tk_system_posix_wait },
  { "setenv", tk_system_posix_setenv },
  { NULL, NULL }
};

int luaopen_santoku_system_posix (lua_State *L)
{
  lua_newtable(L);
  luaL_register(L, NULL, tk_system_posix_fns);
  return 1;
}
