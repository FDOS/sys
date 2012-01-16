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


#ifdef __WATCOMC__
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
