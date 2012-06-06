/* 
Copyright (c) 2012, Nicholai Main
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef PIPER_H
#define PIPER_H

#include <stdio.h>
#include <windows.h>
#include <vfw.h>

extern HINSTANCE thisinstance;

#define CMDLINE_SIZE 512

// running data
typedef struct
{
  char cmdline[CMDLINE_SIZE]; // before substitution, the command to run
  HANDLE proc; // handle to the running process
  HANDLE thread; // handle to the main thread of the running process
  FILE *in; // the process's stdin
  HANDLE hout; // thread serving process's stdout
  HANDLE herr; // thread serving process's stderr
  int fpsnum;
  int fpsden;
} pipe_instance_t;

void loadcfg (pipe_instance_t *this);
void savecfg (pipe_instance_t *this);

void configure (pipe_instance_t *this, HWND parent);
int compress_query (pipe_instance_t *this, PBITMAPINFO bin, PBITMAPINFO bout);
int compress_get_format (pipe_instance_t *this, PBITMAPINFO bin, PBITMAPINFO bout);
int compress_max_size (pipe_instance_t *this, PBITMAPINFO bin, PBITMAPINFO bout);
int compress_start (pipe_instance_t *this, PBITMAPINFO bin, PBITMAPINFO bout);
int compress (pipe_instance_t *this, ICCOMPRESS * icc, size_t iccsz);
void compress_finish (pipe_instance_t *this);


#endif // PIPER_H

