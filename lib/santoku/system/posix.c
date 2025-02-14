#include "lua.h"
#include "lauxlib.h"

#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static inline void tk_lua_callmod (lua_State *L, int nargs, int nret, const char *smod, const char *sfn)
{
  lua_getglobal(L, "require"); // arg req
  lua_pushstring(L, smod); // arg req smod
  lua_call(L, 1, 1); // arg mod
  lua_pushstring(L, sfn); // args mod sfn
  lua_gettable(L, -2); // args mod fn
  lua_remove(L, -2); // args fn
  lua_insert(L, - nargs - 1); // fn args
  lua_call(L, nargs, nret); // results
}

static inline unsigned int tk_lua_checkunsigned (lua_State *L, int i)
{
  lua_Integer l = luaL_checkinteger(L, i);
  if (l < 0)
    luaL_error(L, "value can't be negative");
  if (l > UINT_MAX)
    luaL_error(L, "value is too large");
  return (unsigned int) l;
}

static inline int tk_lua_errno (lua_State *L, int err)
{
  lua_pushstring(L, strerror(errno));
  lua_pushinteger(L, err);
  tk_lua_callmod(L, 2, 0, "santoku.error", "error");
  return 0;
}

static inline int tk_lua_errmalloc (lua_State *L)
{
  lua_pushstring(L, "Error in malloc");
  tk_lua_callmod(L, 1, 0, "santoku.error", "error");
  return 0;
}

static int tk_system_posix_close (lua_State *L)
{
  int n = luaL_checkinteger(L, -1);
  int rc = close(n);
  if (rc == -1)
    return tk_lua_errno(L, errno);
  return 0;
}

// TODO: Use luaL_Buffer / prepbuffer
static int tk_system_posix_read (lua_State *L)
{
  int fd = luaL_checkinteger(L, -2);
  int size = luaL_checkinteger(L, -1);
  char *buf = malloc(size); // buf
  if (buf == NULL)
    return tk_lua_errmalloc(L);
  int bytes = read(fd, buf, size);
  if (bytes == -1) {
    int err = errno;
    free(buf);
    return tk_lua_errno(L, err);
  }
  lua_pushlstring(L, buf, bytes);
  free(buf);
  return 1;
}

static int tk_system_posix_setenv (lua_State *L)
{
  const char *k = luaL_checkstring(L, 1);
  const char *v = luaL_checkstring(L, 2);
  if (setenv(k, v, 1) == -1)
    return tk_lua_errno(L, errno);
  return 0;
}

static int tk_system_posix_dup2 (lua_State *L)
{
  int new = luaL_checkinteger(L, -1);
  int old = luaL_checkinteger(L, -2);
  int rc = dup2(old, new);
  if (rc == -1)
    return tk_lua_errno(L, errno);
  return 0;
}

static int tk_system_posix_pipe (lua_State *L)
{
  int fds[2];
  int rc = pipe(fds);
  if (rc == -1)
    return tk_lua_errno(L, errno);
  lua_pushinteger(L, fds[0]);
  lua_pushinteger(L, fds[1]);
  return 2;
}

static int tk_system_posix_execp (lua_State *L)
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
  return tk_lua_errno(L, errno);
}

static int tk_system_posix_fork (lua_State *L)
{
  pid_t pid = fork();
  if (pid == -1)
    return tk_lua_errno(L, errno);
  lua_pushinteger(L, pid);
  return 1;
}

static int tk_system_posix_sleep (lua_State *L)
{
  unsigned time = tk_lua_checkunsigned(L, 1);
  usleep(time * 1000); // us to ms
  return 0;
}

static int tk_system_posix_wait (lua_State *L)
{
  int pid0 = luaL_checkinteger(L, -1);
  int status;
  int pid1 = waitpid(pid0, &status, 0);
  if (pid1 == -1)
    return tk_lua_errno(L, errno);
  lua_pushinteger(L, pid1);
  if (WIFEXITED(status)) {
    lua_pushstring(L, "exited");
    lua_pushinteger(L, WEXITSTATUS(status));
    return 3;
  } else if (WIFSIGNALED(status)) {
    lua_pushstring(L, "signaled");
    lua_pushinteger(L, WTERMSIG(status));
    return 3;
  } else if (WIFSTOPPED(status)) {
    lua_pushstring(L, "stopped");
    lua_pushinteger(L, WSTOPSIG(status));
    return 3;
  } else if (WIFCONTINUED(status)) {
    lua_pushstring(L, "continued");
    return 2;
  } else {
    lua_pushstring(L, "unknown");
    lua_pushinteger(L, status);
    return 2;
  }
}

static int tk_system_posix_get_num_cores (lua_State *L)
{
  long cores = sysconf(_SC_NPROCESSORS_ONLN);
  if (cores <= 0)
    return tk_lua_errno(L, errno);
  lua_pushinteger(L, cores);
  return 1;
}

static luaL_Reg tk_system_posix_fns[] =
{
  { "get_num_cores", tk_system_posix_get_num_cores },
  { "close", tk_system_posix_close },
  { "dup2", tk_system_posix_dup2 },
  { "execp", tk_system_posix_execp },
  { "pipe", tk_system_posix_pipe },
  { "fork", tk_system_posix_fork },
  { "read", tk_system_posix_read },
  { "wait", tk_system_posix_wait },
  { "setenv", tk_system_posix_setenv },
  { "sleep", tk_system_posix_sleep },
  { NULL, NULL }
};

int luaopen_santoku_system_posix (lua_State *L)
{
  lua_newtable(L);
  luaL_register(L, NULL, tk_system_posix_fns);
  lua_pushinteger(L, BUFSIZ);
  lua_setfield(L, -2, "BUFSIZ");
  return 1;
}
