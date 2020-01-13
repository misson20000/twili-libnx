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

#pragma once

#include<switch.h>

#ifdef __cplusplus
extern "C" {
#endif

// Call this before using any other libtwili functions.
Result twiliInitialize(void);

typedef struct {
	Service _srv;
} TwiliPipe;

// Creates a named output pipe with the given null-terminated name. Make sure to
// call twiliClosePipe when you're done.
Result twiliCreateNamedOutputPipe(TwiliPipe *pipe, const char* name);

// Creates a named output pipe with the given name. In most cases it will be
// more convenient to use twiliCreateNamedOutputPipe with a string literal.
// Make sure to call twiliClosePipe when you're done.
Result twiliCreateExactlyNamedOutputPipe(
	TwiliPipe *pipe,
	const char* name,
	size_t name_len);

// These functions open monitored processes' standard I/O pipes. Make sure to
// call twiliClosePipe when you're done.
Result twiliOpenStdinPipe(TwiliPipe *pipe);
Result twiliOpenStdoutPipe(TwiliPipe *pipe);
Result twiliOpenStderrPipe(TwiliPipe *pipe);

// Opens a file descriptor for the pipe. On success, takes ownership of pipe. Do
// not try to close or otherwise use the pipe object except by the returned file
// descriptor. Returns -1 on error. On error, ownership is not claimed and the
// pipe must still be closed by the caller.
int twiliBindPipe(TwiliPipe *pipe);

// Opens standard I/O pipes and dup2's them to standard file
// descriptors. Returns -1 on error.
int twiliBindStdio();

// Writes data to a pipe. The data is preserved exactly through the entire
// pipeline and is as safe for binary data as it is for logging; that is, there
// is no truncating of NULL characters or CRLF transmutations or whatnot. If
// pipe buffer is full, this call may block until space is available. If pipe
// buffering is disabled, this call will block until the data has been sent to
// the twib client.
Result twiliWritePipe(
	TwiliPipe *pipe,
	const char *data,
	size_t data_length);


// Reads data from a pipe. Blocks if none is available. It is not valid to call
// this on a named output pipe.
Result twiliReadPipe(
	TwiliPipe *pipe,
	char *buffer,
	size_t buffer_length,
	size_t *read_length);

// Closes and destroys a previously created pipe.
void twiliClosePipe(
	TwiliPipe *pipe);

// Call this during cleanup, and do not call any other libtwili functions after
// this.
void twiliExit(void);

#ifdef __cplusplus
}
#endif
