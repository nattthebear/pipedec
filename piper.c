/* 
Copyright (c) 2012, 2014 Nicholai Main
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

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "piper.h"

#include <io.h> // _open_osfhandle

// some programs seem to blow up at one point or another if every video frame is 0 bytes
#define DUMMY_SIZE 4 // how much data will be inserted into each frame of the avi

#include "samplecfg.h"

int save_cfg_defaults (void)
{
  char *location = malloc (MAX_PATH);

  snprintf (location, MAX_PATH, "%s\\pipedec\\.pipedec", getenv ("APPDATA"));
  FILE *cfgfile = fopen (location, "w");

  if (!cfgfile)
  {
    // try to make directory, then try again?
    snprintf (location, MAX_PATH, "%s\\pipedec", getenv ("APPDATA"));
    CreateDirectory (location, NULL);
    snprintf (location, MAX_PATH, "%s\\pipedec\\.pipedec", getenv ("APPDATA"));
    cfgfile = fopen (location, "w");
  }
  
  
  if (cfgfile)
  {
    fputs (default_cfg, cfgfile);
    fclose (cfgfile);
  }
  else
  {
    free (location);
    return 0;  
  }
  free (location);
  return 1;
}


int load_cfg (pipe_instance_t *this, const BITMAPINFOHEADER *h)
{
  char *location = malloc (MAX_PATH);

  snprintf (location, MAX_PATH, "%s\\pipedec\\.pipedec", getenv ("APPDATA"));
  FILE *cfgfile = fopen (location, "r");

  if (!cfgfile)
  {
    if (!save_cfg_defaults ())
    {
      MessageBox (NULL, "Couldn't write config file", "Pipe Codec", 0);
      return 0;
    }
    MessageBox (NULL, "Default configuration written", "Pipe Codec", 0);
    cfgfile = fopen (location, "r");
    if (!cfgfile)
    {
      MessageBox (NULL, "Couldn't read config file", "Pipe Codec", 0);
      return 0;
    }
  }
  
  char fourcc[8];
  switch (h->biCompression)
  {
    case BI_RGB:
      if (h->biHeight < 0)
        // top down
        snprintf (fourcc, 8, "!vGB%02i:", h->biBitCount);
      else
        snprintf (fourcc, 8, "!RGB%02i:", h->biBitCount);
      break;
    case BI_RLE8:
      snprintf (fourcc, 8, "!RLE08:");
      break;
    case BI_RLE4:
      snprintf (fourcc, 8, "!RLE04:");
      break;
    case BI_BITFIELDS:
      if (h->biHeight < 0)
        // top down
        snprintf (fourcc, 8, "!vIT%02i:", h->biBitCount);
      else
        snprintf (fourcc, 8, "!BIT%02i:", h->biBitCount);
      break;
    case BI_JPEG:
      snprintf (fourcc, 8, "!PNG00:");
      break;
    case BI_PNG:
      snprintf (fourcc, 8, "!JPEG0:");
      break;
    default:
      // assumed to be arbitrary vidc fourcc
      snprintf (fourcc, 8, "@%c%c%c%c@:",
        (int) (h->biCompression & 0xff),
        (int) (h->biCompression >>  8 & 0xff),
        (int) (h->biCompression >> 16 & 0xff),
        (int) (h->biCompression >> 24));
      break;
  }

  char *cmdline = malloc (CMDLINE_SIZE);
  while (fgets (cmdline, CMDLINE_SIZE, cfgfile))
  {
    if (strncmp (cmdline, fourcc, 7) == 0)
    {
      // strip newline
      char *loc = strchr (cmdline, '\n');
      if (!loc)
        // line too long?
        continue;
      *loc = 0;
      strcpy (this->cmdline, cmdline + 7);
      free (location);
      free (cmdline);
      fclose (cfgfile);
      return 1;
    }
  }

  sprintf (cmdline, "Couldn't find handler for \"%s\"", fourcc);
  MessageBox (NULL, cmdline, "Pipe Codec", 0);
  
  
  free (location);
  free (cmdline);
  fclose (cfgfile); 
  return 0;
}
  
  

void parseit (pipe_instance_t *this, char *out, int w, int h)
{
  // %w width (px)
  // %h height (px)
  // %n fpsnumerator
  // %d fpsdenomenator
  // %u time of creation (unix epoch)
  // %t time of creation (human readable)
  // %i sequential identifier (unique per session)
  // %% single '%' sign
  const char *in = this->cmdline;
  
  static int init = 0;
  static unsigned id = 0;
  if (!init)
  {
    init = 1;
	srand (time (NULL));
	id = rand ();
  }
  
  while (*in)
  {
    if (*in == '%')
    {
      switch (in[1])
      {
        case 'w':
          out += sprintf (out, "%u", w);
          break;
        case 'h':
          out += sprintf (out, "%u", h);
          break;
        case 'n':
          out += sprintf (out, "%u", this->fpsnum);
          break;
        case 'd':
          out += sprintf (out, "%u", this->fpsden);
          break;
        case 'u':
          out += sprintf (out, "%I64u", (unsigned long long) time (NULL));
          break;
        case 't':
        {
          time_t unixtime;
          time (&unixtime);
          char *humantime = ctime (&unixtime);
          // strip newline
          *strchr (humantime, '\n') = 0;
          // replace both colons with dots
          *strchr (humantime, ':') = '.';
          *strchr (humantime, ':') = '.';
          out += sprintf (out, "%s", humantime);
          break;
        }
		case 'i':
		  out += sprintf (out, "%u", id++);
		  break;
        case '%':
          out += sprintf (out, "%%");
          break;
        default:
          // eat it, do nothing
          break;
      }
      in += 2;
    }
    else
    {
      *out++ = *in++;
    }
  }
  *out = 0;  
}



DWORD WINAPI dumpthreadout (LPVOID lpParam)
{ // eats all input from FILE (and dumps it to disk?)
  FILE *f = (FILE *) lpParam;
  char *location = malloc (MAX_PATH);
  snprintf (location, MAX_PATH, "%s\\pipedec\\stdout.txt", getenv ("APPDATA"));
  FILE *fout = fopen (location, "w");
  free (location);

  char buff[256];
  while (fwrite (buff, 1, fread (buff, 1, 256, f), fout) == 256)
    ;
  fclose (fout);
  fclose (f);
  if (feof (f))
    return 1;
  return 2;
}
DWORD WINAPI dumpthreaderr (LPVOID lpParam)
{ // eats all input from FILE (and dumps it to disk?)
  FILE *f = (FILE *) lpParam;
  char *location = malloc (MAX_PATH);
  snprintf (location, MAX_PATH, "%s\\pipedec\\stderr.txt", getenv ("APPDATA"));
  FILE *fout = fopen (location, "w");
  free (location);

  char buff[256];
  while (fwrite (buff, 1, fread (buff, 1, 256, f), fout) == 256)
    ;
  fclose (fout);
  fclose (f);
  if (feof (f))
    return 1;
  return 2;
}

static void closepipe (pipe_instance_t *this)
{
  if (this->thread == INVALID_HANDLE_VALUE)
    return; // we were never opened

  fclose (this->in);

  while (WaitForSingleObject (this->proc, INFINITE))
    MessageBox (NULL, "Spurious", "Spurious", 0);

  CloseHandle (this->proc);
  CloseHandle (this->thread);

  while (WaitForSingleObject (this->hout, INFINITE))
    MessageBox (NULL, "Spurious", "Spurious", 0);
  while (WaitForSingleObject (this->herr, INFINITE))
    MessageBox (NULL, "Spurious", "Spurious", 0);
  
  CloseHandle (this->hout);
  CloseHandle (this->herr);
  this->in = this->proc = this->thread = this->hout = this->herr = NULL;
  
}

static char cmdparse[CMDLINE_SIZE * 2];  

static int openpipe (pipe_instance_t *this, int w, int h)
{
  parseit (this, cmdparse, w, h);
  
  //MessageBox (NULL, cmdparse, "Pipe Codec: Starting Capture", 0);
  
  FILE *fin = NULL;
  FILE *fout = NULL;
  FILE *ferr = NULL;
  HANDLE child_hin = INVALID_HANDLE_VALUE;
  HANDLE child_hout = INVALID_HANDLE_VALUE;
  HANDLE child_herr = INVALID_HANDLE_VALUE;
  HANDLE parent_hin = INVALID_HANDLE_VALUE;
  HANDLE parent_hout = INVALID_HANDLE_VALUE;
  HANDLE parent_herr = INVALID_HANDLE_VALUE;

  HANDLE thread_hout = INVALID_HANDLE_VALUE;
  HANDLE thread_herr = INVALID_HANDLE_VALUE;
  
  this->proc = INVALID_HANDLE_VALUE;
  this->thread = INVALID_HANDLE_VALUE;

  
  PROCESS_INFORMATION piProcInfo;
  STARTUPINFO siStartInfo;
  SECURITY_ATTRIBUTES sa;


  // make the pipes

  sa.nLength = sizeof (sa);
  sa.bInheritHandle = 1;
  sa.lpSecurityDescriptor = NULL;
  if (!CreatePipe (&child_hin, &parent_hin, &sa, 10 * 1024 * 1024)) // suggest 10MB buffer
    goto fail;
  if (!CreatePipe (&parent_hout, &child_hout, &sa, 0))
    goto fail;
  if (!CreatePipe (&parent_herr, &child_herr, &sa, 0))
    goto fail;


  // very important
  if (!SetHandleInformation (parent_hin, HANDLE_FLAG_INHERIT, 0))
    goto fail;
  if (!SetHandleInformation (parent_hout, HANDLE_FLAG_INHERIT, 0))
    goto fail;
  if (!SetHandleInformation (parent_herr, HANDLE_FLAG_INHERIT, 0))
    goto fail;




  // start the child process

  ZeroMemory (&siStartInfo, sizeof (STARTUPINFO));
  siStartInfo.cb         = sizeof (STARTUPINFO);
  siStartInfo.hStdInput  = child_hin;
  siStartInfo.hStdOutput = child_hout;
  siStartInfo.hStdError  = child_herr;
  siStartInfo.dwFlags    = STARTF_USESTDHANDLES;

  if (!CreateProcess(NULL,// application name
       (LPTSTR)cmdparse,  // command line
       NULL,              // process security attributes
       NULL,              // primary thread security attributes
       TRUE,              // handles are inherited
       DETACHED_PROCESS,  // creation flags
       NULL,              // use parent's environment
       NULL,              // use parent's current directory
       &siStartInfo,      // STARTUPINFO pointer
       &piProcInfo))      // receives PROCESS_INFORMATION
  {
    goto fail;
  }



  this->proc = piProcInfo.hProcess;
  this->thread = piProcInfo.hThread;
  
  
                                // what the hell is this cast for
  if (NULL == (fin = _fdopen (_open_osfhandle ((intptr_t) parent_hin, 0), "wb")))
    goto fail;
  if (NULL == (fout = _fdopen (_open_osfhandle ((intptr_t) parent_hout, 0), "r")))
    goto fail;
  if (NULL == (ferr = _fdopen (_open_osfhandle ((intptr_t) parent_herr, 0), "r")))
    goto fail;
  // after fdopen(osf()), we don't need to keep track of parent handles anymore
  // fclose on the FILE struct will automatically free them

  // spawn child information
  thread_hout = CreateThread (NULL, 0, dumpthreadout, fout, 0, NULL);
  if (!thread_hout)
  {
    MessageBox (NULL, "This is exceptionally bad.", "Bad", 0);   
    goto fail;
  }
  thread_herr = CreateThread (NULL, 0, dumpthreaderr, ferr, 0, NULL);
  if (!thread_herr)
  {
    MessageBox (NULL, "This is exceptionally bad.", "Bad", 0);   
    goto fail;
  }
  
  this->in = fin;
  this->hout = thread_hout;
  this->herr = thread_herr;

  CloseHandle (child_hin);
  CloseHandle (child_hout);
  CloseHandle (child_herr);

  return 1;

  fail:
  if (fin)
    fclose (fin);
  if (fout)
    fclose (fout);
  if (ferr)
    fclose (ferr);

  if (this->proc)
    CloseHandle (this->proc);
  if (this->thread)
    CloseHandle (this->thread);

  if (child_hin != INVALID_HANDLE_VALUE)
    CloseHandle (child_hin);
  if (child_hout != INVALID_HANDLE_VALUE)
    CloseHandle (child_hout);
  if (child_herr != INVALID_HANDLE_VALUE)
    CloseHandle (child_herr);
  if (parent_hin != INVALID_HANDLE_VALUE)
    CloseHandle (parent_hin);
  if (parent_hout != INVALID_HANDLE_VALUE)
    CloseHandle (parent_hout);
  if (parent_herr != INVALID_HANDLE_VALUE)
    CloseHandle (parent_herr);

  return 0;

}


// byte size of image data
// usually just the biSizeImage member
static int compute_size (const BITMAPINFOHEADER *h)
{
  if (h->biSizeImage)
    return h->biSizeImage;
  if (h->biCompression == BI_RGB)
  {
    int pitch = h->biWidth * h->biBitCount / 8;
    // adjust to mod 4
    pitch = (pitch + 3) & ~3;
    // height < 0 means vflip, so strip it here
    if (h->biHeight < 0)
      return pitch * -h->biHeight;
    else
      return pitch * h->biHeight;
  }
  // only BI_RGB allows non-specified size?
  return -1;
}

int compress_query (pipe_instance_t *this, PBITMAPINFO bin, PBITMAPINFO bout)
{
  if (compute_size (&bin->bmiHeader) < 0)
    return 0;

  // try to load the appropriate configurator  
  if (!load_cfg (this, &bin->bmiHeader))
    return 0;

  // don't examine bout
  // compressor will claim to be able to compress "to" anything
  return 1;
}



int compress_get_format (pipe_instance_t *this, PBITMAPINFO bin, PBITMAPINFO bout)
{
  if (!compress_query (this, bin, bout))
    return 0;
    
  // this is mostly to fool things
  bout->bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
  bout->bmiHeader.biWidth = bin->bmiHeader.biWidth;
  bout->bmiHeader.biHeight = bin->bmiHeader.biHeight;
  bout->bmiHeader.biPlanes = 1;
  bout->bmiHeader.biBitCount = 24;
  bout->bmiHeader.biCompression = mmioFOURCC ('P', 'I', 'P', 'E');
  bout->bmiHeader.biSizeImage = DUMMY_SIZE;
  bout->bmiHeader.biXPelsPerMeter = bin->bmiHeader.biXPelsPerMeter;
  bout->bmiHeader.biYPelsPerMeter = bin->bmiHeader.biYPelsPerMeter;  
  bout->bmiHeader.biClrUsed = 0;
  bout->bmiHeader.biClrImportant = 0;
  return 1;
}

int compress_max_size (pipe_instance_t *this, PBITMAPINFO bin, PBITMAPINFO bout)
{
  return DUMMY_SIZE;
}

int compress_start (pipe_instance_t *this, PBITMAPINFO bin, PBITMAPINFO bout)
{
  if (!compress_query (this, bin, bout))
    return 0;

  int w = bin->bmiHeader.biWidth;
  int h = bin->bmiHeader.biHeight;
    
    
  if (!openpipe (this, w, h))
    return 0;


  return 1;
    
}

int compress (pipe_instance_t *this, ICCOMPRESS * icc, size_t iccsz)
{
  int sz = compute_size (icc->lpbiInput);
  if (sz < 0)
    return 0;
  
  icc->lpbiOutput->biSizeImage = DUMMY_SIZE;

  // are we fooling anybody with this?
  *icc->lpdwFlags = AVIIF_KEYFRAME;
  
  
  if (fwrite (icc->lpInput, 1, sz, this->in) != sz)
    return 0;
  
  return 1;
}

void compress_finish (pipe_instance_t *this)
{
  closepipe (this);
}



