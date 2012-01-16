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

/* #define DEBUG */           /* to display extra information */
/* #define DDEBUG */          /* to enable display of sector dumps */
/* #define WITHOEMCOMPATBS */ /* include support for OEM MS/PC DOS 3.??-6.x */
#define FDCONFIG              /* include support to configure FD kernel */
/* #define DRSYS */           /* SYS for Enhanced DR-DOS (OpenDOS enhancement Project) */

#define SYS_VERSION "v3.7a"
#define SYS_NAME "FreeDOS System Installer "


#ifdef DRSYS            /* set displayed name & drop FD kernel config */
#undef SYS_NAME
#define SYS_NAME "Enhanced DR-DOS System Installer "
#ifdef FDCONFIG
#undef FDCONFIG
#endif
#ifdef WITHOEMCOMPATBS
#undef WITHOEMCOMPATBS
#endif
#endif

#include <stdlib.h>
#include <dos.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef __TURBOC__
#include <mem.h>
#else
#include <memory.h>
#endif
#include <string.h>
#ifdef __TURBOC__
#include <dir.h>
#endif
#define SYS_MAXPATH   260
#include "portab.h"
#include "algnbyte.h"
#include "device.h"
#include "dcb.h"
#include "xstructs.h"
#include "date.h"
#include "../hdr/time.h"
#include "fat.h"

/* These definitions deliberately put here instead of
 * #including <stdio.h> to make executable MUCH smaller
 * using [s]printf from prf.c!
 */
extern int VA_CDECL printf(CONST char FAR * fmt, ...);
extern int VA_CDECL sprintf(char FAR * buff, CONST char FAR * fmt, ...);


#ifndef __WATCOMC__
#include <io.h>
#else
/* some non-conforming functions to make the executable smaller */
int open(const char *pathname, int flags, ...);
int read(int fd, void *buf, unsigned count);
int write(int fd, const void *buf, unsigned count);
#define close _dos_close
#endif


extern BYTE pgm[]; // = "SYS";


/*
 * globals needed by put_boot & check_space
 */
typedef enum {FAT12 = 12, FAT16 = 16, FAT32 = 32} FileSystem;
extern FileSystem fs;  /* file system type */


#ifdef FDCONFIG
int FDKrnConfigMain(int argc, char **argv);
#endif

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
  BYTE *fnKernel;               /* optional override to source kernel filename (src only) */
  BYTE *fnCmd;                  /* optional override to cmd interpreter filename (src & dest) */
  enum {AUTO=0,LBA,CHS} force;  /* optional force boot sector to only use LBA or CHS */
  BOOL verbose;                 /* show extra (DEBUG) output */
  int bsCount;                  /* how many sectors to read/write */
} SYSOptions;

/* display how to use and basic help information */
void showHelpAndExit(void);
/* List OEM options supported, I.e. known DOS flavors supported */
void showOemHelpAndExit(void);

/* get and validate arguments */
void initOptions(int argc, char *argv[], SYSOptions *opts);

void correct_bpb(struct bootsectortype *default_bpb,
                 struct bootsectortype *oldboot, BOOL verbose);


/* reads in boot sector (1st SEC_SIZE bytes) from file */
void readBS(const char *bsFile, UBYTE *bootsector);
/* write bs in bsFile to drive's boot record unmodified */
void restoreBS(const char *bsFile, int drive);
/* write bootsector to file bsFile */
void saveBS(const char *bsFile, UBYTE *bootsector);
/* write drive's boot record unmodified to bsFile */
void dumpBS(const char *bsFile, int drive);

void put_boot(SYSOptions *opts);

/* copies file (path+filename specified by srcFile) to drive:\filename */
BOOL copy(const BYTE *source, COUNT drive, const BYTE * filename);