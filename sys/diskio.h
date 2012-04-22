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
long lseek(int handle, long offset, int origin ); 
#ifndef SEEK_END
#define SEEK_END 2
#endif
#endif


int MyAbsReadWrite(int DosDrive, int count, ULONG sector, void *buffer, int write);

void lockDrive(unsigned drive);
void unLockDrive(unsigned drive);

/* returns default BPB (and other device parameters) */
int getDeviceParms(unsigned drive, FileSystem fs, unsigned char *buffer);

/*
 * If BIOS has got LBA extensions, after the Int 13h call BX will be 0xAA55.
 * If extended disk access functions are supported, bit 0 of CX will be set.
 */
BOOL haveLBA(void);     /* return TRUE if we have LBA BIOS, FALSE otherwise */

/* return canonical (full) name */
void truename(char *dest, const char *src);

#if defined __WATCOMC__ && defined __DOS__
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
      
#pragma aux lseek = \
      "mov ah, 0x42" /* DOS set current file position */ \
      "int 0x21"       \
      "jnc ok"         \
      "mov ax, -1"     \
      "cwd"            \
"ok:"                  \
      parm [bx] [cx dx] [al] \
      modify [ax bx cx dx] \
      value [dx ax];

/* return canonical (full) name */
void truename(char *dest, const char *src);
#pragma aux truename =  \
      "push ds"         \
      "pop es"          \
      "mov ah,0x60"     \
      "int 0x21"        \
      parm [di] [si]    \
      modify [ax di si es];
#endif
