/***************************************************************

                                    sys.c
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


#ifndef __WATCOMC__

/* returns current DOS drive, A=0, B=1,C=2, ... */
#ifdef __TURBOC__
#define getcurdrive (unsigned)getdisk
#else
unsigned getcurdrive(void)
{
  union REGS regs;
  regs.h.ah = 0x19;
  int86(0x21, &regs, &regs);
  return regs.h.al;
}
#endif

#else

/* returns current DOS drive, A=0, B=1,C=2, ... */
unsigned getcurdrive(void);
#pragma aux getcurdrive = \
      "mov ah, 0x19"      \
      "int 0x21"          \
      "xor ah, ah"        \
      value [ax];


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

extern int unlink(const char *pathname);

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

#define close _dos_close

int stat(const char *file_name, struct stat *statbuf)
{
  struct find_t find_tbuf;

  int ret = _dos_findfirst(file_name, _A_NORMAL | _A_HIDDEN | _A_SYSTEM, &find_tbuf);
  statbuf->st_size = (off_t)find_tbuf.size;
  /* statbuf->st_attr = (ULONG)find_tbuf.attrib; */
  return ret;
}

/* WATCOM's getenv is case-insensitive which wastes a lot of space
   for our purposes. So here's a simple case-sensitive one */
char *getenv(const char *name)
{
  char **envp, *ep;
  const char *np;
  char ec, nc;

  for (envp = environ; (ep = *envp) != NULL; envp++) {
    np = name;
    do {
      ec = *ep++;
      nc = *np++;
      if (nc == 0) {
        if (ec == '=')
          return ep;
        break;
      }
    } while (ec == nc);
  }
  return NULL;
}
#endif


BYTE pgm[] = "SYS";


int main(int argc, char **argv)
{
  SYSOptions opts;            /* boot options and other flags */
  BYTE srcFile[SYS_MAXPATH];  /* full path+name of [kernel] file [to copy] */

  printf(SYS_NAME SYS_VERSION ", " __DATE__ "\n");

#ifdef FDCONFIG
  if (argc > 1 && memicmp(argv[1], "CONFIG", 6) == 0)
  {
    exit(FDKrnConfigMain(argc, argv));
  }
#endif

  initOptions(argc, argv, &opts);

  printf("Processing boot sector...\n");
  put_boot(&opts);

  if (opts.copyKernel)
  {
    printf("Now copying system files...\n");

    sprintf(srcFile, "%s%s", opts.srcDrive, (opts.fnKernel)?opts.fnKernel:opts.kernel.kernel);
    if (!copy(srcFile, opts.dstDrive, opts.kernel.kernel))
    {
      printf("%s: cannot copy \"%s\"\n", pgm, srcFile);
      exit(1);
    } /* copy kernel */

    if (opts.kernel.dos)
    {
      sprintf(srcFile, "%s%s", opts.srcDrive, opts.kernel.dos);
      if (!copy(srcFile, opts.dstDrive, opts.kernel.dos) && opts.kernel.minsize)
      {
        printf("%s: cannot copy \"%s\"\n", pgm, srcFile);
        exit(1);
      } /* copy secondary file (DOS) */
    }
  }

  if (opts.copyShell)
  {
    printf("Copying shell (command interpreter)...\n");
  
    /* copy command.com, 1st try source path, then try %COMSPEC% */
    sprintf(srcFile, "%s%s", opts.srcDrive, (opts.fnCmd)?opts.fnCmd:"COMMAND.COM");
    if (!copy(srcFile, opts.dstDrive, "COMMAND.COM"))
    {
      char *comspec = getenv("COMSPEC");
      if (!opts.fnCmd && (comspec != NULL))
        printf("%s: Trying shell from %%COMSPEC%%=\"%s\"\n", pgm, comspec);
      if (opts.fnCmd || (comspec == NULL) || !copy(comspec, opts.dstDrive, "COMMAND.COM"))
      {
        printf("\n%s: failed to find command interpreter (shell) file %s\n", pgm, (opts.fnCmd)?opts.fnCmd:"COMMAND.COM");
        exit(1);
      }
    } /* copy shell */
  }

  printf("\nSystem transferred.\n");
  return 0;
}
