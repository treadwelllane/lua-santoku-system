#include "lua.h"
#include "lauxlib.h"

#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>

#define MT_ATOM "santoku_atom"
#define MT_MUTEX "santoku_mutex"

static int tk_ppid (lua_State *);

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

static inline int tk_lua_error (lua_State *L, const char *err)
{
  lua_pushstring(L, err);
  tk_lua_callmod(L, 1, 0, "santoku.error", "error");
  return 0;
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

static int tk_close (lua_State *L)
{
  int n = luaL_checkinteger(L, -1);
  int rc = close(n);
  if (rc == -1)
    return tk_lua_errno(L, errno);
  return 0;
}

// TODO: Use luaL_Buffer / prepbuffer
static int tk_read (lua_State *L)
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

static int tk_setenv (lua_State *L)
{
  const char *k = luaL_checkstring(L, 1);
  const char *v = luaL_checkstring(L, 2);
  if (setenv(k, v, 1) == -1)
    return tk_lua_errno(L, errno);
  return 0;
}

static int tk_dup2 (lua_State *L)
{
  int new = luaL_checkinteger(L, -1);
  int old = luaL_checkinteger(L, -2);
  int rc = dup2(old, new);
  if (rc == -1)
    return tk_lua_errno(L, errno);
  return 0;
}

static int tk_pipe (lua_State *L)
{
  int fds[2];
  int rc = pipe(fds);
  if (rc == -1)
    return tk_lua_errno(L, errno);
  lua_pushinteger(L, fds[0]);
  lua_pushinteger(L, fds[1]);
  return 2;
}

static int tk_execp (lua_State *L)
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

static int tk_fork (lua_State *L)
{
  lua_settop(L, 0);
  pid_t ppid = getpid();
  pid_t pid = fork();
  if (pid == -1)
    return tk_lua_errno(L, errno);
  if (pid != 0) {
    lua_pushinteger(L, pid);
    return 1;
  }
  if (prctl(PR_SET_PDEATHSIG, SIGHUP))
    return tk_lua_errno(L, errno);
  tk_ppid(L);
  pid_t ppid0 = (pid_t) luaL_checkinteger(L, -1);
  lua_pop(L, 1);
  if (ppid != ppid0)
    return tk_lua_error(L, "parent exited before prctl");
  lua_pushinteger(L, pid);
  return 1;
}

static int tk_sleep (lua_State *L)
{
  time_t time = luaL_checkinteger(L, 1);
  useconds_t subsec = (luaL_checknumber(L, 1) - time) * 1000000.0;
  if (time >= 1)
    sleep(time);
  usleep(subsec);
  return 0;
}

static int tk_wait (lua_State *L)
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

static int tk_get_num_cores (lua_State *L)
{
  long cores = sysconf(_SC_NPROCESSORS_ONLN);
  if (cores <= 0)
    return tk_lua_errno(L, errno);
  lua_pushinteger(L, cores);
  return 1;
}

static int tk_pid (lua_State *L) {
  pid_t pid = getpid();
  lua_pushinteger(L, pid);
  return 1;
}

// TODO: only works on linux!
static int tk_ppid (lua_State *L) {
  lua_Integer n = luaL_optinteger(L, 1, -1);
  pid_t pid = n < 0 ? getpid() : n;
  int ppid;
  char buf[BUFSIZ * 2];
  char procname[32];
  FILE *fp;
  snprintf(procname, sizeof(procname), "/proc/%u/status", pid);
  fp = fopen(procname, "r");
  if (fp != NULL) {
    size_t ret = fread(buf, sizeof(char), BUFSIZ * 2 - 1, fp);
    if (!ret) {
      // should error here?
      return 0;
    } else {
      buf[ret++] = '\0';  // Terminate it.
    }
  }
  fclose(fp);
  char *ppid_loc = strstr(buf, "\nPPid:");
  if (ppid_loc) {
    int ret = sscanf(ppid_loc, "\nPPid:%d", &ppid);
    if (!ret || ret == EOF) {
      return 1;
    }
    lua_pushinteger(L, ppid);
    return 1;
  } else {
    // should error here?
    return 0;
  }
}

typedef struct {
  lua_Number value;
  lua_Number last;
} tk_atom_data_t;

typedef struct {
  sem_t *sem;
  lua_Number throttle;
  lua_Number last;
  tk_atom_data_t *data;
} tk_atom_t;

static tk_atom_t *peek_atom (lua_State *L, int i)
{
  return (tk_atom_t *) luaL_checkudata(L, i, MT_ATOM);
}

static int tk_atom_destroy (lua_State *L)
{
  tk_atom_t *atomp = peek_atom(L, 1);
  sem_close(atomp->sem);
  munmap(atomp->data, sizeof(tk_atom_data_t));
  return 0;
}

static int tk_atom_closure (lua_State *L)
{
  lua_Number inc = luaL_optnumber(L, 1, 1);
  tk_atom_t *atomp = peek_atom(L, lua_upvalueindex(1));
  lua_settop(L, 0);

  if (atomp->throttle > 0) {

    sem_wait(atomp->sem);

    lua_Number last = atomp->data->last;

    lua_pushboolean(L, true);
    tk_lua_callmod(L, 1, 1, "santoku.utc", "time");
    lua_Number now = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_Number diff = now - last;
    if (diff < atomp->throttle) {
      lua_pushnumber(L, atomp->throttle - diff);
      tk_lua_callmod(L, 1, 0, "santoku.system", "sleep");
    }

    lua_pushboolean(L, true);
    tk_lua_callmod(L, 1, 1, "santoku.utc", "time");
    now = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_Number old = atomp->data->value;
    atomp->data->value = old + inc;
    atomp->data->last = now;
    sem_post(atomp->sem);
    lua_pushnumber(L, old);

  } else {

    sem_wait(atomp->sem);
    lua_Number old = atomp->data->value;
    atomp->data->value = old + inc;
    sem_post(atomp->sem);
    lua_pushnumber(L, old);

  }

  return 1;
}

static int tk_atom (lua_State *L)
{
  lua_Number def = luaL_optnumber(L, 1, 1);
  lua_Number throttle = luaL_optnumber(L, 2, -1);
  lua_settop(L, 0);

  lua_pushinteger(L, 32);
  lua_pushinteger(L, 97);
  lua_pushinteger(L, 122);
  tk_lua_callmod(L, 3, 1, "santoku.random", "str");

  lua_pushinteger(L, 32);
  lua_pushinteger(L, 97);
  lua_pushinteger(L, 122);
  tk_lua_callmod(L, 3, 1, "santoku.random", "str");

  char shm_path[33];
  char sem_path[33];

  sprintf(shm_path, "/%s", luaL_checkstring(L, 1));
  sprintf(sem_path, "/%s", luaL_checkstring(L, 2));

  int shm_fd = shm_open(shm_path, O_CREAT | O_RDWR, 0666);

  if (shm_fd == -1)
    return tk_lua_errno(L, errno);

  if (shm_unlink(shm_path))
    return tk_lua_errno(L, errno);

  if (ftruncate(shm_fd, sizeof(tk_atom_data_t)) == -1)
    return tk_lua_errno(L, errno);

  tk_atom_data_t *data = mmap(NULL, sizeof(tk_atom_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (data == MAP_FAILED)
    return tk_lua_errno(L, errno);

  data->value = def;
  data->last = 0;
  close(shm_fd);

  sem_t* sem = sem_open(sem_path, O_CREAT, 0666, 1);

  if (sem == SEM_FAILED)
    return tk_lua_errno(L, errno);

  if (sem_unlink(sem_path))
    return tk_lua_errno(L, errno);

  tk_atom_t *atomp = lua_newuserdata(L, sizeof(tk_atom_t));
  atomp->sem = sem;
  atomp->throttle = throttle;
  atomp->data = data;
  luaL_getmetatable(L, MT_ATOM);
  lua_setmetatable(L, -2);
  lua_pushcclosure(L, tk_atom_closure, 1);
  return 1;
}

typedef struct {
  sem_t *sem;
  char *sem_path;
} tk_mutex_t;

static tk_mutex_t *peek_mutex (lua_State *L, int i)
{
  return (tk_mutex_t *) luaL_checkudata(L, i, MT_MUTEX);
}

static int tk_mutex_destroy (lua_State *L)
{
  tk_mutex_t *mutexp = peek_mutex(L, 1);
  sem_close(mutexp->sem);
  return 0;
}

static int tk_mutex_closure (lua_State *L)
{
  if (lua_gettop(L) == 0)
    return 0;
  tk_mutex_t *mutexp = peek_mutex(L, lua_upvalueindex(1));
  // fn ...args
  sem_wait(mutexp->sem);
  tk_lua_callmod(L, lua_gettop(L), LUA_MULTRET, "santoku.error", "pcall"); // ok ...ret
  sem_post(mutexp->sem);
  tk_lua_callmod(L, lua_gettop(L), LUA_MULTRET, "santoku.error", "checkok"); // ...ret
  return lua_gettop(L);
}

static int tk_mutex (lua_State *L)
{
  lua_settop(L, 0);

  lua_pushinteger(L, 32);
  lua_pushinteger(L, 97);
  lua_pushinteger(L, 122);
  tk_lua_callmod(L, 3, 1, "santoku.random", "str"); // path

  char sem_path[33];

  sprintf(sem_path, "/%s", luaL_checkstring(L, 1)); // path
  lua_pop(L, 1); //

  sem_t* sem = sem_open(sem_path, O_CREAT, 0666, 1);

  if (sem == SEM_FAILED)
    return tk_lua_errno(L, errno);

  if (sem_unlink(sem_path))
    return tk_lua_errno(L, errno);

  tk_mutex_t *mutexp = lua_newuserdata(L, sizeof(tk_mutex_t)); // t
  mutexp->sem = sem;
  luaL_getmetatable(L, MT_MUTEX); // t mt
  lua_setmetatable(L, -2); // t
  lua_pushcclosure(L, tk_mutex_closure, 1); // fn
  return 1;
}

static luaL_Reg tk_fns[] =
{
  { "get_num_cores", tk_get_num_cores },
  { "close", tk_close },
  { "dup2", tk_dup2 },
  { "execp", tk_execp },
  { "pipe", tk_pipe },
  { "fork", tk_fork },
  { "read", tk_read },
  { "wait", tk_wait },
  { "setenv", tk_setenv },
  { "sleep", tk_sleep },
  { "pid", tk_pid },
  { "ppid", tk_ppid },
  { "atom", tk_atom },
  { "mutex", tk_mutex },
  { NULL, NULL }
};

int luaopen_santoku_system_posix (lua_State *L)
{
  lua_newtable(L);
  luaL_register(L, NULL, tk_fns);
  lua_pushinteger(L, BUFSIZ);
  lua_setfield(L, -2, "BUFSIZ");
  luaL_newmetatable(L, MT_ATOM); // t mt
  lua_pushcfunction(L, tk_atom_destroy); // t mt fn
  lua_setfield(L, -2, "__gc"); // t mt
  lua_pop(L, 1); // t
  luaL_newmetatable(L, MT_MUTEX); // t mt
  lua_pushcfunction(L, tk_mutex_destroy); // t mt fn
  lua_setfield(L, -2, "__gc"); // t mt
  lua_pop(L, 1); // t
  return 1;
}
