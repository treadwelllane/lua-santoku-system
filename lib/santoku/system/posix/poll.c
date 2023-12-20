/* Taken from github.com/luaposix/luaposix
 *
 * LICENSE:
 *
 * Copyright (C) 2006-2023 luaposix authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "lua.h"
#include "lauxlib.h"

#include <errno.h>
#include <sys/wait.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define tk_system_pushintegerresult(n) (lua_pushinteger(L, (n)), 1)
#define tk_system_optint(L,n,d) ((int)tk_system_expectoptinteger(L,n,d,"integer or nil"))

lua_Number tk_system_tonumberx (lua_State *L, int i, int *isnum) {
  lua_Number n = lua_tonumber(L, i);
  if (isnum != NULL) {
    *isnum = (n != 0 || lua_isnumber(L, i));
  }
  return n;
}

lua_Integer tk_system_tointegerx (lua_State *L, int i, int *isnum) {
  int ok = 0;
  lua_Number n = tk_system_tonumberx (L, i, &ok);
  if (ok) {
    if (n == (lua_Integer)n) {
      if (isnum)
        *isnum = 1;
      return (lua_Integer)n;
    }
  }
  if (isnum)
    *isnum = 0;
  return 0;
}

int tk_system_argtypeerror (lua_State *L, int narg, const char *expected)
{
	const char *got = luaL_typename(L, narg);
	return luaL_argerror(L, narg,
		lua_pushfstring(L, "%s expected, got %s", expected, got));
}

lua_Integer tk_system_expectinteger (lua_State *L, int narg, const char *expected)
{
	int isconverted = 0;
	lua_Integer d = tk_system_tointegerx(L, narg, &isconverted);
	if (!isconverted)
		tk_system_argtypeerror(L, narg, expected);
	return d;
}

lua_Integer tk_system_expectoptinteger (lua_State *L, int narg, lua_Integer def, const char *expected)
{
	if (lua_isnoneornil(L, narg))
		return def;
	return tk_system_expectinteger(L, narg, expected);
}

int tk_system_pusherror (lua_State *L, const char *info)
{
	lua_pushnil(L);
	if (info==NULL)
		lua_pushstring(L, strerror(errno));
	else
		lua_pushfstring(L, "%s: %s", info, strerror(errno));
	lua_pushinteger(L, errno);
	return 3;
}

int tk_system_pushresult (lua_State *L, int i, const char *info)
{
	if (i==-1)
		return tk_system_pusherror(L, info);
	return tk_system_pushintegerresult(i);
}

void tk_system_checknargs (lua_State *L, int maxargs)
{
	int nargs = lua_gettop(L);
	lua_pushfstring(L, "no more than %d argument%s expected, got %d",
		        maxargs, maxargs == 1 ? "" : "s", nargs);
	luaL_argcheck(L, nargs <= maxargs, maxargs + 1, lua_tostring (L, -1));
	lua_pop(L, 1);
}

struct {

	short bit;
	const char *name;

} tk_system_poll_event_map[] = {

	{ POLLIN, "IN" },
	{ POLLPRI, "PRI" },
	{ POLLOUT, "OUT" },
	{ POLLERR, "ERR" },
	{ POLLHUP, "HUP" },
	{ POLLNVAL, "NVAL" }

};

#define TK_SYSTEM_PPOLL_EVENT_NUM (sizeof(tk_system_poll_event_map) / sizeof(*tk_system_poll_event_map))

void tk_system_poll_events_createtable (lua_State *L)
{
	lua_createtable(L, 0, TK_SYSTEM_PPOLL_EVENT_NUM);
}

short tk_system_poll_events_from_table (lua_State *L, int table)
{
	short events = 0;
	size_t i;

	/* Convert to absolute index */
	if (table < 0)
		table = lua_gettop(L) + table + 1;

	for (i = 0; i < TK_SYSTEM_PPOLL_EVENT_NUM; i++)
	{
		lua_getfield(L, table, tk_system_poll_event_map[i].name);
		if (lua_toboolean(L, -1))
			events |= tk_system_poll_event_map[i].bit;
		lua_pop(L, 1);
	}

	return events;
}

void tk_system_poll_events_to_table (lua_State *L, int table, short events)
{
	size_t i;

	/* Convert to absolute index */
	if (table < 0)
		table = lua_gettop(L) + table + 1;

	for (i = 0; i < TK_SYSTEM_PPOLL_EVENT_NUM; i++)
	{
		lua_pushboolean(L, events & tk_system_poll_event_map[i].bit);
		lua_setfield(L, table, tk_system_poll_event_map[i].name);
	}
}

nfds_t tk_system_poll_fd_list_check_table (lua_State *L, int table)
{
	nfds_t fd_num = 0;

	/*
	 * Assume table is an argument number.
	 * Should be an assert(table > 0).
	 */

	luaL_checktype(L, table, LUA_TTABLE);

	/* Nil key - the one before first */
	lua_pushnil(L);

	/* Push each key/value pair, popping previous key */
	while (lua_next(L, 1) != 0)
	{
		/* Verify the fd key */
		// luaL_argcheck(L, lua_isinteger(L, -2), table,
		// 			  "contains non-integer key(s)");

		/* Verify the table value */
		luaL_argcheck(L, lua_istable(L, -1), table,
					  "contains non-table value(s)");
		lua_getfield(L, -1, "events");
		luaL_argcheck(L, lua_istable(L, -1), table,
					  "contains invalid value table(s)");
		lua_pop(L, 1);
		lua_getfield(L, -1, "revents");
		luaL_argcheck(L, lua_isnil(L, -1) || lua_istable(L, -1), table,
					  "contains invalid value table(s)");
		lua_pop(L, 1);

		/* Remove value (but leave the key) */
		lua_pop(L, 1);

		/* Count the fds */
		fd_num++;
	}

	return fd_num;
}

void tk_system_poll_fd_list_from_table (lua_State *L, int table, struct pollfd *fd_list)
{
	struct pollfd *pollfd = fd_list;

	/*
	 * Assume the table didn't change since
	 * the call to poll_fd_list_check_table
	 */

	/* Convert to absolute index */
	if (table < 0)
		table = lua_gettop(L) + table + 1;

	/* Nil key - the one before first */
	lua_pushnil(L);

	/* Push each key/value pair, popping previous key */
	while (lua_next(L, table) != 0)
	{
		/* Transfer the fd key */
		pollfd->fd = lua_tointeger(L, -2);

		/* Transfer "events" field from the value */
		lua_getfield(L, -1, "events");
		pollfd->events = tk_system_poll_events_from_table(L, -1);
		lua_pop(L, 1);

		/* Remove value (but leave the key) */
		lua_pop(L, 1);

		/* Proceed to next fd */
		pollfd++;
	}
}

void tk_system_poll_fd_list_to_table (lua_State *L, int table, const struct pollfd *fd_list)
{
	const struct pollfd *pollfd = fd_list;

	/*
	 * Assume the table didn't change since
	 * the call to poll_fd_list_check_table.
	 */

	/* Convert to absolute index */
	if (table < 0)
		table = lua_gettop(L) + table + 1;

	/* Nil key - the one before first */
	lua_pushnil(L);

	/* Push each key/value pair, popping previous key */
	while (lua_next(L, 1) != 0)
	{
		/* Transfer "revents" field to the value */
		lua_getfield(L, -1, "revents");
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			tk_system_poll_events_createtable(L);
			lua_pushvalue(L, -1);
			lua_setfield(L, -3, "revents");
		}
		tk_system_poll_events_to_table(L, -1, pollfd->revents);
		lua_pop(L, 1);

		/* Remove value (but leave the key) */
		lua_pop(L, 1);

		/* Proceed to next fd */
		pollfd++;
	}
}

int tk_system_poll (lua_State *L)
{
	struct pollfd *fd_list, static_fd_list[16];
	nfds_t fd_num = tk_system_poll_fd_list_check_table(L, 1);
	int r, timeout = tk_system_optint(L, 2, -1);
	tk_system_checknargs(L, 2);

	fd_list = (fd_num <= sizeof(static_fd_list) / sizeof(*static_fd_list))
					? static_fd_list
					: lua_newuserdata(L, sizeof(*fd_list) * fd_num);


	tk_system_poll_fd_list_from_table(L, 1, fd_list);

	r = poll(fd_list, fd_num, timeout);

	/* If any of the descriptors changed state */
	if (r > 0)
		tk_system_poll_fd_list_to_table(L, 1, fd_list);

	return tk_system_pushresult(L, r, NULL);
}

int luaopen_santoku_system_posix_poll (lua_State *L)
{
  lua_pushcfunction(L, tk_system_poll);
	return 1;
}
