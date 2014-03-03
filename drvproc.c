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
#include <vfw.h>
#include <stdlib.h>
#include <stdio.h>

#include "piper.h"

// mingw header problem
#ifndef ICVERSION
#define ICVERSION 0x0104
#endif

HINSTANCE thisinstance;

BOOL WINAPI DllMain (HINSTANCE hinst, DWORD reason, LPVOID lpReserved)
{
  thisinstance = hinst;
  return 1;
}


LRESULT WINAPI DriverProc (DWORD_PTR dwDriverID, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2)
{
  //static FILE *dbgf = NULL;
  //if (!dbgf)
  //  dbgf = fopen ("logging.txt", "w");
  //fprintf (stdout, "DriverProc %08x %08x %08x %08x %08x\n", (int) dwDriverID, (int) hDriver, (int) uiMessage, (int) lParam1, (int) lParam2);

  pipe_instance_t *this = (pipe_instance_t *) dwDriverID;

  
  switch (uiMessage)
  {
    case DRV_DISABLE:
    case DRV_ENABLE:
    case DRV_INSTALL:
    case DRV_REMOVE:
      return 1;

    case DRV_OPEN:
      // return value sent as dwDriverID to subsequent messages
      {
        pipe_instance_t *new = malloc (sizeof (*new));
        if (new)
        {
          strncpy (new->cmdline, "cfg load failed?", CMDLINE_SIZE);
          new->proc = INVALID_HANDLE_VALUE;
          new->thread = INVALID_HANDLE_VALUE;
          new->in = NULL;
          new->hout = INVALID_HANDLE_VALUE;
          new->herr = INVALID_HANDLE_VALUE;
          new->fpsnum = 25;
          new->fpsden = 1;          
          //loadcfg (new);
        }
        return (LRESULT) new;
      }
        
      
    case DRV_CLOSE:
      // make sure to stop any currently running compression (??)
      if (this)
      {
        free (this);
      }
      return 1;
      
    case DRV_QUERYCONFIGURE:
      //  configuration from the drivers applet
      return 0; // unsupported
      
    case DRV_CONFIGURE:
      return DRVCNF_OK; // shouldn't ever get this message


    case ICM_GETDEFAULTQUALITY:
    case ICM_GETQUALITY:
      // we don't really use this
      if (lParam1)
        *(DWORD *) lParam1 = 10000; // "max" quality
      return ICERR_OK;

    // we're stateless (settings depend only on the input format)
    case ICM_GETSTATE:
    case ICM_SETSTATE:
      return 0;
      
    case ICM_GETINFO:
      if (lParam1)
      {
        if (lParam2 < sizeof (ICINFO))
          // different structure? not my problem
          return ICERR_UNSUPPORTED;
        ICINFO *inf = (ICINFO *) lParam1;
        inf->dwSize = sizeof (ICINFO);
        inf->fccType = mmioFOURCC ('V', 'I', 'D', 'C');
        inf->fccHandler = mmioFOURCC ('P', 'I', 'P', 'E');
        inf->dwFlags = VIDCF_FASTTEMPORALC | VIDCF_TEMPORAL;
        inf->dwVersion = 1; // arbitrary version number
        inf->dwVersionICM = ICVERSION;
        MultiByteToWideChar (CP_UTF8, 0, "Pipe Codec", -1, inf->szName, sizeof (inf->szName) / sizeof (WCHAR));
        MultiByteToWideChar (CP_UTF8, 0, "Pipe Codec", -1, inf->szDescription, sizeof (inf->szDescription) / sizeof (WCHAR));
        return sizeof (ICINFO);
      }
      return sizeof (ICINFO);
      
      
    case ICM_CONFIGURE:
      // if param1 = -1, return ICERR_OK if you have a config box or ICERR_UNSUPPORTED otherwise
      // otherwise, param1 = parent HWND
      if (lParam1 == -1)
        return ICERR_OK;
      else
      {
        //configure (this, (HWND) lParam1);
        MessageBox ((HWND) lParam1, "Check %APPDATA%\\pipedec\\.pipedec for configuration.\nIf it does not exist, a sample configuration will be created after a format query is received.", "Pipe Codec Configuration", 0);
        return ICERR_OK;
      }
      
    case ICM_ABOUT:  
      // if param1 = -1, return ICERR_OK if you have an about box or ICERR_UNSUPPORTED otherwise
      // otherwise, param1 = parent HWND
      if (lParam1 == -1)
        return ICERR_OK;
      else
      {
        #include "lictext.h"
        MessageBox ((HWND) lParam1, lick_text, "About Pipe Codec", 0);
        return ICERR_OK;
      }
      
    case ICM_COMPRESS_QUERY:
      // 1 = PBITMAPINFO in
      // 2 = PBITMAPINFO out (or null)
      // can you compress 1->2? ICERR_OK or ICERR_BADFORMAT
      if (compress_query (this, (PBITMAPINFO) lParam1, (PBITMAPINFO) lParam2))
        return ICERR_OK;
      else
        return ICERR_BADFORMAT;
      
    case ICM_COMPRESS_GET_FORMAT:
      // 1 = PBITMAPINFO in
      // 2 = PBITMAPINFO out
      // populate 2 with something you can compress to
      if (!lParam2)
        return sizeof (BITMAPINFO);
      if (compress_get_format (this, (PBITMAPINFO) lParam1, (PBITMAPINFO) lParam2))
        return ICERR_OK;
      else
        return ICERR_BADFORMAT;
      
    case ICM_COMPRESS_GET_SIZE:
      // 1 = PBITMAPINFO in
      // 2 = PBITMAPINFO out
      // return max size of compressed frame (worst case)
      return compress_max_size (this, (PBITMAPINFO) lParam1, (PBITMAPINFO) lParam2);
      
      
    case ICM_COMPRESS_FRAMES_INFO: // zmbv does not respond to this
      // 1 = ICCOMPRESSFRAMES *
      // 2 = sizeof (ICCOMPRESSFRAMES)
      // set parameters for upcoming compression. ICERR_OK or error
      if (lParam1)
      {
        ICCOMPRESSFRAMES *icm = (ICCOMPRESSFRAMES *) lParam1;
        this->fpsnum = icm->dwRate;
        this->fpsden = icm->dwScale;
      }
      return ICERR_OK;  
      
    case ICM_COMPRESS_BEGIN:
      // 1 = PBITMAPINFO in
      // 2 = PBITMAPINFO out
      // start compression stream.  ICERR_OK or ICERR_BADFORMAT
      if (compress_start (this, (PBITMAPINFO) lParam1, (PBITMAPINFO) lParam2))
        return ICERR_OK;
      else
        return ICERR_BADFORMAT;
      
      
    case ICM_COMPRESS:
      // 1 = ICCOMPRESS *
      // 2 = sizeof (ICCOMPRESS)
      // compress a frame.  ICERR_OK or ICERR_BADFORMAT
      if (compress (this, (ICCOMPRESS *) lParam1, lParam2))
        return ICERR_OK;
      else
        return ICERR_ERROR;
      
    case ICM_COMPRESS_END:
      // 1 = 0
      // 2 = 0
      // finish compression stream.  ICERR_OK or error
      compress_finish (this);
      //MessageBox (NULL, "Capture finalized successfully!", "Pipe Codec", 0);
      return ICERR_OK;
      
      
    default:
      break;
  }
  
  
  if (uiMessage < DRV_USER)
    return DefDriverProc (dwDriverID, hDriver, uiMessage, lParam1, lParam2);
  else
    return ICERR_UNSUPPORTED;
}  


