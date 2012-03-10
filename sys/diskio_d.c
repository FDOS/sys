/***************************************************************

                                    diskio_d.c
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
#include "diskio.h"

void reset_drive(int DosDrive);
int generic_block_ioctl(unsigned drive, unsigned cx, unsigned char *par);

#ifdef __WATCOMC__
#pragma aux haveLBA =  \
      "mov ax, 0x4100"  /* IBM/MS Int 13h Extensions - installation check */ \
      "mov bx, 0x55AA" \
      "mov dl, 0x80"   \
      "int 0x13"       \
      "xor ax, ax"     \
      "cmp bx, 0xAA55" \
      "jne quit"       \
      "and cx, 1"      \
      "xchg cx, ax"    \
"quit:"                \
      modify [bx cx dx]   \
      value [ax];
      
#pragma aux reset_drive "*_" = \
      "push ds" \
      "inc dx" \
      "mov ah, 0xd" \ 
      "int 0x21" \
      "mov ah,0x32" \
      "int 0x21" \
      "pop ds" \
      parm [dx] \
      modify [ax bx];

#pragma aux generic_block_ioctl = \
      "mov ax, 0x440d" \
      "int 0x21" \
      "sbb ax, ax" \
      value [ax] \
      parm [bx] [cx] [dx]; /* BH must be 0 for lock! */


int absread(int DosDrive, int nsects, int foo, void *diskReadPacket);
#pragma aux absread =  \
      "push bp"           \
      "int 0x25"          \
      "sbb ax, ax"        \
      "popf"              \
      "pop bp"            \
      parm [ax] [cx] [dx] [bx] \
      modify [si di] \
      value [ax];

int abswrite(int DosDrive, int nsects, int foo, void *diskReadPacket);
#pragma aux abswrite =  \
      "push bp"           \
      "int 0x26"          \
      "sbb ax, ax"        \
      "popf"              \
      "pop bp"            \
      parm [ax] [cx] [dx] [bx] \
      modify [si di] \
      value [ax];

int fat32readwrite(int DosDrive, void *diskReadPacket, unsigned intno);
#pragma aux fat32readwrite =  \
      "mov ax, 0x7305"    \
      "mov cx, 0xffff"    \
      "int 0x21"          \
      "sbb ax, ax"        \
      parm [dx] [bx] [si] \
      modify [cx dx si]   \
      value [ax];

#else

#ifndef __TURBOC__

int2526readwrite(int DosDrive, void *diskReadPacket, unsigned intno)
{
  union REGS regs;

  regs.h.al = (BYTE) DosDrive;
  regs.x.bx = (short)diskReadPacket;
  regs.x.cx = 0xffff;

  int86(intno, &regs, &regs);

  return regs.x.cflag;
}

#define absread(DosDrive, foo, cx, diskReadPacket) \
int2526readwrite(DosDrive, diskReadPacket, 0x25)

#define abswrite(DosDrive, foo, cx, diskReadPacket) \
int2526readwrite(DosDrive, diskReadPacket, 0x26)

#endif

int fat32readwrite(int DosDrive, void *diskReadPacket, unsigned intno)
{
  union REGS regs;

  regs.x.ax = 0x7305;
  regs.h.dl = DosDrive;
  regs.x.bx = (short)diskReadPacket;
  regs.x.cx = 0xffff;
  regs.x.si = intno;
  intdos(&regs, &regs);
  
  return regs.x.cflag;
} /* fat32readwrite */

#endif

int MyAbsReadWrite(int DosDrive, int count, ULONG sector, void *buffer,
                   int write)
{
  struct {
    unsigned long sectorNumber;
    unsigned short count;
    void far *address;
  } diskReadPacket;

  diskReadPacket.sectorNumber = sector;
  diskReadPacket.count = count;
  diskReadPacket.address = buffer;

  if ((!write && absread(DosDrive, -1, -1, &diskReadPacket) == -1)
      || (write && abswrite(DosDrive, -1, -1, &diskReadPacket) == -1))
  {
#ifdef WITHFAT32
    return fat32readwrite(DosDrive + 1, &diskReadPacket, write);
#else
    return 0xff;
#endif
  }
  return 0;
} /* MyAbsReadWrite */


#ifdef __WATCOMC__

/* some non-conforming functions to make the executable smaller */
int open(const char *pathname, int flags, ...)
{
  int handle;
  int result = (flags & O_CREAT ?
                _dos_creat(pathname, _A_NORMAL, &handle) :
                _dos_open(pathname, flags & (O_RDONLY | O_WRONLY | O_RDWR),
                          &handle));

  return (result == 0 ? handle : -1);
}

int read(int fd, void *buf, unsigned count)
{
  unsigned bytes;
  int result = _dos_read(fd, buf, count, &bytes);

  return (result == 0 ? bytes : -1);
}

int write(int fd, const void *buf, unsigned count)
{
  unsigned bytes;
  int result = _dos_write(fd, buf, count, &bytes);

  return (result == 0 ? bytes : -1);
}

int stat(const char *file_name, struct stat *statbuf)
{
  struct find_t find_tbuf;

  int ret = _dos_findfirst(file_name, _A_NORMAL | _A_HIDDEN | _A_SYSTEM, &find_tbuf);
  statbuf->st_size = (off_t)find_tbuf.size;
  /* statbuf->st_attr = (ULONG)find_tbuf.attrib; */
  return ret;
}


#else

BOOL haveLBA(void)
{
  union REGS r;
  r.x.ax = 0x4100;
  r.x.bx = 0x55AA;
  r.h.dl = 0x80;
  int86(0x13, &r, &r);
  return r.x.bx == 0xAA55 && r.x.cx & 1;
}

void reset_drive(int DosDrive)
{
  union REGS regs;

  regs.h.ah = 0xd;
  intdos(&regs, &regs);
  regs.h.ah = 0x32;
  regs.h.dl = DosDrive + 1;
  intdos(&regs, &regs);
} /* reset_drive */

int generic_block_ioctl(unsigned drive, unsigned cx, unsigned char *par)
{
  union REGS regs;

  regs.x.ax = 0x440d;
  regs.x.cx = cx;
  regs.x.dx = (unsigned)par;
  regs.x.bx = drive; /* BH must be 0 for lock! */
  intdos(&regs, &regs);
  return regs.x.cflag;
} /* generic_block_ioctl */

#endif
