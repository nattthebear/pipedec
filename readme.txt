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



"Pipe Codec" (pipedec.dll and pipedec64.dll) acts as a VFW compressor.  When
called upon to compress data, it runs an external program, feeds it the
uncompressed data on stdin, and sends a dummy compressed stream back to VFW.

Rationale:  VFW is legacy these days.  FFDShow has discontinued support for
many of their VFW encoders.  x264VFW is not officially supported and lags
behind the official releases in many ways.  The AVI container is outdated and
doesn't support modern constructs like bframes correctly.  However, many VFW
apps are still in use, and if you have a program that encodes to VFW only, it
would be nice to have options other than Intel Indeo =p.

Compatibility:  Pipe Codec should work on any Windows 2000 or later machine.
On a 32 bit OS, only the 32 bit build will work, and it should run from any
32 bit VFW host and be able to call any 32 bit compression application.  On
a 64 bit OS, both 32 and 64 bit builds of Pipe Codec can be used.  The build
used in any particular case must match the VFW host, but the encoding
application called can be 32 or 64 bit with either 32 or 64 bit Pipe Codec.

Installation, 32 bit OS:
There is one applicable registry file:

register pipedec (32 bit on 32 bit system).reg

You should never run untrusted registry files.  Load this file into a text
editor and examine the content.  When you are satsified that it is not
malicious, you can load it into the registry.

The pipedec.dll file is customarily copied to %Windir%\system32, but it can
be placed anywhere in the DLL search path for the VFW host in question,
including the application's path.

Installation, 64 bit OS:
There are two applicable registry file:

register pipedec (32 bit on 64 bit system).reg
register pipedec (64 bit on 64 bit system).reg

You should never run untrusted registry files.  Load these files into a text
editor and examine their content.  When you are satsified that they are not
malicious, you can load them into the registry.

For 32 bit VFW hosts, the pipedec.dll file is used.  It is customarily copied
to %Windir%\SysWoW64, but it can be placed anywhere in the DLL search path for
the VFW host in question, including the application's path.

For 64 bit VFW hosts, the pipedec64.dll file is used.  It is customarily copied
to %Windir%\system32, but it can be placed anywhere in the DLL search path for
the VFW host in question, including the application's path.

Uninstallation:  Because you didn't install anything that you didn't fully
understand, you understand how to uninstall it without further instruction.

Configuration:  Pipe Codec is configured through a text file located at:
%APPDATA%\pipedec\.pipedec
If this file does not exist, the first time Pipe Codec is queried for
formats, it will be created.  The file is editable in any text editor,
and the default one includes comments on how to edit it.  If you're still
lost after reading the default config file, the included exampleconfig.pipedec
has some actual handler examples.  Note that the same configuration file is
used for both 32 and 64 bit VFW hosts on a 64 bit OS.  This should not be a
problem since you may freely specify a 32 bit or 64 bit encoding application
and both will use it successfully.

Operation:  When asked to compress, Pipe Codec will look in the configuration
file for a "handler": a command line to run together with a FOURCC that it works
with.  Any DIB format or FOURCC can be used, provided you have an external
application that will understand it.  The command line in question will be
run with escape sequences replaced as described in the configuration file.

The application will be sent the contents of icc->lpInput for each frame,
concatenated together as pure binary data.  The application can recieve the
width and height of the stream, as well as the frame rate, on the command line.
No other information is passed, including icc->lpbiInput->biImageSize, or
any palette colors (for DIB).  Out of the box, some basic formats like
various uncompressed YUV and RGB will work with popular programs like x264
and ffmpeg.  Pipe Codec does no internal conversion.

Pipe Codec sends a dummy video stream back to VFW, which typically ends up
in an AVI file.  This stream will have fourCC PIPE, every frame will be
marked as a keyframe, and each frame will have 4 bytes of data to it.
Pipe Codec does not register itself as a decoder for this format.

Notes:  The files pipecodec_stderr.txt and pipecodec_stdout.txt, placed
in the current directory, contain the output of the program that was run.

The current directory that the piped application runs in is not
nessecarily predictable, but probably will be the same as the AVI file being
created.
