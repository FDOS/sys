/***************************************************************

                                    sys.h
                                    DOS-C

                            sys utility for DOS-C

                             Copyright (c) 1991
                             Pasquale J. Villani
                             All Rights Reserved

 This file is part of DOS-C.

 DOS-C is free software; you can redistribute it and/or modify it under the
 terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 DOS-C is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 DOS-C; see the file COPYING.  If not, write to the Free Software Foundation,
 675 Mass Ave, Cambridge, MA 02139, USA.

***************************************************************/
#ifndef _SYS_H_
#define _SYS_H_

#define SYS_VERSION "v3.8"
#include "config.h"

#ifndef SYS_NAME
#define SYS_NAME "FreeDOS"
#endif

#ifdef DRSYS
#undef SYS_NAME
#define SYS_NAME "Enhanced DR-DOS"
#ifdef FDCONFIG
#undef FDCONFIG
#endif
#ifdef WITHOEMCOMPATBS
#undef WITHOEMCOMPATBS
#endif
#endif


#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif
#include <dos.h>
#define SYS_MAXPATH   260
#include "portab.h"
#include "algnbyte.h"
#include "device.h"
#include "dcb.h"
#include "fat.h"

#ifdef _WIN32
#include <stdio.h>
#include <direct.h>
#else
/* These definitions deliberately put here instead of
 * #including <stdio.h> to make executable MUCH smaller
 * using [s]printf from prf.c!
 */
extern int VA_CDECL printf(CONST char FAR * fmt, ...);
extern int VA_CDECL sprintf(char FAR * buff, CONST char FAR * fmt, ...);
#endif

#ifndef __WATCOMC__
#include <io.h>
#else
int stat(const char *file_name, struct stat *statbuf);
#endif


extern BYTE pgm[]; // = "SYS";


#ifdef FDCONFIG
int FDKrnConfigMain(int argc, char **argv);
#endif

/* Indicates file system destination currently formatted as */
typedef enum {UNKNOWN=0, FAT12 = 12, FAT16 = 16, FAT32 = 32} FileSystem;

/* FreeDOS sys, we default to our kernel and load segment, but
   if not found (or explicitly given) support OEM DOS variants
   (such as DR-DOS or a FreeDOS kernel mimicing other DOSes).
   Note: other (especially older) DOS versions expect the boot
   loader to perform particular steps, which we may not do;
   older PC/MS DOS variants may work with the OEM compatible
   boot sector (optionally included).
*/
typedef struct DOSBootFiles {
  const char * kernel;   /* filename boot sector loads and chains to */
  const char * dos;      /* optional secondary file for OS */
  WORD         loadaddr; /* segment kernel file expects to start at for stdbs */
                         /* or offset to jump into kernel for oem compat bs */
  BOOL         stdbs;    /* use FD boot sector (T) or oem compat one (F) */
  LONG         minsize;  /* smallest dos file can be and be valid, 0=existance optional */
} DOSBootFiles;
extern DOSBootFiles bootFiles[];

typedef struct SYSOptions {
  BYTE srcDrive[SYS_MAXPATH];   /* source drive:[path], root assumed if no path */
  BYTE dstDrive;                /* destination drive [STD SYS option] */
  int flavor;                   /* DOS variant we want to boot, default is AUTO/FD */
  DOSBootFiles kernel;          /* file name(s) and relevant data for kernel */
  BYTE defBootDrive;            /* value stored in boot sector for drive, eg 0x0=A, 0x80=C */
  BOOL ignoreBIOS;              /* true to NOP out boot sector code to get drive# from BIOS */
  BOOL skipBakBSCopy;           /* true to not copy boot sector to backup boot sector */
  BOOL copyKernel;              /* true to copy kernel files */
  BOOL copyShell;               /* true to copy command interpreter */
  BOOL writeBS;                 /* true to write boot sector to drive/partition LBA 0 */
  BYTE *bsFile;                 /* file name & path to save bs to when saving to file */
  BYTE *bsFileOrig;             /* file name & path to save original bs when backing up */
  BYTE *altBSCode;              /* file name & path for external boot code file */
  BYTE *fnKernel;               /* optional override to source kernel filename (src only) */
  BYTE *fnCmd;                  /* optional override to cmd interpreter filename (src & dest) */
  enum {AUTO=0,LBA,CHS} force;  /* optional force boot sector to only use LBA or CHS */
  BOOL verbose;                 /* show extra (DEBUG) output */
  int bsCount;                  /* how many sectors to read/write */
  
  FileSystem fs;                /* current file system, set based on existing BPB not user option */
  ULONG rootSector;             /* obtained from existing BPB, used for updating root directory */
  UCOUNT rootDirSectors;        /* when booting with OEM boot logic with boot files in 1st entries */
} SYSOptions;

/* display how to use and basic help information */
void showHelpAndExit(void);
/* List OEM options supported, I.e. known DOS flavors supported */
void showOemHelpAndExit(void);

/* get and validate arguments */
void initOptions(int argc, char *argv[], SYSOptions *opts);


/* installs boot sector */
void put_boot(SYSOptions *opts);

/* write bs in bsFile to drive's boot record unmodified */
void restoreBS(SYSOptions *opts);
/* write bs in bsFile to drive's boot record updating BPB */
void putBS(SYSOptions *opts);
/* write drive's boot record unmodified to bsFile */
void dumpBS(SYSOptions *opts);

/* copies file (path+filename specified by srcFile) to drive:\filename */
BOOL copy(const BYTE *source, COUNT drive, const BYTE * filename);

#endif /* _SYS_H_ */
