/***************************************************************

                                    diskio.h
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

/* assumes sys.h already included */

#ifdef __WATCOMC__
/* some non-conforming functions to make the executable smaller */
int open(const char *pathname, int flags, ...);
int read(int fd, void *buf, unsigned count);
int write(int fd, const void *buf, unsigned count);
#define close _dos_close
int unlink(const char *pathname);
#endif


int MyAbsReadWrite(int DosDrive, int count, ULONG sector, void *buffer, int write);

void reset_drive(int DosDrive);
int generic_block_ioctl(unsigned drive, unsigned cx, unsigned char *par);
/*
 * If BIOS has got LBA extensions, after the Int 13h call BX will be 0xAA55.
 * If extended disk access functions are supported, bit 0 of CX will be set.
 */
BOOL haveLBA(void);     /* return TRUE if we have LBA BIOS, FALSE otherwise */

#ifndef WIN32
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

#endif /* WIN32 */
#endif
