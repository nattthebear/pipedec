CFLAGS = -Wall -g -O0
CC = gcc

all:	pipedec.dll vfwtest.exe pipedec.zip


drvproc.o:  drvproc.c piper.h lictext.h
	$(CC) $(CFLAGS) -c drvproc.c -o drvproc.o
    
piper.o: piper.c piper.h samplecfg.h
	$(CC) $(CFLAGS) -c piper.c -o piper.o

exports.o:  pipedec.def
	dlltool -d pipedec.def -e exports.o
    
pipedec.dll:   drvproc.o piper.o exports.o
	$(CC) $(CFLAGS) drvproc.o piper.o exports.o -o pipedec.dll -shared -lwinmm

vfwtest.exe:	vfwtest.c
	$(CC) $(CFLAGS) -std=c99 vfwtest.c -o vfwtest.exe -lvfw32

pipedec.zip:  pipedec.dll
	7z a pipedec.zip pipedec.dll readme.txt *.reg exampleconfig.pipedec
