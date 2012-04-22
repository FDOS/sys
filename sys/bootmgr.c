/***************************************************************

                                    bootmgr.c
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

#ifdef USEBOOTMANAGER

#include "diskio.h"

#ifdef _WIN32
typedef unsigned long FileAttributes;
#else
typedef unsigned FileAttributes;
static unsigned GetFileAttributes(const char *filename)
{
  unsigned attributes = 0;
  _dos_getfileattr(filename, &attributes);
  return attributes;
}
#define SetFileAttributes(fname, attr) _dos_setfileattr(fname, attr)
#endif


/* returns TRUE if successfully adds section, FALSE any errors */
BOOL writeConfigSection(const char *fname, const char *configText)
{
  int fd;
  BOOL ret = FALSE;
  FileAttributes attr = GetFileAttributes(fname);
  SetFileAttributes(fname, 0); /* remove any readonly, system, or hidden attributes */
  fd = open(fname, O_RDWR|O_TEXT);  /* file must exist */
  if (fd >= 0)
  {
    /* append configText to end of current file */
    if (lseek(fd, 0, SEEK_END) != -1L)
    {
      register int len = strlen(configText);
      if (write(fd, configText, len) == len)
        ret = TRUE;
    }
    close(fd);
  }
  SetFileAttributes(fname, attr); /* restore original attributes */
  return ret;
}

/* returns true if filename exists, false otherwise */
static BOOL exists(const char *fname)
{
  struct stat statbuf;
  return (stat(fname, &statbuf)==0?TRUE:FALSE);
}

/* attempts to determine if file exists
   checks root directory (\basename), then a subdirectory (\name\basename)
   and finally a subdirectory in boot directory (\boot\name\basename)
   returning NULL if file not found
*/
static const char *getBootConfigFile(char drive, const char *basename, const char *name)
{
  static char buffer[SYS_MAXPATH];
  sprintf(buffer, "%c:\\%s", drive, basename);
  if (exists(buffer)) return buffer;
  if (name != NULL)
  {
    sprintf(buffer, "%c:\\%s\\%s", drive, name, basename);
    if (exists(buffer)) return buffer;
    sprintf(buffer, "%c:\\boot\\%s\\%s", drive, name, basename);
    if (exists(buffer)) return buffer;
  }
  return NULL;
}


const char *menuName = "\"FreeDOS (C:)\"";
typedef void (* getConfigSectionTextFn)(char *buffer, const char *bsFilename, unsigned drive);

/* entry for syslinux.cfg (or variants) (check /, /syslinux/, /boot/syslinux/)
*/
void getSyslinuxConfig(char *buffer, const char *bsFilename, unsigned drive)
{
  sprintf(buffer, "\n" \
    "LABEL %s\n" \
    "  BOOT %s\n" /* maybe "BSS %s\n" */ \
    "\n",
    menuName, bsFilename);
}

/* entry for freeldr.ini (check /)
    http://svn.reactos.org/svn/reactos/trunk/reactos/boot/freeldr/FREELDR.INI?revision=29690&view=markup
*/
void getFreeLdrConfig(char *buffer, const char *bsFilename, unsigned drive)
{
  sprintf(buffer, "\n" \
    "[Operating Systems]\n" \
    "FreeDOS=%s\n" \
    "[FreeDOS]\n" \
    "BootType=BootSector\n" \
    "BootSector=%s\n" \
    "\n",
    menuName, bsFilename);
}


/* entry for boot.ini (check /)
   For Windows NT loader, provide a boot sector and name to display
   (note: if using MSDOS and MS Windows 95 you may need /WIN95 & /WIN95DOS)
   http://www.tburke.net/info/ntldr/ntldr_hacking_guide.htm
*/
void getNtldrConfig(char *buffer, const char *bsFilename, unsigned drive)
{
  sprintf(buffer, "\n" \
    "%s=%s\n" \
    "\n",
    bsFilename, menuName);
}

static const char *grubFormatDrive(unsigned drive)
{
  static char disk[8];
  if (drive >= 'C')
    sprintf(disk, "hd%u", (drive-'C'));
  else
    sprintf(disk, "fd%u", (drive-'A'));
  return disk;
}

static const char *grubFormatPath(const char *bsFilename)
{
  static char buffer[SYS_MAXPATH];
  register char *p;
  /* skip drive */
  if (bsFilename[1] == ':')
    strcpy(buffer, bsFilename+2);
  else
    strcpy(buffer, bsFilename);
  for (p = buffer; *p != '\0'; p++)
  {
    if (*p == '\\') *p = '/';
  }  
  return bsFilename;
}

/* entry for menu.lst (check /, /grub/, /boot/grub/)
   http://www.gnu.org/software/grub/manual/legacy/grub.html
*/
void getGrubConfig(char *buffer, const char *bsFilename, unsigned drive)
{
  sprintf(buffer, "\n" \
    "title %s\n" \
    "root (%s,0)\n" /* or possibly "rootnoverify (hd0,0)\n", use fd0 for floppy, note 0 based partitions */ \
    "chainloader %s\n"   /* or chainloader +1 to read from drive or even better chainloader /kernel.sys */ \
    /* "makeactive\n" */ \
    "\n",
    menuName, grubFormatDrive(drive), grubFormatPath(bsFilename));
}

/* entry for grub.cfg (burg.cfg also?) (check /, /grub/, /boot/grub/, /boot/grub2/, /grub2/)
   http://www.gnu.org/software/grub/manual/grub.html
*/
void getGrub2Config(char *buffer, const char *bsFilename, unsigned drive)
{
  sprintf(buffer, "\n" \
    "menuentry %s {\n" \
    "insmod chain\n" \
    "set root=(%s,1)\n"  /* can leave out so uses default (where grub installed), also supports hd0=0x80, note 1 based partitions */ \
    "chainloader %s\n"   /* or chainloader +1 to read from drive */ \
    /* "parttool ${root} boot+\n" */ \
    "}\n" \
    "\n",
    menuName, grubFormatDrive(drive), grubFormatPath(bsFilename));
}


BOOL writeBootLoaderEntry(SYSOptions *opts)
{
  getConfigSectionTextFn fn = NULL;
  const char *cfgFilename;
  static char buffer[4*SYS_MAXPATH];
  char drive = 'A' + opts->dstDrive;
  
  if (opts->verbose)
    printf("Adding entry to boot manager.\n");
  
  memset(buffer, 0, sizeof(buffer));
  switch(opts->addToBtMgr)
  {
    case USEBTMGR:
      /* fall through testing each in order they appear here */
    case SYSLINUX:
      if ((cfgFilename = getBootConfigFile(drive, "syslinux.cfg", "syslinux")) != NULL)
      {
        #ifdef DEBUG
          printf("SysLinux\n");
        #endif
        opts->addToBtMgr = SYSLINUX;
        fn = getSyslinuxConfig;
        break;
      }
    case FREELDR:
      if ((cfgFilename = getBootConfigFile(drive, "freeldr.ini", NULL)) != NULL)
      {
        #ifdef DEBUG
          printf("FreeLdr\n");
        #endif
        opts->addToBtMgr = FREELDR;
        fn = getFreeLdrConfig;
        break;
      }
    case NTLDR:
      if ((cfgFilename = getBootConfigFile(drive, "boot.ini", NULL)) != NULL)
      {
        #ifdef DEBUG
          printf("Ntldr\n");
        #endif
        opts->addToBtMgr = NTLDR;
        fn = getNtldrConfig;
        break;
      }
    case GRUB:
      if ((cfgFilename = getBootConfigFile(drive, "menu.lst", "grub")) != NULL)
      {
        #ifdef DEBUG
          printf("Grub Legacy\n");
        #endif
        opts->addToBtMgr = GRUB;
        fn = getGrubConfig;
        break;
      }
    case GRUB2:
      if ( ((cfgFilename = getBootConfigFile(drive, "grub.cfg", "grub2")) != NULL) ||
           ((cfgFilename = getBootConfigFile(drive, "grub.cfg", "grub")) != NULL) )
      {
        #ifdef DEBUG
          printf("Grub2\n");
        #endif
        opts->addToBtMgr = GRUB2;
        fn = getGrub2Config;
        break;
      }
  }
  if (fn != NULL)
  {
    char bsf[SYS_MAXPATH];
    truename(bsf, opts->bsFile);
    fn(buffer, bsf, opts->defBootDrive);
    #ifdef DEBUG
      if (opts->verbose)
          printf("Updating %s with:\n%s", cfgFilename, buffer);
    #endif
    return writeConfigSection(cfgFilename, buffer);
  }
  return FALSE;
}

#endif /* USEBOOTMANAGER */
