/*
  Copyright (c) 2020 misson20000

  Permission to use, copy, modify, and/or distribute this software for any purpose
  with or without fee is hereby granted, provided that the above copyright notice
  and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
  REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
  INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
  OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
  THIS SOFTWARE.
*/

#define NX_SERVICE_ASSUME_NON_DOMAIN

#include<string.h>
#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include<switch.h>
#include<sys/iosupport.h>

#include "twili.h"

static Service g_twiliSrv;

// service guard
static Mutex g_twiliMutex;
static u64 g_twiliRefCnt;

static int _twiliDotabOpen(struct _reent *r, void *fdptr, const char *path, int flags, int mode);
static int _twiliDotabClose(struct _reent *r, void *fdptr);
static ssize_t _twiliDotabWrite(struct _reent *r, void *fdptr, const char *buf, size_t count);
static ssize_t _twiliDotabRead(struct _reent *r, void *fdptr, char *buf, size_t count);

static devoptab_t g_twiliDotab = {
	.name         = "twili",
	.structSize   = sizeof(TwiliPipe),
	.open_r       = _twiliDotabOpen,
	.close_r      = _twiliDotabClose,
	.write_r      = _twiliDotabWrite,
	.read_r       = _twiliDotabRead,
	.seek_r       = NULL,
	.fstat_r      = NULL,
	.stat_r       = NULL,
	.link_r       = NULL,
	.unlink_r     = NULL,
	.chdir_r      = NULL,
	.rename_r     = NULL,
	.mkdir_r      = NULL,
	.dirStateSize = 0,
	.diropen_r    = NULL,
	.dirreset_r   = NULL,
	.dirnext_r    = NULL,
	.dirclose_r   = NULL,
	.statvfs_r    = NULL,
	.ftruncate_r  = NULL,
	.fsync_r      = NULL,
	.deviceData   = 0,
	.chmod_r      = NULL,
	.fchmod_r     = NULL,
	.rmdir_r      = NULL,
};

Result twiliInitialize(void) {
	Result r = 0;

	mutexLock(&g_twiliMutex);
	if(g_twiliRefCnt++ != 0) {
		mutexUnlock(&g_twiliMutex);
		return r;
	}

	r = smGetService(&g_twiliSrv, "twili");

	if(R_SUCCEEDED(r)) {
		int dev = AddDevice(&g_twiliDotab);
		if(dev == -1) {
			r = MAKERESULT(Module_Libnx, LibnxError_TooManyDevOpTabs);
		}
	}

	if(R_FAILED(r)) {
		twiliExit();
	}

	return r;
}

Result twiliCreateNamedOutputPipe(TwiliPipe *pipe, const char* name) {
	return twiliCreateExactlyNamedOutputPipe(pipe, name, strlen(name));
}

Result twiliCreateExactlyNamedOutputPipe(TwiliPipe *pipe, const char* name, size_t len) {
	return serviceDispatch(&g_twiliSrv, 10,
	                       .out_num_objects = 1,
	                       .out_objects = &pipe->_srv,
	                       .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
	                       .buffers = { { name, len } });
}

static Result _twiliOpenStandardPipeByCommandId(TwiliPipe *pipe, int cmd_id) {
	return serviceDispatch(&g_twiliSrv, cmd_id,
	                       .out_num_objects = 1,
	                       .out_objects = &pipe->_srv,
	                       .in_send_pid = true);
}

Result twiliOpenStdinPipe(TwiliPipe *pipe) {
	return _twiliOpenStandardPipeByCommandId(pipe, 0);
}

Result twiliOpenStdoutPipe(TwiliPipe *pipe) {
	return _twiliOpenStandardPipeByCommandId(pipe, 1);
}

Result twiliOpenStderrPipe(TwiliPipe *pipe) {
	return _twiliOpenStandardPipeByCommandId(pipe, 2);
}

int twiliBindPipe(TwiliPipe *pipe) {
	int dev = FindDevice("twili:");
	if(dev == -1) {
		return -1;
	}

	int fd = __alloc_handle(dev);
	if(fd == -1) {
		return -1;
	}

	memcpy(__get_handle(fd)->fileStruct,
	       pipe,
	       sizeof(*pipe));

	return fd;
}

int twiliBindStdio() {
	TwiliPipe tp_stdin;  bool owns_stdin  = false;
	TwiliPipe tp_stdout; bool owns_stdout = false;
	TwiliPipe tp_stderr; bool owns_stderr = false;

	Result rc = 0;

	// open twili pipes
	if(R_SUCCEEDED(rc)) {
		rc = twiliOpenStdinPipe(&tp_stdin);
		if(R_SUCCEEDED(rc)) {
			owns_stdin = true;
		}
	}
	if(R_SUCCEEDED(rc)) {
		rc = twiliOpenStdoutPipe(&tp_stdout);
		if(R_SUCCEEDED(rc)) {
			owns_stdout = true;
		}
	}
	if(R_SUCCEEDED(rc)) {
		rc = twiliOpenStderrPipe(&tp_stderr);
		if(R_SUCCEEDED(rc)) {
			owns_stderr = true;
		}
	}

	// create file descriptors
	int fd_stdin = -1;
	int fd_stdout = -1;
	int fd_stderr = -1;
	if(R_SUCCEEDED(rc)) {
		fd_stdin  = twiliBindPipe(&tp_stdin);
		fd_stdout = twiliBindPipe(&tp_stdout);
		fd_stderr = twiliBindPipe(&tp_stderr);
		if(fd_stdin  != -1) { owns_stdin  = false; } else { rc = 1; }
		if(fd_stdout != -1) { owns_stdout = false; } else { rc = 1; }
		if(fd_stderr != -1) { owns_stderr = false; } else { rc = 1; }
	}

	if(R_SUCCEEDED(rc)) {
		if(dup2(fd_stdin,  STDIN_FILENO)  != -1) { fd_stdin  = STDIN_FILENO;  } else { rc = 1; }
		if(dup2(fd_stdout, STDOUT_FILENO) != -1) { fd_stdout = STDOUT_FILENO; } else { rc = 1; }
		if(dup2(fd_stderr, STDERR_FILENO) != -1) { fd_stderr = STDERR_FILENO; } else { rc = 1; }
	}

	if(R_FAILED(rc)) {
		if(fd_stdin  != -1) { close(fd_stdin ); }
		if(fd_stdout != -1) { close(fd_stdout); }
		if(fd_stderr != -1) { close(fd_stderr); }
	}

	if(owns_stdin)  { twiliClosePipe(&tp_stdin ); }
	if(owns_stdout) { twiliClosePipe(&tp_stdout); }
	if(owns_stderr) { twiliClosePipe(&tp_stderr); }

	return R_SUCCEEDED(rc) ? 0 : -1;
}

Result twiliWritePipe(TwiliPipe *pipe, const char *data, size_t data_length) {
	return serviceDispatch(&pipe->_srv, 1,
	                       .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
	                       .buffers = { { data, data_length } });
}

Result twiliReadPipe(TwiliPipe *pipe, char *buffer, size_t buffer_length, size_t *read_length) {
	return serviceDispatchOut(&pipe->_srv, 0, *read_length,
	                          .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcMapAlias },
	                          .buffers = { { buffer, buffer_length } });
}

void twiliClosePipe(TwiliPipe *pipe) {
	serviceClose(&pipe->_srv);
}

void twiliExit(void) {
	RemoveDevice("twili:");
	serviceClose(&g_twiliSrv);
}

static inline bool _twiliAdjustErrno(Result rc, struct _reent *r) {
	if(R_FAILED(rc)) {
		if(r == NULL) {
			errno = EIO;
		} else {
			r->_errno = EIO;
		}
		return false;
	} else {
		return true;
	}
}

static int _twiliDotabOpen(struct _reent *r, void *fdptr, const char *path, int flags, int mode) {
	(void) flags;
	(void) mode;

	if(strncmp(path, "twili:", 6) == 0) { path += 6; }

	return _twiliAdjustErrno(twiliCreateNamedOutputPipe((TwiliPipe*) fdptr, path), r) ? 0 : -1;
}

static int _twiliDotabClose(struct _reent *r, void *fdptr) {
	twiliClosePipe((TwiliPipe*) fdptr);
	return 0;
}

static ssize_t _twiliDotabWrite(struct _reent *r, void *fdptr, const char *buf, size_t count) {
	return _twiliAdjustErrno(twiliWritePipe((TwiliPipe*) fdptr, buf, count), r) ? count : -1;
}

static ssize_t _twiliDotabRead(struct _reent *r, void *fdptr, char *buf, size_t count) {
	size_t actual_count;
	return _twiliAdjustErrno(twiliReadPipe((TwiliPipe*) fdptr, buf, count, &actual_count), r) ? actual_count : -1;
}
