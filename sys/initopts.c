/***************************************************************

                                    initopts.c
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


#define FREEDOS_FILES      { "KERNEL.SYS", NULL, 0x60/*:0*/, 1, 0 },
DOSBootFiles bootFiles[] = {
  /* Note: This order is the order OEM:AUTO uses to determine DOS flavor. */
#ifdef OEM_FILES
OEM_FILES
#endif
#ifndef DRSYS
  /* FreeDOS */   FREEDOS_FILES
#endif
  /* DR-DOS  */ { "DRBIO.SYS", "DRDOS.SYS", 0x70/*:0*/, 1, 1 },
  /* DR-DOS  */ { "IBMBIO.COM", "IBMDOS.COM", 0x70/*:0*/, 1, 1 },
#ifdef DRSYS
  /* FreeDOS */   FREEDOS_FILES
#endif
#ifdef WITHOEMCOMPATBS
  /* PC-DOS  */ { "IBMBIO.COM", "IBMDOS.COM", /*0x70:*/0x0, 0, 6138 },  /* pre v7 DR ??? */
  /* MS-DOS  */ { "IO.SYS", "MSDOS.SYS", /*0x70:*/0x0, 0, 10240 },
  /* W9x-DOS */ { "IO.SYS", "MSDOS.SYS", /*0x70:*/0x0200, 0, 0 },
  /* Rx-DOS  */ { "RXDOSBIO.SYS", "RXDOS.SYS", /*0x70:*/0x0, 0, 1 },
  /* DRMK    */ { "DELLBIO.BIN", "DELLRMK.BIN", /*0x70:*/0x0, 0, 1 },
#endif
#ifdef METAKERN
  /* METAKERN*/ { "METAKERN.SYS", "KERNEL.SYS", 0x60/*:0*/, 1, 0 },
#endif
#ifdef FREELDR
  /* ReactOS */ { "FREELDR.SYS", "FREELDR.INI", /*0x0800:*/0x0, 1, 0 },
#if 0
  /* NTLdr   */ { "NTLDR", "BOOT.INI", /*0x70:*/0x0, 0, 0 },
  /* NTBtMgr */ { "BOOTMGR", NULL, /*0x70:*/0x0, 0, 0 },
#endif
#endif
};
#define DOSFLAVORS (sizeof(bootFiles) / sizeof(*bootFiles))

/* associate friendly name with index into bootFiles array */
#define OEM_AUTO (-1) /* attempt to guess DOS on source drive */
#ifdef OEM_FILES
#define MSG_BASE   1
#define OEM_CUSTOM 0
#else
#define MSG_BASE   0
#endif
#ifndef DRSYS
#define OEM_FD     (MSG_BASE+0)  /* standard FreeDOS mode */
#define OEM_EDR    (MSG_BASE+1)  /* use FreeDOS boot sector, but OEM names */
#define OEM_DR     (MSG_BASE+2)  /* FD boot sector,(Enhanced) DR-DOS names */
#else
#define OEM_FD     (MSG_BASE+2)  /* standard FreeDOS mode */
#define OEM_EDR    (MSG_BASE+0)  /* use FreeDOS boot sector, but OEM names */
#define OEM_DR     (MSG_BASE+1)  /* FD boot sector,(Enhanced) DR-DOS names */
#endif
#ifdef WITHOEMCOMPATBS
#define OEM_PC     (MSG_BASE+3)  /* use PC-DOS compatible boot sector and names */ 
#define OEM_MS     (MSG_BASE+4)  /* use PC-DOS compatible BS with MS names */
#define OEM_W9x    (MSG_BASE+5)  /* use PC-DOS compatible BS with MS names */
#define OEM_RX     (MSG_BASE+6)  /* use PC-DOS compatible BS with Rx names */
#define OEM_DRMK   (MSG_BASE+7)  /* use PC-DOS compatible BS with Dell names */
#define OEM_META   (MSG_BASE+8)
#else
#define OEM_META   (MSG_BASE+3)  /* use FreeDOS boot sector for METAKERN */
#endif
#ifdef METAKERN
#define OEM_FLDR   (OEM_META+1)  /* use FreeLoader - Reactos compatible mode */
#else
#define OEM_FLDR   (OEM_META+0)
#endif
#ifdef FREELDR
#if 0
#define OEM_NLDR   (MSG_BASE+9)  /* use NTLoader - MS Windows NT 3,4,5 mode */
#define OEM_NMGR   (MSG_BASE+10) /* use BootManager - MS Windows NT 6,7 mode */
#endif
#endif

CONST char * msgDOS[DOSFLAVORS] = {  /* order should match above items */
#ifdef OEM_FILES
  "OEM mode\n",  /* only shown if custom kernel files used */
#endif
  "\n",  /* In standard FreeDOS/EnhancedDR mode, don't print anything special */
#ifndef DRSYS
  "Enhanced DR DOS (OpenDOS Enhancement Project) mode\n",
#endif
  "DR DOS (OpenDOS Enhancement Project) mode\n",
#ifdef DRSYS
  "\n",  /* FreeDOS mode so don't print anything special */
#endif
#ifdef WITHOEMCOMPATBS
  "PC-DOS compatibility mode\n",
  "MS-DOS compatibility mode\n",
  "Win9x DOS compatibility mode\n",
  "RxDOS compatibility mode\n",
  "Dell Real Mode Kernel (DRMK) mode\n",
#endif
#ifdef METAKERN
  "MetaKern\n",
#endif
#ifdef FREELDR
  "ReactOS FreeLdr\n"
#if 0
  "NTLdr\n"
  "NTBtMgr\n"
#endif
#endif
};


#ifdef _WIN32
#define getcurdrive -1+(unsigned)_getdrive
#else
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
#endif
#endif


/* get and validate arguments */
void initOptions(int argc, char *argv[], SYSOptions *opts)
{
  int argno;
  int drivearg = 0;           /* drive argument, position of 1st or 2nd non option */
  int srcarg = 0;             /* nonzero if optional source argument */
  BYTE srcFile[SYS_MAXPATH];  /* full path+name of [kernel] file [to copy] */
  struct stat fstatbuf;
  void (*otherAction)(SYSOptions *opts) = NULL;

  /* initialize to defaults */
  memset(opts, 0, sizeof(SYSOptions));
  /* set srcDrive and dstDrive after processing args */
  opts->flavor = OEM_AUTO;      /* attempt to detect DOS user wants to boot */
  opts->copyKernel = 1;         /* actually copy the kernel and cmd interpreter to dstDrive */
  opts->copyShell = 1;

  /* cycle through processing cmd line arguments */
  for(argno = 1; argno < argc; argno++)
  {
    char *argp = argv[argno];

    if (argp[0] == '/')  /* optional switch */
    {
      argp++;  /* skip past the '/' character */

      /* explicit request for base help/usage */
      if ((*argp == '?') || (memicmp(argp, "HELP", 4) == 0))
      {
        /* see if requesting specific help details */
        if ((argno+1)<argc)
        {    
          /* help on OEM options */
          if (memicmp(argv[argno+1], "OEM", 3) == 0)
            showOemHelpAndExit();
#ifdef FDCONFIG
          else if (memicmp(argv[argno+1], "CONFIG", 6) == 0)
            exit(FDKrnConfigMain(argc, argv));
#endif
          /* else bad option so fall through to standard usage help */
        }
        showHelpAndExit();
      }
      /* enable extra (DEBUG) output */
      else if (memicmp(argp, "VERBOSE", 7) == 0)
      {
        opts->verbose = 1;
      }
      /* write to *both* the real boot sector and the image file */
      else if (memicmp(argp, "BOTH", 4) == 0)
      {
        opts->writeBS = 1;  /* note: if bs file omitted, then same as omitting /BOTH */
      }
      /* do *not* copy kernel / shell, only update boot sector or image */
      else if (memicmp(argp, "BOOTONLY", 8) == 0)
      {
        opts->copyKernel = 0;
        opts->copyShell = 0;
      }
      /* copy kernel and update boot sector (do *not* copy shell) */
      else if (memicmp(argp, "UPDATE", 6) == 0)
      {
        opts->copyKernel = 1;
        opts->copyShell = 0;
      }
      /* indicates compatibility mode, fs, filenames, and load segment to use */
      else if (memicmp(argp, "OEM", 3) == 0)
      {
        char *msgBadOEM = "%s: unknown OEM qualifier %s\n";
        argp += 3;
        if (!*argp)
            opts->flavor = OEM_AUTO;
        else if (*argp == ':')
        {
          argp++;  /* point to DR/PC/MS that follows */
          if (memicmp(argp, "AUTO", 4) == 0)
            opts->flavor = OEM_AUTO;
          else if (memicmp(argp, "EDR", 3) == 0)
            opts->flavor = OEM_EDR;
          else if (memicmp(argp, "DR", 2) == 0)
            opts->flavor = OEM_DR;
#ifdef WITHOEMCOMPATBS
          else if (memicmp(argp, "PC", 2) == 0)
            opts->flavor = OEM_PC;
          else if (memicmp(argp, "MS", 2) == 0)
            opts->flavor = OEM_MS;
          else if (memicmp(argp, "W9", 2) == 0)
            opts->flavor = OEM_W9x;
          else if (memicmp(argp, "RX", 2) == 0)
            opts->flavor = OEM_RX;
          else if (memicmp(argp, "DE", 2) == 0) /* DELL */
            opts->flavor = OEM_DRMK;
#endif
#ifdef METAKERN
#endif
#ifdef FREELDR
          else if (memicmp(argp, "FRL", 3) == 0)
            opts->flavor = OEM_FLDR;
#if 0
          else if (memicmp(argp, "NT", 2) == 0)
            opts->flavor = OEM_NLDR;
#endif
#endif
          else if (memicmp(argp, "FD", 2) == 0)
            opts->flavor = OEM_FD;
          else
          {
            printf(msgBadOEM, pgm, argp);
            showHelpAndExit();
          }
        }
        else
        {
          printf(msgBadOEM, pgm, argp);
          showHelpAndExit();
        }
      }
#ifdef USEBOOTMANAGER
      /* indicates compatibility mode, fs, filenames, and load segment to use */
      else if (memicmp(argp, "BOOTMGR", 7) == 0)
      {
        char *msgBadBtMgr = "%s: unknown boot manager %s\n";
        argp += 7;
        if (!*argp)
            /* auto detect boot manager to add entry to */
            opts->addToBtMgr = USEBTMGR;
        else if (*argp == ':')
        {
          argp++;  /* point to FreeLDR/NTLDR/SYSLINUX that follows */
          if (memicmp(argp, "AUTO", 4) == 0)
            /* auto detect boot manager to add entry to */
            opts->addToBtMgr = USEBTMGR;
          else if (memicmp(argp, "SYS"/*LINUX*/, 3) == 0)
            /* use specified boot manager to add entry to */
            opts->addToBtMgr = SYSLINUX;
          else if (memicmp(argp, "Fre"/*eLdr*/, 3) == 0)
            /* use specified boot manager to add entry to */
            opts->addToBtMgr = FREELDR;
          else if (memicmp(argp, "NTL"/*DR*/, 3) == 0)
            /* use specified boot manager to add entry to */
            opts->addToBtMgr = NTLDR;
          else if (memicmp(argp, "GRUB", 5) == 0)
            /* use specified boot manager to add entry to */
            opts->addToBtMgr = GRUB;
          else if (memicmp(argp, "GRUB2", 5) == 0)
            /* use specified boot manager to add entry to */
            opts->addToBtMgr = GRUB2;
          else
          {
            printf(msgBadBtMgr, pgm, argp);
            showHelpAndExit();
          }
        }
        else
        {
          printf(msgBadBtMgr, pgm, argp);
          showHelpAndExit();
        }
      }
#endif /* USEBOOTMANAGER */
      /* override auto options */
      else if (memicmp(argp, "FORCE", 5) == 0)
      {
        argp += 5;
        if (*argp == ':')
        {
          argp++;  /* point to CHS/LBA/... that follows */

          /* specify which BIOS access mode to use */
          if (memicmp(argp, "AUTO", 4) == 0) /* default */
            opts->force = AUTO;
          else if (memicmp(argp, "CHS", 3) == 0)
            opts->force = CHS;
          else if (memicmp(argp, "LBA", 3) == 0)
            opts->force = LBA;

          /* specify if BIOS or BOOTSECTOR provided boot drive # is to be used */
          else if (memicmp(argp, "BSDRV", 5) == 0) /* same as FORCEDRV */
            opts->ignoreBIOS = 1;
          else if (memicmp(argp, "BIOSDRV", 7) == 0) /* always use BIOS passed */
            opts->ignoreBIOS = -1;
          else
          {
            printf("%s: invalid FORCE qualifier %s\n", pgm, argp);
            showHelpAndExit();
          }
        }
        else if (memicmp(argp, "DRV", 3) == 0) /* FORCEDRV */
        {
          /* force use of drive # set in bs instead of BIOS boot value */
          /* deprecated, use FORCE:BSDRV                               */
          opts->ignoreBIOS = 1;
        }
        else
        {
          printf("%s: invalid FORCE qualifier %s\n", pgm, argp);
          showHelpAndExit();
        }
      }
      /* skips copying boot sector to backup bs, FAT32 only else ignored */
      else if (memicmp(argp, "NOBAKBS", 7) == 0)
      {
        opts->skipBakBSCopy = 1;
      }
      else if (argno + 1 < argc)   /* two part options, /SWITCH VALUE */
      {
        argno++;
        if (toupper(*argp) == 'K')      /* set Kernel name */
        {
          opts->kernel.kernel = argv[argno];
        }
        else if (toupper(*argp) == 'L') /* set Load segment */
        {
          opts->kernel.loadaddr = (WORD)strtol(argv[argno], NULL, 16);
        }
        else if (memicmp(argp, "B", 2) == 0) /* set boot drive # */
        {
          opts->defBootDrive = (BYTE)strtol(argv[argno], NULL, 16);
        }
        /* options not documented by showHelpAndExit() */
        else if (memicmp(argp, "SKFN", 4) == 0) /* set KERNEL.SYS input file and /OEM:FD */
        {
          opts->flavor = OEM_FD;
          opts->fnKernel = argv[argno];
        }
        else if (memicmp(argp, "SCFN", 4) == 0) /* sets COMMAND.COM input file */
        {
          opts->fnCmd = argv[argno];
        }
        else if (memicmp(argp, "BACKUPBS", 8) == 0) /* save current bs before overwriting */
        {
          opts->bsFileOrig = argv[argno];
        }
        else if ((memicmp(argp, "DUMPBS", 6) == 0) || (memicmp(argp, "GETBS", 5) == 0)) /* save current bs and exit */
        {
          otherAction = dumpBS;
          opts->altBSCode = argv[argno];
        }
        else if ((memicmp(argp, "RESTORBS", 8) == 0) || (memicmp(argp, "RESTOREBS", 9) == 0)) /* overwrite bs (using unmodified external file) and exit */
        {
          otherAction = restoreBS;
          opts->altBSCode = argv[argno];
        }
        else if (memicmp(argp, "PUTBS", 5) == 0) /* overwrite bs (using external file but adjusting bpb) and exit */
        {
          otherAction = putBS;
          opts->altBSCode = argv[argno];
        }
        else if (memicmp(argp, "BSCODE", 6) == 0) /* specify external code to using instead of internal boot code */
        {
          opts->altBSCode = argv[argno];
        }
        else if (memicmp(argp, "BSCOUNT", 7) == 0) /* how many SECTOR_SIZE sectors boot code is */
        {
          /* opts->??? = (WORD)strtol(argv[argno], NULL, 10); */
          printf("Warning: ignoring option BSCOUNT\n");
        }
        else
        {
          printf("%s: unknown option, %s\n", pgm, argv[argno]);
          showHelpAndExit();
        }
      }
      else
      {
        printf("%s: unknown option or missing parameter, %s\n", pgm, argv[argno]);
        showHelpAndExit();
      }
    }
    else if (!drivearg)
    {
      drivearg = argno;         /* either source or destination drive */
    }
    else if (!srcarg /* && drivearg */ && !opts->bsFile)
    {
      /* need to determine is user specified [source] dest or dest [bootfile] (or [source] dest [bootfile])
         - dest must be either X or X: as only a drive specifier without any path is valid -
         if 1st arg not drive and 2nd is then [source] dest form
         if 1st arg drive and 2nd is not drive then dest [bootfile] form
         if both 1st arg and 2nd are not drives then invalid arguments
         if both 1st arg and 2nd are drives then assume [source] dest form (use ./X form is single letter used)
      */
      if (!argv[drivearg][1] || (argv[drivearg][1]==':' && !argv[drivearg][2])) /* if 1st arg drive */
      {
        if (!argv[argno][1] || (argv[argno][1]==':' && !argv[argno][2])) /* if 2nd arg drive */
        {
          srcarg = drivearg; /* set source path */
          drivearg = argno;  /* set destination drive */
        }
        else
        {
          opts->bsFile = argv[argno];
        }
      }
      else
      {
        if (!argv[argno][1] || (argv[argno][1]==':' && !argv[argno][2])) /* if 2nd arg drive */
        {
          srcarg = drivearg; /* set source path */
          drivearg = argno;  /* set destination drive */
        }
        else
        {
          goto EXITBADARG;
        }
      }
    }
    else if (!opts->bsFile /* && srcarg && drivearg */)
    {
      opts->bsFile = argv[argno];
    }
    else /* if (opts->bsFile && srcarg && drivearg) */
    {
      EXITBADARG:
      printf("%s: invalid argument %s\n", pgm, argv[argno]);
      showHelpAndExit();
    }
  } /* for */

  /* set dest path */
  if (!drivearg)
    showHelpAndExit();
  opts->dstDrive = (BYTE)(toupper(*(argv[drivearg])) - 'A');
  if (/* (opts->dstDrive < 0) || */ (opts->dstDrive >= 26))
  {
    printf("%s: drive %c must be A:..Z:\n", pgm, *(argv[drivearg]));
    exit(1);
  }

#ifdef USEBOOTMANAGER
  /* if want to add to boot manager but no name set, use default; error if /BOTH used */
  if (opts->addToBtMgr != NONE)
  {
    if (opts->writeBS)
    {
      printf("%s: /BOTH and /BTMGR can NOT both be used!\n", pgm);
      showHelpAndExit();
    }
    if (!opts->bsFile)
      opts->bsFile = "/FREEDOS.BSS";
  }
#endif
  
  /* if neither BOTH nor a boot sector file specified, then write to boot record */
  if (!opts->bsFile)
    opts->writeBS = 1;

  /* set source path, default to current drive */
  sprintf(opts->srcDrive, "%c:", 'A' + getcurdrive());
  if (srcarg)
  {
    int slen;
    /* set source path, reserving room to append filename */
    if ( (argv[srcarg][1] == ':') || ((argv[srcarg][0]=='\\') && (argv[srcarg][1] == '\\')) ) 
      strncpy(opts->srcDrive, argv[srcarg], SYS_MAXPATH-13);
    else if (argv[srcarg][1] == '\0') /* assume 1 char is drive not path specifier */
      sprintf(opts->srcDrive, "%c:", toupper(*(argv[srcarg])));
    else /* only path provided, append to default drive */
      strncat(opts->srcDrive, argv[srcarg], SYS_MAXPATH-15);
    slen = strlen(opts->srcDrive);
    /* if path follows drive, ensure ends in a slash, ie X:-->X: or X:.\mypath-->X:.\mypath\ */
    if ((slen>2) && (opts->srcDrive[slen-1] != '\\') && (opts->srcDrive[slen-1] != '/'))
      strcat(opts->srcDrive, "\\");
  }
  /* source path is now in form of just a drive, "X:" 
     or form of drive + path + directory separator, "X:\path\" or "\\path\"
     If just drive we try current path then root, else just indicated path.
  */


  /* if source and dest are same drive, then source should not be root,
     so if is same drive and not explicit path, force only current 
     Note: actual copy routine prevents overwriting self when src=dst
  */
  if ( (opts->dstDrive == (toupper(*(opts->srcDrive))-'A')) && (!opts->srcDrive[2]) )
    strcat(opts->srcDrive, ".\\");


  /* attempt to detect compatibility settings user needs */
  if (opts->flavor == OEM_AUTO)
  {
    /* 1st loop checking current just source path provided */
    for (argno = 0; argno < DOSFLAVORS; argno++)
    {
      /* look for existing file matching kernel filename */
      sprintf(srcFile, "%s%s", opts->srcDrive, bootFiles[argno].kernel);
      if (stat(srcFile, &fstatbuf)) continue; /* if !exists() try again */
      if (!fstatbuf.st_size) continue;  /* file must not be empty */

      /* now check if secondary file exists and of minimal size */
      if (bootFiles[argno].minsize)
      {
        sprintf(srcFile, "%s%s", opts->srcDrive, bootFiles[argno].dos);
        if (stat(srcFile, &fstatbuf)) continue;
        if (fstatbuf.st_size < bootFiles[argno].minsize) continue;
      }

      /* above criteria succeeded, so default to corresponding DOS */
      opts->flavor = argno;
      break;
    }

    /* if no match, and source just drive, try root */
    if ( (opts->flavor == OEM_AUTO) && (!opts->srcDrive[2]) )
    {
      for (argno = 0; argno < DOSFLAVORS; argno++)
      {
        /* look for existing file matching kernel filename */
        sprintf(srcFile, "%s\\%s", opts->srcDrive, bootFiles[argno].kernel);
        if (stat(srcFile, &fstatbuf)) continue; /* if !exists() try again */
        if (!fstatbuf.st_size) continue;  /* file must not be empty */

        /* now check if secondary file exists and of minimal size */
        if (bootFiles[argno].minsize)
        {
          sprintf(srcFile, "%s\\%s", opts->srcDrive, bootFiles[argno].dos);
          if (stat(srcFile, &fstatbuf)) continue;
          if (fstatbuf.st_size < bootFiles[argno].minsize) continue;
        }

        /* above criteria succeeded, so default to corresponding DOS */
        opts->flavor = argno;
        strcat(opts->srcDrive, "\\"); /* indicate to use root from now on */
        break;
      }
    }
  }

  /* if unable to determine DOS, assume FreeDOS */
  if (opts->flavor == OEM_AUTO) opts->flavor = 
#ifdef DRSYS
  OEM_EDR;
#else
  OEM_FD;
#endif

  printf(msgDOS[opts->flavor]);

  /* set compatibility settings not explicitly set */
  if (!opts->kernel.kernel) opts->kernel.kernel = bootFiles[opts->flavor].kernel;
  if (!opts->kernel.dos) opts->kernel.dos = bootFiles[opts->flavor].dos;
  if (!opts->kernel.loadaddr) opts->kernel.loadaddr = bootFiles[opts->flavor].loadaddr;
  opts->kernel.stdbs = bootFiles[opts->flavor].stdbs;
  opts->kernel.minsize = bootFiles[opts->flavor].minsize;


  /* did user insist on always using BIOS provided drive # */
  if (opts->ignoreBIOS == -1)
    opts->ignoreBIOS = 0;  /* its really a boolean value in rest of code */
  /* if destination is floppy (A: or B:) then use drive # stored in boot sector */
  else if (opts->dstDrive < 2)
    opts->ignoreBIOS = 1;

  /* if bios drive to store in boot sector not set and not floppy set to 1st hd */
  if (!opts->defBootDrive && (opts->dstDrive >= 2))
    opts->defBootDrive = 0x80;
  /* else opts->defBootDrive = 0x0; the 1st floppy */

  
  /* if nonstandard action, perform action and exit */
  if (otherAction != NULL) 
  {
    if (!opts->altBSCode)
      printf("%s: missing filename for boot sector file!\n", pgm);
    else
      otherAction(opts);
    exit(1);
  }

  /* unless we are only setting boot sector, verify kernel file exists */
  if (opts->copyKernel)
  {
    /* check kernel (primary file) 1st */
    sprintf(srcFile, "%s%s", opts->srcDrive, (opts->fnKernel)?opts->fnKernel:opts->kernel.kernel);
    if (stat(srcFile, &fstatbuf))  /* if !exists() */
    {
      /* check root path as well if src is drive only */
      sprintf(srcFile, "%s\\%s", opts->srcDrive, (opts->fnKernel)?opts->fnKernel:opts->kernel.kernel);
      if (opts->srcDrive[2] || stat(srcFile, &fstatbuf))
      {
        printf("%s: failed to find kernel file %s\n", pgm, (opts->fnKernel)?opts->fnKernel:opts->kernel.kernel);
        exit(1);
      }
      /* else found, but in root, so force to always use root */
      strcat(opts->srcDrive, "\\");
    }

    /* now check for secondary file */
    if (opts->kernel.dos && opts->kernel.minsize)
    {
      sprintf(srcFile, "%s%s", opts->srcDrive, opts->kernel.dos);
      if (stat(srcFile, &fstatbuf))
      {
        printf("%s: failed to find source file %s\n", pgm, opts->kernel.dos);
        exit(1);
      }
      if (fstatbuf.st_size < opts->kernel.minsize)
      {
        printf("%s: source file %s appears corrupt, invalid size\n", pgm, opts->kernel.dos);
        exit(1);
      }
    }
  }

  /* if updating or only setting bootsector then skip this check */
  if (opts->copyShell)
  {
    /* lastly check for command interpreter, 1st try source path, then try %COMSPEC% */
    sprintf(srcFile, "%s%s", opts->srcDrive, (opts->fnCmd)?opts->fnCmd:"COMMAND.COM");
    if (stat(srcFile, &fstatbuf))  /* if !exists() */
    {
      char *comspec = getenv("COMSPEC");
      /* don't use comspec if shell filename specified, comspec env var not found, or file pointed to not exists */
      if (opts->fnCmd || (comspec == NULL) || stat(comspec, &fstatbuf))
      {
        printf("%s: failed to find command interpreter (shell) file %s\n", pgm, srcFile);
        exit(1);
      }
      else
      {
          printf("%s: Using shell from %%COMSPEC%%=\"%s\"\n", pgm, comspec);
          opts->fnCmd = strdup(comspec);  /* note: memory never free'd, but that's ok */
      }
    }
    else
        opts->fnCmd = strdup(srcFile); /* store full path to command shell to use; note: memory never free'd */
  } /* copy shell */

}
