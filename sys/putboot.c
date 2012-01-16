/***************************************************************

                                    putboot.c
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

#include "fat12com.h"
#include "fat16com.h"
#ifdef WITHFAT32
#include "fat32chs.h"
#include "fat32lba.h"
#endif
#ifdef WITHOEMCOMPATBS
#include "oemfat12.h"
#include "oemfat16.h"
#endif


#define SEC_SIZE        512


struct bootsectortype {
  UBYTE bsJump[3];
  char OemName[8];
  UWORD bsBytesPerSec;
  UBYTE bsSecPerClust;
  UWORD bsResSectors;
  UBYTE bsFATs;
  UWORD bsRootDirEnts;
  UWORD bsSectors;
  UBYTE bsMedia;
  UWORD bsFATsecs;
  UWORD bsSecPerTrack;
  UWORD bsHeads;
  ULONG bsHiddenSecs;
  ULONG bsHugeSectors;
  UBYTE bsDriveNumber;
  UBYTE bsReserved1;
  UBYTE bsBootSignature;
  ULONG bsVolumeID;
  char bsVolumeLabel[11];
  char bsFileSysType[8];
};

struct bootsectortype32 {
  UBYTE bsJump[3];
  char OemName[8];
  UWORD bsBytesPerSec;
  UBYTE bsSecPerClust;
  UWORD bsResSectors;
  UBYTE bsFATs;
  UWORD bsRootDirEnts;
  UWORD bsSectors;
  UBYTE bsMedia;
  UWORD bsFATsecs;
  UWORD bsSecPerTrack;
  UWORD bsHeads;
  ULONG bsHiddenSecs;
  ULONG bsHugeSectors;
  ULONG bsBigFatSize;
  UBYTE bsFlags;
  UBYTE bsMajorVersion;
  UWORD bsMinorVersion;
  ULONG bsRootCluster;
  UWORD bsFSInfoSector;
  UWORD bsBackupBoot;
  ULONG bsReserved2[3];
  UBYTE bsDriveNumber;
  UBYTE bsReserved3;
  UBYTE bsExtendedSignature;
  ULONG bsSerialNumber;
  char bsVolumeLabel[11];
  char bsFileSystemID[8];
};

/*
 * globals needed by put_boot & check_space
 */
//enum {FAT12 = 12, FAT16 = 16, FAT32 = 32} fs;  /* file system type */
FileSystem fs;

#define SBOFFSET        11
#define SBSIZE          (sizeof(struct bootsectortype) - SBOFFSET)
#define SBSIZE32        (sizeof(struct bootsectortype32) - SBOFFSET)

/* essentially - verify alignment on byte boundaries at compile time  */
struct VerifyBootSectorSize {
  char failure1[sizeof(struct bootsectortype) == 62 ? 1 : -1];
  char failure2[sizeof(struct bootsectortype) == 62 ? 1 : 0];
/* (Watcom has a nice warning for this, by the way) */
};



#ifdef DDEBUG
VOID dump_sector(unsigned char far * sec)
{
  COUNT x, y;
  char c;

  for (x = 0; x < 32; x++)
  {
    printf("%03X  ", x * 16);
    for (y = 0; y < 16; y++)
    {
      printf("%02X ", sec[x * 16 + y]);
    }
    for (y = 0; y < 16; y++)
    {
      c = sec[x * 16 + y];
      if (isprint(c))
        printf("%c", c);
      else
        printf(".");
    }
    printf("\n");
  }

  printf("\n");
}

#endif


/* copies ASCIIZ string to directory 83 format padded with spaces */
void setFilename(char *buffer, char const *filename)
{
  int i;
  /* pad with spaces, if name.ext is less than 8.3 blanks filled with spaces */
  memset(buffer, ' ', 11);
  /* copy over up to 8 characters of filename and 3 characters of extension */
  for (i = 0; *filename && (*filename != '.'); i++, filename++)
    if (i<FNAME_SIZE) buffer[i] = toupper(*filename);
  if (*filename == '.')
  {
    filename++; /* skip past . */
    for (i = 8; (i < 11) && *filename; i++, filename++)
      buffer[i] = toupper(*filename);
  }
}


#ifdef WITHOEMCOMPATBS
/* for FAT12/16 rearranges root directory so kernel & dos files are 1st two entries */
void updateRootDir(ULONG rootSector, UCOUNT rootDirSectors, char const *kernel, char const *dos)
{
  struct dirent *dir;
  struct lfn_entry *lfn;
  BYTE buffer[SEC_SIZE];
  BYTE filename[11];
  
  setFilename(filename, kernel);
  // read in sector
  if (MyAbsReadWrite(opts->dstDrive, 1, rootSector, buffer, 0) != 0)
  {
    printf("Error reading root directory, not updated!\n");
    return;
  }
  // loop through directory entries until kernel found or 
  dir = (struct dirent *)buffer;
  lfn = (struct lfn_entry *)dir;
  if ((void*)dir == (void*)lfn) dir = (void *)lfn;
}
#endif


void correct_bpb(struct bootsectortype *default_bpb,
                 struct bootsectortype *oldboot, BOOL verbose)
{
  /* don't touch partitions (floppies most likely) that don't have hidden
     sectors */
  if (default_bpb->bsHiddenSecs == 0)
    return;

  if (verbose)
  {
    printf("Old boot sector values: sectors/track: %u, heads: %u, hidden: %lu\n",
           oldboot->bsSecPerTrack, oldboot->bsHeads, oldboot->bsHiddenSecs);
    printf("Default and new boot sector values: sectors/track: %u, heads: %u, "
           "hidden: %lu\n", default_bpb->bsSecPerTrack, default_bpb->bsHeads,
           default_bpb->bsHiddenSecs);
  }

  oldboot->bsSecPerTrack = default_bpb->bsSecPerTrack;
  oldboot->bsHeads = default_bpb->bsHeads;
  oldboot->bsHiddenSecs = default_bpb->bsHiddenSecs;
}


/* reads in boot sector (1st SEC_SIZE bytes) from file */
void readBS(const char *bsFile, UBYTE *bootsector)
{
  if (bsFile != NULL)
  {
    int fd;

#ifdef DEBUG
    printf("reading bootsector from file %s\n", bsFile);
#endif

    /* open boot sector file, it must exists, then overwrite
       drive with 1st SEC_SIZE bytes from the [image] file
    */
    if ((fd = open(bsFile, O_RDONLY | O_BINARY)) < 0)
    {
      printf("%s: can't open\"%s\"\nDOS errnum %d", pgm, bsFile, errno);
      exit(1);
    }
    if (read(fd, bootsector, SEC_SIZE) != SEC_SIZE)
    {
      printf("%s: failed to read %u bytes from %s\n", pgm, SEC_SIZE, bsFile);
      close(fd);
      exit(1);
    }
    close(fd);
  }
}

/* write bs in bsFile to drive's boot record unmodified */
void restoreBS(const char *bsFile, int drive)
{
  UBYTE bootsector[SEC_SIZE];

  if (bsFile == NULL)
  {
    printf("%s: missing filename of boot sector to restore\n", pgm);
    exit(1);
  }

  readBS(bsFile, bootsector);

  /* lock drive */
  generic_block_ioctl(drive + 1, 0x84a, NULL);

  reset_drive(drive);

  /* write bootsector to drive */
  if (MyAbsReadWrite(drive, 1, 0, bootsector, 1) != 0)
  {
    printf("%s: failed to write boot sector to drive %c:\n", pgm, drive + 'A');
    exit(1);
  }

  reset_drive(drive);

  /* unlock_drive */
  generic_block_ioctl(drive + 1, 0x86a, NULL);
}

/* write bootsector to file bsFile */
void saveBS(const char *bsFile, UBYTE *bootsector)
{
  if (bsFile != NULL)
  {
    int fd;

#ifdef DEBUG
    printf("writing bootsector to file %s\n", bsFile);
#endif

    /* open boot sector file, create it if not exists,
       but don't truncate if exists so we can replace
       1st SEC_SIZE bytes of an image file
    */
    if ((fd = open(bsFile, O_WRONLY | O_CREAT | O_BINARY,
              S_IREAD | S_IWRITE)) < 0)
    {
      printf("%s: can't create\"%s\"\nDOS errnum %d", pgm, bsFile, errno);
      exit(1);
    }
    if (write(fd, bootsector, SEC_SIZE) != SEC_SIZE)
    {
      printf("%s: failed to write %u bytes to %s\n", pgm, SEC_SIZE, bsFile);
      close(fd);
      /* unlink(bsFile); don't delete in case was image */
      exit(1);
    }
    close(fd);
  } /* if write boot sector file */
}

/* write drive's boot record unmodified to bsFile */
void dumpBS(const char *bsFile, int drive)
{
  UBYTE bootsector[SEC_SIZE];

  if (bsFile == NULL)
  {
    printf("%s: missing filename to dump boot sector to\n", pgm);
    exit(1);
  }

  /* lock drive */
  generic_block_ioctl(drive + 1, 0x84a, NULL);

  reset_drive(drive);

  /* suggestion: allow reading from a boot sector or image file here */
  if (MyAbsReadWrite(drive, 1, 0, bootsector, 0) != 0)
  {
    printf("%s: failed to read boot sector for drive %c:\n", pgm, drive + 'A');
    exit(1);
  }

  reset_drive(drive);

  /* unlock_drive */
  generic_block_ioctl(drive + 1, 0x86a, NULL);

  saveBS(bsFile, bootsector);
}



void put_boot(SYSOptions *opts)
{
#ifdef WITHFAT32
  struct bootsectortype32 *bs32;
#endif
  struct bootsectortype *bs;
  UBYTE oldboot[SEC_SIZE], newboot[SEC_SIZE];
  UBYTE default_bpb[0x5c];
  int bsBiosMovOff;  /* offset in bs to mov [drive],dl that we NOP out */

  /* for OEM kernels, ensure 1st two root dir entries correspond with kernel files */
  ULONG rootSector; /* first data sector, for FAT12/16 also 1st sector of root directory */
  UCOUNT rootDirSectors;
  
  if (opts->verbose)
  {
    printf("Reading old bootsector from drive %c:\n", opts->dstDrive + 'A');
  }

  /* lock drive */
  generic_block_ioctl(opts->dstDrive + 1, 0x84a, NULL);

  reset_drive(opts->dstDrive);

  /* suggestion: allow reading from a boot sector or image file here */
  if (MyAbsReadWrite(opts->dstDrive, 1, 0, oldboot, 0) != 0)
  {
    printf("%s: can't read old boot sector for drive %c:\n", pgm, opts->dstDrive + 'A');
    exit(1);
  }

#ifdef DDEBUG
  printf("Old Boot Sector:\n");
  dump_sector(oldboot);
#endif

  /* backup original boot sector when requested */
  if (opts->bsFileOrig)
  {
    printf("Backing up original boot sector to %s\n", opts->bsFileOrig);
    saveBS(opts->bsFileOrig, oldboot);
  }

  bs = (struct bootsectortype *)&oldboot;

  if (bs->bsBytesPerSec != SEC_SIZE)
  {
    printf("Sector size is not 512 but %d bytes - not currently supported!\n",
      bs->bsBytesPerSec);
    exit(1); /* Japan?! */
  }

  {
   /* see "FAT: General Overview of On-Disk Format" v1.02, 5.V.1999
    * (http://www.nondot.org/sabre/os/files/FileSystems/FatFormat.pdf)
    */
    ULONG fatSize, totalSectors, dataSectors, clusters;

    bs32 = (struct bootsectortype32 *)bs;
    rootDirSectors = (bs->bsRootDirEnts * DIRENT_SIZE  /* 32 */
                 + bs32->bsBytesPerSec - 1) / bs32->bsBytesPerSec;
    fatSize      = bs32->bsFATsecs ? bs32->bsFATsecs : bs32->bsBigFatSize;
    totalSectors = bs32->bsSectors ? bs32->bsSectors : bs32->bsHugeSectors;

    /* 1st data sector, also root dir sector for FAT12/16, for FAT32 root dir = bsRootCluster-2+rootSector */
    rootSector = bs32->bsResSectors + (bs32->bsFATs * fatSize);

    dataSectors = totalSectors - rootSector - rootDirSectors;
    clusters = dataSectors / bs32->bsSecPerClust;
 
    if (clusters < FAT_MAGIC)        /* < 4085 */
      fs = FAT12;
    else if (clusters < FAT_MAGIC16) /* < 65525 */
      fs = FAT16;
    else
      fs = FAT32;      
  }

  /* bit 0 set if function to use current BPB, clear if Device
           BIOS Parameter Block field contains new default BPB
     bit 1 set if function to use track layout fields only
           must be clear if CL=60h
     bit 2 set if all sectors in track same size (should be set) (RBIL) */
  default_bpb[0] = 4;

  if (fs == FAT32)
  {
    printf("FAT type: FAT32\n");
    /* get default bpb (but not for floppies) */
    if (opts->dstDrive >= 2 &&
        generic_block_ioctl(opts->dstDrive + 1, 0x4860, default_bpb) == 0)
      correct_bpb((struct bootsectortype *)(default_bpb + 7 - 11), bs, opts->verbose);

#ifdef WITHFAT32                /* copy one of the FAT32 boot sectors */
    if (!opts->kernel.stdbs)    /* MS/PC DOS compatible BS requested */
    {
      printf("%s: FAT32 versions of PC/MS DOS compatible boot sectors\n"
             "are not supported.\n", pgm);
      exit(1);
    }

    /* user may force explicity lba or chs, otherwise base on if LBA available */
    if ((opts->force==LBA) || ((opts->force==AUTO) && haveLBA()))
      memcpy(newboot, fat32lba, SEC_SIZE);
    else /* either auto mode & no LBA detected or forced CHS */
      memcpy(newboot, fat32chs, SEC_SIZE);
#else
    printf("SYS hasn't been compiled with FAT32 support.\n"
           "Consider using -DWITHFAT32 option.\n");
    exit(1);
#endif
  }
  else
  { /* copy the FAT12/16 CHS+LBA boot sector */
    printf("FAT type: FAT1%c\n", fs + '0' - 10);
    if (opts->dstDrive >= 2 &&
        generic_block_ioctl(opts->dstDrive + 1, 0x860, default_bpb) == 0)
      correct_bpb((struct bootsectortype *)(default_bpb + 7 - 11), bs, opts->verbose);

    if (opts->kernel.stdbs)
    {
      /* copy over appropriate boot sector, FAT12 or FAT16 */
      memcpy(newboot, (fs == FAT16) ? fat16com : fat12com, SEC_SIZE);

      /* !!! if boot sector changes then update these locations !!! */
      {
          unsigned offset;
          offset = (fs == FAT16) ? 0x175 : 0x178;
          
          if ( (newboot[offset]==0x84) && (newboot[offset+1]==0xD2) ) /* test dl,dl */
          {
            /* if always use LBA then NOP out conditional jmp over LBA logic if A: */
            if (opts->force==LBA)
            {
                offset+=2;  /* jz */
                newboot[offset] = 0x90;  /* NOP */  ++offset;
                newboot[offset] = 0x90;  /* NOP */
            }
            else if (opts->force==CHS) /* if force CHS then always skip LBA logic */
            {
                newboot[offset] = 0x30;  /* XOR */
            }
          }
          else
          {
            printf("%s : fat boot sector does not match expected layout\n", pgm);
            exit(1);
          }
      }
    }
    else
    {
#ifdef WITHOEMCOMPATBS
      printf("Using OEM (PC/MS-DOS) compatible boot sector.\n");
      memcpy(newboot, (fs == FAT16) ? oemfat16 : oemfat12, SEC_SIZE);
#else
      printf("Internal Error: no OEM compatible boot sector!\n");
#endif
    }
  }

  /* Copy disk parameter from old sector to new sector */
#ifdef WITHFAT32
  if (fs == FAT32)
    memcpy(&newboot[SBOFFSET], &oldboot[SBOFFSET], SBSIZE32);
  else
#endif
    memcpy(&newboot[SBOFFSET], &oldboot[SBOFFSET], SBSIZE);

  bs = (struct bootsectortype *)&newboot;

  /* originally OemName was "FreeDOS", changed for better compatibility */
  memcpy(bs->OemName, "FRDOS5.1", 8); /* Win9x seems to require
                                         5 uppercase letters,
                                         digit(4 or 5) dot digit */

#ifdef WITHFAT32
  if (fs == FAT32)
  {
    bs32 = (struct bootsectortype32 *)&newboot;
    /* ensure appears valid, if not then force valid */
    if ((bs32->bsBackupBoot < 1) || (bs32->bsBackupBoot > bs32->bsResSectors))
    {
      if (opts->verbose)
        printf("BPB appears to have invalid backup boot sector #, forcing to default.\n");
      bs32->bsBackupBoot = 0x6; /* ensure set, 6 is MS defined bs size */
    }
    bs32->bsDriveNumber = opts->defBootDrive;

    /* the location of the "0060" segment portion of the far pointer
       in the boot sector is just before cont: in boot*.asm.
       This happens to be offset 0x78 for FAT32 and offset 0x5c for FAT16 

       force use of value stored in bs by NOPping out mov [drive], dl
       0x82: 88h,56h,40h for fat32 chs & lba boot sectors

       i.e. BE CAREFUL WHEN YOU CHANGE THE BOOT SECTORS !!! 
    */
    if (opts->kernel.stdbs)
    {
      ((int *)newboot)[0x78/sizeof(int)] = opts->kernel.loadaddr;
      bsBiosMovOff = 0x82;
    }
    else /* compatible bs */
    {
      printf("%s: INTERNAL ERROR: how did you get here?\n", pgm);
      exit(1);
    }

#ifdef DEBUG
    printf(" FAT starts at sector %lx + %x\n",
           bs32->bsHiddenSecs, bs32->bsResSectors);
#endif
  }
  else
#endif
  {

    /* establish default BIOS drive # set in boot sector */
    bs->bsDriveNumber = opts->defBootDrive;

    /* the location of the "0060" segment portion of the far pointer
       in the boot sector is just before cont: in boot*.asm.
       This happens to be offset 0x78 for FAT32 and offset 0x5c for FAT16 
       The oem boot sectors do not have/need this value for patching.

       the location of the jmp address (patching from
       EA00007000 [jmp 0x0070:0000] to EA00207000 [jmp 0x0070:0200])
       0x11b: for fat12 oem boot sector
       0x118: for fat16 oem boot sector
       The standard boot sectors do not have/need this value patched.

       force use of value stored in bs by NOPping out mov [drive], dl
       0x66: 88h,56h,24h for fat16 and fat12 boot sectors
       0x4F: 88h,56h,24h for oem compatible fat16 and fat12 boot sectors
       
       i.e. BE CAREFUL WHEN YOU CHANGE THE BOOT SECTORS !!! 
    */
    if (opts->kernel.stdbs)
    {
      /* this sets the segment we load the kernel to, default is 0x60:0 */
      ((int *)newboot)[0x5c/sizeof(int)] = opts->kernel.loadaddr;
      bsBiosMovOff = 0x66;
    }
    else
    {
      /* load segment hard coded to 0x70 in oem compatible boot sector, */
      /* this however changes the offset jumped to default 0x70:0       */
      if (fs == FAT12)
        ((int *)newboot)[0x11c/sizeof(int)] = opts->kernel.loadaddr;
      else
        ((int *)newboot)[0x119/sizeof(int)] = opts->kernel.loadaddr;
      bsBiosMovOff = 0x4F;
    }
  }

  if (opts->ignoreBIOS)
  {
    if ( (newboot[bsBiosMovOff]==0x88) && (newboot[bsBiosMovOff+1]==0x56) )
    {
      newboot[bsBiosMovOff] = 0x90;  /* NOP */  ++bsBiosMovOff;
      newboot[bsBiosMovOff] = 0x90;  /* NOP */  ++bsBiosMovOff;
      newboot[bsBiosMovOff] = 0x90;  /* NOP */  ++bsBiosMovOff;
    }
    else
    {
      printf("%s : fat boot sector does not match expected layout\n", pgm);
      exit(1);
    }
  }

  if (opts->verbose) /* display information about filesystem */
  {
  printf("Root dir entries = %u\n", bs->bsRootDirEnts);

  printf("FAT starts at sector (%lu + %u)\n",
         bs->bsHiddenSecs, bs->bsResSectors);
  printf("Root directory starts at sector (PREVIOUS + %u * %u)\n",
         bs->bsFATsecs, bs->bsFATs);
  }
  
  setFilename(&newboot[0x1f1], opts->kernel.kernel);

  if (opts->verbose)
  {
    /* there's a zero past the kernel name in all boot sectors */
    printf("Boot sector kernel name set to %s\n", &newboot[0x1f1]);
    if (opts->kernel.stdbs)
      printf("Boot sector kernel load segment set to %X:0h\n", opts->kernel.loadaddr);
    else
      printf("Boot sector kernel jmp address set to 70:%Xh\n", opts->kernel.loadaddr);
  }

#ifdef DDEBUG
  printf("\nNew Boot Sector:\n");
  dump_sector(newboot);
#endif

  if (opts->writeBS)
  {
#ifdef DEBUG
    printf("Writing new bootsector to drive %c:\n", opts->dstDrive + 'A');
#endif

    /* write newboot to a drive */
    if (MyAbsReadWrite(opts->dstDrive, 1, 0, newboot, 1) != 0)
    {
      printf("Can't write new boot sector to drive %c:\n", opts->dstDrive + 'A');
      exit(1);
    }
    
    /* for FAT32, we need to update the backup copy as well */
    /* unless user has asked us not to, eg for better dual boot support */
    /* Note: assuming sectors 1-5 & 7-11 (FSINFO+additional boot code)
       are properly setup by prior format and need no modification
       [technically freespace, etc. should be updated]
    */
    if ((fs == FAT32) && !opts->skipBakBSCopy)
    {
      bs32 = (struct bootsectortype32 *)&newboot;
#ifdef DEBUG
      printf("writing backup bootsector to sector %d\n", bs32->bsBackupBoot);
#endif
      if (MyAbsReadWrite(opts->dstDrive, 1, bs32->bsBackupBoot, newboot, 1) != 0)
      {
        printf("Can't write backup boot sector to drive %c:\n", opts->dstDrive + 'A');
        exit(1);
      }
    }
    
#ifdef WITHOEMCOMPATBS
    /* if OEM and FAT12/16 then update root directory as well */
    if ((fs != FAT32) && !opts->kernel.stdbs)
    {
      updateRootDir(rootSector, rootDirSectors, opts->kernel.kernel, opts->kernel.dos);
    }
#endif

  } /* if write boot sector to boot record*/

  if (opts->bsFile != NULL)
  {
    if (opts->verbose)
      printf("writing new bootsector to file %s\n", opts->bsFile);

    saveBS(opts->bsFile, newboot);
  } /* if write boot sector to file*/

  reset_drive(opts->dstDrive);

  /* unlock_drive */
  generic_block_ioctl(opts->dstDrive + 1, 0x86a, NULL);
} /* put_boot */
