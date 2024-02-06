# Now

- pread, sh, etc should return iter, handle such that the underlying process can
  be killed with close(handle)
- Re-use chunking logic from santoku-fs to more efficiently read from processes

- Add support for reading from multiple jobs
- Add support for writing to multiple jobs (load balancing, multiplexing)
- Add general configuration options for stdin, stdout, stderr handling

# Later

- Add atexit
- Add better/more asserts and error checking
- Re-write poll
