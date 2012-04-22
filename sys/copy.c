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


#ifdef _WIN32

void truename(char *dest, const char *src)
{
  if (_fullpath(dest, src, SYS_MAXPATH) == NULL)
    strcpy(dest, src); /* oops! fallback to what user specified */
}

/*
 * Returns TRUE if `drive` has at least `bytes` free space, FALSE otherwise.
 */
BOOL check_space(COUNT drive, ULONG bytes)
{
  ULARGE_INTEGER freeBytes;
  char *drivename = "A:\\";
  drivename[0] = 'A' + drive;
  if (GetDiskFreeSpaceExA(drivename, &freeBytes, NULL, NULL))
  {
    if (freeBytes.QuadPart >= bytes) 
      return TRUE;
  }
  return FALSE;
} /* check_space */


#define allocBlock(blksize) (BYTE *)malloc((unsigned)(blksize))
#define freeBlock(ptr) free((void *)(ptr))

#define normalizePtr(ptr)


typedef struct _utimbuf filetime_t;

/* get original file date and time */
void getFileTime(int fdin, filetime_t *filetime)
{
  struct stat fstatbuf;
  if (fstat(fdin, &fstatbuf)) 
	  memset(&fstatbuf, 0, sizeof(fstatbuf));
  filetime->actime =              /* access time */
      filetime->modtime = fstatbuf.st_mtime;      /* modification time */
}

  /* set copied files time to match original */
#ifdef _MSC_VER
#define setFileTimeAndClose(fname, fd, filetime) _futime(fd, (struct _utimbuf *)filetime); close(fd)
#else
#define setFileTimeAndClose(fname, fd, filetime) close(fdout); _utime(fname, (struct _utimbuf *)filetime)
#endif

#else

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

unsigned getextdrivespace(void far *drivename, void *buf, unsigned buf_size);
#pragma aux getextdrivespace =  \
      "mov ax, 0x7303"    \
      "stc"               \
      "int 0x21"          \
      "sbb ax, ax"        \
      parm [es dx] [di] [cx] \
      value [ax];

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

#endif /* __WATCOMC__ */

/* static */ struct xfreespace x; /* we make this static to be 0 by default -
                                     this avoids FAT misdetections */
/*
 * Returns TRUE if `drive` has at least `bytes` free space, FALSE otherwise.
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


/* allocate memory from DOS, return NULL on error, pointer to buffer otherwise */
BYTE FAR* allocBlock(ULONG memsize)
{
  unsigned dseg;
#ifdef __TURBOC__
  if (allocmem((unsigned)((memsize+15)>>4), &dseg)!=-1)
#else
  if (_dos_allocmem((unsigned)((memsize+15)>>4), &dseg)!=0)
#endif
    return NULL; /* failed to allocate memory */
    
  return MK_FP(dseg, 0); /* success */
}

#ifdef __TURBOC__
#define freeBlock(ptr) freemem(FP_SEG(ptr))
#else
#define freeBlock(ptr) _dos_freemem(FP_SEG(ptr))
#endif

/* adjust segment:offset to maintain valid, takes a pointer to a far pointer to updated */
void normalizePtr(BYTE FAR **ptr)
{
  register BYTE FAR *bufptr = *ptr;
  if (FP_OFF(bufptr) > 0x7777) /* watcom needs this in tiny model */
  {
    bufptr = MK_FP(FP_SEG(bufptr)+0x700, FP_OFF(bufptr)-0x7000);
  }
  *ptr = bufptr;
}


#if defined __WATCOMC__ || defined _MSC_VER /* || defined __BORLANDC__ */
  typedef struct {
  #if defined(__WATCOMC__) && __WATCOMC__ < 1280
    unsigned short date, time;
  #else
    unsigned date, time;
  #endif
  } filetime_t;
#elif defined __TURBOC__
  typedef struct ftime filetime_t;
#endif

/* get original file date and time */
#if defined __WATCOMC__ || defined _MSC_VER /* || defined __BORLANDC__ */
  #define getFileTime(fdin, filetime) _dos_getftime(fdin, &((filetime)->date), &((filetime)->time));
#elif defined __TURBOC__
  #define getFileTime(fdin, filetime) getftime(fdin, filetime);
#endif

/* set copied files time to match original */
#if defined __WATCOMC__ || defined _MSC_VER /* || defined __BORLANDC__ */
  #define setFileTimeAndClose(fname, fd, filetime) _dos_setftime(fd, (filetime)->date, (filetime)->time); close(fd)
#elif defined __TURBOC__
  #define setFileTimeAndClose(fname, fd, filetime) setftime(fd, ftime); close(fd)
#endif


#endif /* _WIN32 */




BYTE copybuffer[COPY_SIZE];


/* copies file (path+filename specified by srcFile) to drive:\filename */
BOOL copy(const BYTE *source, COUNT drive, const BYTE * filename)
{
  static BYTE src[SYS_MAXPATH];
  static BYTE dest[SYS_MAXPATH];
  unsigned ret;
  int fdin, fdout;
  ULONG copied = 0;
  filetime_t filetime;

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

  /* get original creation file date & time */
  getFileTime(fdin, &filetime);

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
    BYTE far *buffer, far *bufptr;
    UWORD offs;
    unsigned chunk_size;
    
    /* get length of file to copy, then allocate enough memory for whole file */
    filesize = filelength(fdin);
      buffer = allocBlock(filesize);
      if (buffer == NULL)
      {
        printf("Not enough memory to buffer %lu bytes for %s\n", filesize, source);
        return FALSE;
      }
    bufptr = buffer;

    /* read in whole file, a chunk at a time; adjust size of last chunk to match remaining bytes */
    chunk_size = (COPY_SIZE < filesize)?COPY_SIZE:(unsigned)filesize;
    while ((ret = read(fdin, copybuffer, chunk_size)) > 0)
    {
      for (offs = 0; offs < ret; offs++)
      {
        *bufptr = copybuffer[offs];
        bufptr++;
        normalizePtr(&bufptr);
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
        normalizePtr(&bufptr);
      }

      /* write the data to disk, abort on any error */
      if ((unsigned)write(fdout, copybuffer, chunk_size) != chunk_size)
      {
        printf("Can't write %u bytes to %s\n", ret, dest);
        close(fdout);
        unlink(dest);
        return FALSE;
      }
    } while (copied < filesize);

    freeBlock(buffer);
  }
 #endif /* simple copy loop */

  /* reduce disk swap on single drives, close file on drive last accessed 1st */

  /* set copied files time to match original and close file */
  setFileTimeAndClose(dest, fdout, &filetime);
  
  /* close input file, usually same drive as next action will access */
  close(fdin);


  printf("%lu Bytes transferred\n", copied);

  return TRUE;
} /* copy */

