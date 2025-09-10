# Santoku System

System-level operations for Lua including process management, shell command execution, and POSIX system calls.

## Module Reference

### `santoku.system`

Main system module providing process execution and shell command utilities.

| Function | Arguments | Returns | Description |
|----------|-----------|---------|-------------|
| `execute` | `opts` | `nil` | Execute command and print output line by line |
| `sh` | `opts` | `iterator` | Execute shell command with line-buffered output |
| `pread` | `opts` | `iterator` | Low-level process reading with chunked output |

Additionally inherits all functions from `santoku.system.posix` and `os` modules.

### `santoku.system.sh`

Line-buffered shell command execution.

#### Options Table

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `[1]` | `string` | required | Command to execute |
| `[2...]` | `string` | - | Command arguments |
| `jobs` | `number` | `1` | Number of parallel jobs |
| `env` | `table` | - | Environment variables |
| `job_var` | `string` | - | Environment variable for job number |
| `bufsize` | `number` | `BUFSIZ` | Buffer size for reading |

#### Returns

Iterator function yielding output lines (strings).

### `santoku.system.pread`

Low-level process execution with event-based output handling.

#### Options Table

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `[1]` | `string` | - | Command to execute |
| `[2...]` | `string` | - | Command arguments |
| `stdout` | `boolean` | `true` | Capture stdout |
| `stderr` | `boolean` | `false` | Capture stderr |
| `jobs` | `number` | `1` | Number of parallel jobs |
| `execute` | `boolean` | `false` | Execute mode flag |
| `env` | `table` | - | Environment variables |
| `job_var` | `string` | - | Environment variable for job number |
| `fn` | `function` | - | Function to execute instead of command |
| `bufsize` | `number` | `BUFSIZ` | Buffer size for reading |

#### Returns

Iterator function yielding events:

| Event | Arguments | Description |
|-------|-----------|-------------|
| `"stdout"` | `pid, data` | Stdout data from process |
| `"stderr"` | `pid, data` | Stderr data from process |
| `"exit"` | `pid, reason, status` | Process exit event |

### `santoku.system.posix`

POSIX system call bindings.

| Function | Arguments | Returns | Description |
|----------|-----------|---------|-------------|
| `pipe` | `-` | `read_fd, write_fd` | Create a pipe |
| `fork` | `-` | `pid` | Fork current process |
| `read` | `fd, size` | `data` | Read from file descriptor |
| `close` | `fd` | `-` | Close file descriptor |
| `setenv` | `key, value` | `-` | Set environment variable |
| `dup2` | `oldfd, newfd` | `-` | Duplicate file descriptor |
| `execp` | `program, args` | `-` | Execute program (replaces process) |
| `wait` | `pid` | `pid, reason, status` | Wait for process |
| `BUFSIZ` | - | `number` | System buffer size constant |

### `santoku.system.posix.poll`

File descriptor polling support (if available).

| Function | Arguments | Returns | Description |
|----------|-----------|---------|-------------|
| `poll` | `fds` | `-` | Poll file descriptors for events |

## License

Copyright 2025 Matthew Brooks

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.