/***************************************************************

                                    usage.c
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


/* display how to use and basic help information */
void showHelpAndExit(void)
{
  printf(
      "Usage: %s [source] drive: [bootsect] [{option}]\n"
      "  source   = A:,B:,C:\\DOS\\,etc., or current directory if not given\n"
      "  drive    = A,B,etc.\n"
      "  bootsect = name of 512-byte boot sector file image for drive:\n"
      "             to write to *instead* of real boot sector\n"
      "  {option} is one or more of the following:\n"
      "  /BOTH    : write to *both* the real boot sector and the image file\n"
#ifdef USEBOOTMANAGER
      "  /BOOTMGR : add to boot manager menu (can't be used with /BOTH)\n"
#endif
      "  /BOOTONLY: do *not* copy kernel / shell, only update boot sector or image\n"
      "  /UPDATE  : copy kernel and update boot sector (do *not* copy shell)\n"
      "  /OEM     : indicates boot sector, filenames, and load segment to use\n"
      "             default is /OEM[:AUTO], select DOS based on existing files\n"
      "             see %s /HELP OEM\n"
      "  /K name  : name of kernel to use in boot sector instead of %s\n"
      "  /L segm  : hex load segment to use in boot sector instead of %02x\n"
      "  /B btdrv : hex BIOS # of boot drive set in bs, 0=A:, 80=1st hd,...\n"
      "  /FORCE   : override automatic selection of BIOS related settings\n"
      "             /FORCE:BSDRV use boot drive # set in bootsector\n"
      "             /FORCE:BIOSDRV use boot drive # provided by BIOS\n"
      "  /NOBAKBS : skips copying boot sector to backup bs, FAT32 only else ignored\n"
      "  /HELP    : display this usage screen and exit\n"
#ifdef FDCONFIG
      "Usage: %s CONFIG /HELP\n"
#endif
      /*SYS, KERNEL.SYS/DRBIO.SYS 0x60/0x70*/
      , pgm, pgm, bootFiles[0].kernel, bootFiles[0].loadaddr
#ifdef FDCONFIG
      , pgm
#endif
  );
  exit(1);
}


/* List OEM options supported, I.e. known DOS flavors supported */
void showOemHelpAndExit(void)
{
  printf(
      "Usage: %s [source] drive: [bootsect] [{option}]\n"
      "  /OEM     : indicates boot sector, filenames, and load segment to use\n"
      "             /OEM:FD   use FreeDOS compatible settings\n"
      "             /OEM:EDR  use Enhanced DR DOS 7+ compatible settings\n"
      "             /OEM:DR   use DR DOS 7+ compatible settings\n"
#ifdef WITHOEMCOMPATBS
      "             /OEM:PC   use PC-DOS compatible settings\n"
      "             /OEM:MS   use MS-DOS compatible settings\n"
      "             /OEM:W9x  use MS Win9x DOS compatible settings\n"
      "             /OEM:Rx   use RxDOS compatible settings\n"
      "             /OEM:DELL use Dell Real Mode Kernel (DRMK) settings\n"
#else
      " Note that PC/MS/... compatible settings not compiled in\n"
#endif
      "\n  Default is /OEM[:AUTO], select DOS based on existing files\n"
      , pgm
  );
  exit(1);
}
