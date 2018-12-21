/*
  Copyright (c) 2018, misson20000

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

#include<stdio.h>
#include<errno.h>
#include<switch.h>
#include<sys/iosupport.h>

#include "twili.h"

static Service g_twiliSrv;
static u64 g_twiliRefCnt;

static Result twiliOpenPipe(Service *srv_out, int id) {
	IpcCommand c;
	ipcInitialize(&c);
    
	struct {
		u64 magic;
		u64 cmd_id;
	} *raw;

	ipcSendPid(&c);
	raw = ipcPrepareHeader(&c, sizeof(*raw));

	raw->magic = SFCI_MAGIC;
	raw->cmd_id = id;

	Result rc = serviceIpcDispatch(&g_twiliSrv);
	if(R_SUCCEEDED(rc)) {
		IpcParsedCommand r;
		ipcParse(&r);

		struct {
			u64 magic;
			u64 result;
		} *resp = r.Raw;

		rc = resp->result;

		if(R_SUCCEEDED(rc)) {
			serviceCreate(srv_out, r.Handles[0]);
		}
	}

	return rc;
}

static Service g_twiliPipeStdin;
static Service g_twiliPipeStdout;
static Service g_twiliPipeStderr;

static ssize_t twiliWrite(Service *pipe, struct _reent *r, void *fd, const char *ptr, size_t len) {
	IpcCommand c;
	ipcInitialize(&c);
    
	struct {
		u64 magic;
		u64 cmd_id;
	} *raw;

	ipcAddSendBuffer(&c, ptr, len, 0);
	raw = ipcPrepareHeader(&c, sizeof(*raw));

	raw->magic = SFCI_MAGIC;
	raw->cmd_id = 1;

	Result rc = serviceIpcDispatch(pipe);
	if(R_SUCCEEDED(rc)) {
		IpcParsedCommand r;
		ipcParse(&r);

		struct {
			u64 magic;
			u64 result;
		} *resp = r.Raw;

		rc = resp->result;
	}

	if(R_SUCCEEDED(rc)) {
		return len;
	} else {
		if(r == NULL) {
			errno = EIO;
		} else {
			r->_errno = EIO;
		}
		return -1;
	}
}

static ssize_t twiliRead(Service *pipe, struct _reent *r, void *fd, char *ptr, size_t len) {
	IpcCommand c;
	ipcInitialize(&c);
    
	struct {
		u64 magic;
		u64 cmd_id;
	} *raw;

	ipcAddRecvBuffer(&c, ptr, len, 0);
	raw = ipcPrepareHeader(&c, sizeof(*raw));

	raw->magic = SFCI_MAGIC;
	raw->cmd_id = 0;

	Result rc = serviceIpcDispatch(pipe);
	if(R_SUCCEEDED(rc)) {
		IpcParsedCommand r;
		ipcParse(&r);

		struct {
			u64 magic;
			u64 result;
			u64 size;
		} *resp = r.Raw;

		rc = resp->result;
		
		if(R_SUCCEEDED(rc)) {
			return resp->size;
		}
	}
	
	if(r == NULL) {
		errno = EIO;
	} else {
		r->_errno = EIO;
	}
	return -1;
}

static ssize_t twiliReadStdin(struct _reent *r, void *fd, char *ptr, size_t len) {
	return twiliRead(&g_twiliPipeStdin, r, fd, ptr, len);
}

static ssize_t twiliWriteStdout(struct _reent *r, void *fd, const char *ptr, size_t len) {
	return twiliWrite(&g_twiliPipeStdout, r, fd, ptr, len);
}

static ssize_t twiliWriteStderr(struct _reent *r, void *fd, const char *ptr, size_t len) {
	return twiliWrite(&g_twiliPipeStderr, r, fd, ptr, len);
}

static const inline devoptab_t twiliMakeDotab(const char *name, ssize_t (*write_r)(struct _reent *r, void *fd, const char *ptr, size_t len), ssize_t (*read_r)(struct _reent *r, void *fd, char *ptr, size_t len)) {
	devoptab_t ret = {
		.name = name,
		.structSize = 0,
		.open_r = NULL,
		.close_r = NULL,
		.write_r = write_r,
		.read_r = read_r,
		.seek_r = NULL,
		.fstat_r = NULL,
		.deviceData = NULL
	};
	return ret;
}
    
Result twiliInitialize(void) {
	Result r = 0;

	atomicIncrement64(&g_twiliRefCnt);

	if(!serviceIsActive(&g_twiliSrv)) {
		r = smGetService(&g_twiliSrv, "twili");
		if(R_SUCCEEDED(r)) {
			r = twiliOpenPipe(&g_twiliPipeStdin, 0);
		}
		if(R_SUCCEEDED(r)) {
			r = twiliOpenPipe(&g_twiliPipeStdout, 1);
		}
		if(R_SUCCEEDED(r)) {
			r = twiliOpenPipe(&g_twiliPipeStderr, 2);
		}
		if(R_SUCCEEDED(r)) {
			static devoptab_t dotab_twili_stdin;
			static devoptab_t dotab_twili_stdout;
			static devoptab_t dotab_twili_stderr;
			dotab_twili_stdin  = twiliMakeDotab("twili-in",
			                                    NULL, twiliReadStdin);
			dotab_twili_stdout = twiliMakeDotab("twili-out",
			                                    twiliWriteStdout, NULL);
			dotab_twili_stderr = twiliMakeDotab("twili-err",
			                                    twiliWriteStderr, NULL);
    
			devoptab_list[STD_IN]  = &dotab_twili_stdin;
			devoptab_list[STD_OUT] = &dotab_twili_stdout;
			devoptab_list[STD_ERR] = &dotab_twili_stderr;
		}
	}

	if(R_FAILED(r)) {
		twiliExit();
	}

	return r;
}

void twiliExit(void) {
	serviceClose(&g_twiliPipeStderr);
	serviceClose(&g_twiliPipeStdout);
	serviceClose(&g_twiliPipeStdin);
	serviceClose(&g_twiliSrv);
}
