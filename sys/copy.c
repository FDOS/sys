/***************************************************************

                                    copy.c
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

#include "sys.h"
#include "xstructs.h"
#include "diskio.h"

#ifdef __TURBOC__
#include <dir.h>
#include <mem.h>
#else
#include <memory.h>
#endif

#define COPY_SIZE       0x4000

#ifdef __WATCOMC__

long filelength(int __handle);
#pragma aux filelength = \
      "mov ax, 0x4202" \
      "xor cx, cx" \
      "xor dx, dx" \
      "int 0x21" \
      "push ax" \
      "push dx" \
      "mov ax, 0x4200" \
      "xor cx, cx" \
      "xor dx, dx" \
      "int 0x21" \
      "pop dx" \
      "pop ax" \
      parm [bx] \
      modify [cx] \
      value [dx ax];

#endif



/* static */ struct xfreespace x; /* we make this static to be 0 by default -
                                     this avoids FAT misdetections */

#ifdef __WATCOMC__

void truename(char far *dest, const char *src);
#pragma aux truename = \
      "mov ah,0x60"       \
      "int 0x21"          \
      parm [es di] [si];

#else

void truename(char *dest, const char *src)
{
  union REGS regs;
  struct SREGS sregs;

  regs.h.ah = 0x60;
  sregs.es = FP_SEG(dest);
  regs.x.di = FP_OFF(dest);
  sregs.ds = FP_SEG(src);
  regs.x.si = FP_OFF(src);
  intdosx(&regs, &regs, &sregs);
} /* truename */

#endif

#ifdef __WATCOMC__

unsigned getextdrivespace(void far *drivename, void *buf, unsigned buf_size);
#pragma aux getextdrivespace =  \
      "mov ax, 0x7303"    \
      "stc"               \
      "int 0x21"          \
      "sbb ax, ax"        \
      parm [es dx] [di] [cx] \
      value [ax];

#else /* !defined __WATCOMC__ */

unsigned getextdrivespace(void far *drivename, void *buf, unsigned buf_size)
{
  union REGS regs;
  struct SREGS sregs;

  regs.x.ax = 0x7303;         /* get extended drive free space */

  sregs.es = FP_SEG(buf);
  regs.x.di = FP_OFF(buf);
  sregs.ds = FP_SEG(drivename);
  regs.x.dx = FP_OFF(drivename);

  regs.x.cx = buf_size;

  intdosx(&regs, &regs, &sregs);
  return regs.x.ax == 0x7300 || regs.x.cflag;
} /* getextdrivespace */

#endif /* defined __WATCOMC__ */



/*
 * Returns TRUE if `drive` has at least `bytes` free space, FALSE otherwise.
 * put_sector() must have been already called to determine file system type.
 */
BOOL check_space(COUNT drive, ULONG bytes)
{
  /* try extended drive space check 1st, if unsupported or other error fallback to standard check */
  char *drivename = "A:\\";
  drivename[0] = 'A' + drive;
  if (getextdrivespace(drivename, &x, sizeof(x)))
  {
#ifdef __TURBOC__
    struct dfree df;
    getdfree(drive + 1, &df);
    return (ULONG)df.df_avail * df.df_sclus * df.df_bsec >= bytes;
#else
    struct _diskfree_t df;
    _dos_getdiskfree(drive + 1, &df);
    return (ULONG)df.avail_clusters * df.sectors_per_cluster
      * df.bytes_per_sector >= bytes;
#endif
  }
  else
    return x.xfs_freeclusters > (bytes / (x.xfs_clussize * x.xfs_secsize));
} /* check_space */


BYTE copybuffer[COPY_SIZE];

#ifndef WIN32
/* allocate memory from DOS, return 0 on success, nonzero otherwise */
int alloc_dos_mem(ULONG memsize, UWORD *theseg)
{
  unsigned dseg;
#ifdef __TURBOC__
  if (allocmem((unsigned)((memsize+15)>>4), &dseg)!=-1)
#else
  if (_dos_allocmem((unsigned)((memsize+15)>>4), &dseg)!=0)
#endif
    return -1; /* failed to allocate memory */

  *theseg = (UWORD)dseg;
  return 0; /* success */
}
#ifdef __TURBOC__
#define dos_freemem freemem
#else
#define dos_freemem _dos_freemem
#endif
#endif

/* copies file (path+filename specified by srcFile) to drive:\filename */
BOOL copy(const BYTE *source, COUNT drive, const BYTE * filename)
{
  static BYTE src[SYS_MAXPATH];
  static BYTE dest[SYS_MAXPATH];
  unsigned ret;
  int fdin, fdout;
  ULONG copied = 0;

#if defined __WATCOMC__ || defined _MSC_VER /* || defined __BORLANDC__ */
#if defined(__WATCOMC__) && __WATCOMC__ < 1280
  unsigned short date, time;
#else
  unsigned date, time;
#endif
#elif defined __TURBOC__
  struct ftime ftime;
#endif

  printf("Copying %s...\n", source);

  truename(src, source);
  sprintf(dest, "%c:\\%s", 'A' + drive, filename);
  if (stricmp(src, dest) == 0)
  {
    printf("%s: source and destination are identical: skipping \"%s\"\n",
           pgm, source);
    return TRUE;
  }

  if ((fdin = open(source, O_RDONLY | O_BINARY)) < 0)
  {
    printf("%s: failed to open \"%s\"\n", pgm, source);
    return FALSE;
  }

#if defined __WATCOMC__ || defined _MSC_VER /* || defined __BORLANDC__ */
  _dos_getftime(fdin, &date, &time);
#elif defined __TURBOC__
  getftime(fdin, &ftime);
#endif

  if (!check_space(drive, filelength(fdin)))
  {
    printf("%s: Not enough space to transfer %s\n", pgm, filename);
    close(fdin);
    return FALSE;
  }

  if ((fdout =
       open(dest, O_RDWR | O_TRUNC | O_CREAT | O_BINARY,
            S_IREAD | S_IWRITE)) < 0)
  {
    printf(" %s: can't create\"%s\"\nDOS errnum %d\n", pgm, dest, errno);
    close(fdin);
    return FALSE;
  }

#if 0 /* simple copy loop, read chunk then write chunk, repeat until all data copied */
  while ((ret = read(fdin, copybuffer, COPY_SIZE)) > 0)
  {
    if (write(fdout, copybuffer, ret) != ret)
    {
      printf("Can't write %u bytes to %s\n", ret, dest);
      close(fdout);
      unlink(dest);
      return FALSE;
    }
    copied += ret;
  }
 #else /* read in whole file, then write out whole file */
  {
    ULONG filesize;
    UWORD theseg;
    BYTE far *buffer, far *bufptr;
    UWORD offs;
    unsigned chunk_size;
    
    /* get length of file to copy, then allocate enough memory for whole file */
    filesize = filelength(fdin);
    #ifdef WIN32
      buffer = (BYTE *)malloc((unsigned)filesize);
    #else
      if (alloc_dos_mem(filesize, &theseg)!=0)
      {
        printf("Not enough memory to buffer %lu bytes for %s\n", filesize, source);
        return FALSE;
      }
      buffer = MK_FP(theseg, 0);
    #endif
    bufptr = buffer;

    /* read in whole file, a chunk at a time; adjust size of last chunk to match remaining bytes */
    chunk_size = (COPY_SIZE < filesize)?COPY_SIZE:(unsigned)filesize;
    while ((ret = read(fdin, copybuffer, chunk_size)) > 0)
    {
      for (offs = 0; offs < ret; offs++)
      {
        *bufptr = copybuffer[offs];
        bufptr++;
        if (FP_OFF(bufptr) > 0x7777) /* watcom needs this in tiny model */
        {
          bufptr = MK_FP(FP_SEG(bufptr)+0x700, FP_OFF(bufptr)-0x7000);
        }
      }
      /* keep track of how much read in, and only read in filesize bytes */
      copied += ret;
      chunk_size = (COPY_SIZE < (filesize-copied))?COPY_SIZE:(unsigned)(filesize-copied);
    }

    /* write out file, a chunk at a time; adjust size of last chunk to match remaining bytes */
    bufptr = buffer;
    copied = 0;
    do
    {
      /* keep track of how much read in, and only read in filesize bytes */
      chunk_size = (COPY_SIZE < (filesize-copied))?COPY_SIZE:(unsigned)(filesize-copied);
      copied += chunk_size;

      /* setup chunk of data to be written out */
      for (offs = 0; offs < chunk_size; offs++)
      {
        copybuffer[offs] = *bufptr;
        bufptr++;
        if (FP_OFF(bufptr) > 0x7777) /* watcom needs this in tiny model */
        {
          bufptr = MK_FP(FP_SEG(bufptr)+0x700, FP_OFF(bufptr)-0x7000);
        }
      }

      /* write the data to disk, abort on any error */
      if (write(fdout, copybuffer, chunk_size) != chunk_size)
      {
        printf("Can't write %u bytes to %s\n", ret, dest);
        close(fdout);
        unlink(dest);
        return FALSE;
      }
    } while (copied < filesize);

    #ifdef WIN32
      free((void *)buffer);
    #else
      dos_freemem(theseg);
    #endif
  }
 #endif

#if defined __WATCOMC__ || defined _MSC_VER /* || defined __BORLANDC__ */
  _dos_setftime(fdout, date, time);
#elif defined __TURBOC__
  setftime(fdout, &ftime);
#endif

  /* reduce disk swap on single drives, close file on drive last accessed 1st */
  close(fdout);

#ifdef __SOME_OTHER_COMPILER__
  {
#include <utime.h>
    struct utimbuf utimb;

    utimb.actime =              /* access time */
        utimb.modtime = fstatbuf.st_mtime;      /* modification time */
    utime(dest, &utimb);
  };
#endif

  /* and close input file, usually same drive as next action will access */
  close(fdin);


  printf("%lu Bytes transferred\n", copied);

  return TRUE;
} /* copy */

