/***************************************************************

                                    diskio_w.c
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
#include <winioctl.h>

void ShowErrorMsg(void)
{
LPVOID lpMsgBuf;
FormatMessage( 
    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
    FORMAT_MESSAGE_FROM_SYSTEM | 
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    (LPTSTR) &lpMsgBuf,
    0,
    NULL 
);
// Process any inserts in lpMsgBuf.
// ...
// Display the string.
MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
// Free the buffer.
LocalFree( lpMsgBuf );
}

/* NT 5+ only, floppies only */
/* See http://www.codeguru.com/system/ReadSector.html by Sreejith S
   to add Win9x support 
*/
int MyAbsReadWrite(int DosDrive, int count, ULONG sector, void *buffer, int write)
//int MyAbsReadWrite(int DosDrive, UWORD count, ULONG sector, char *buffer, int write)
{
  char devName[] = "\\\\.\\X:";
  HANDLE hFloppy;
  ULONG bytesTransferred;

  /* get handle to floppy */
  devName[4] = 'A' + DosDrive;
  if ((hFloppy = CreateFile(devName, write?GENERIC_WRITE:GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
       NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE) return 0xFF;
    
  /* seek to desired sector */
  SetFilePointer(hFloppy, sector*SEC_SIZE, NULL, FILE_BEGIN);

  /* actually perform the read or write */
  if (write)
    WriteFile(hFloppy, buffer, count*SEC_SIZE, &bytesTransferred, NULL);
  else
    ReadFile(hFloppy, buffer, count*SEC_SIZE, &bytesTransferred, NULL);

  /* free handle */
  CloseHandle(hFloppy);

  if (bytesTransferred != (ULONG)(count*SEC_SIZE))
  {
    printf("Not all bytes read/written, transferred %Lu bytes\n", bytesTransferred);
    ShowErrorMsg();
  }

  /* return success or failure depending on if all data transferred or not */
  if (bytesTransferred == (ULONG)(count*SEC_SIZE))
    return 0;
  else
    return 0xFF;
}


void reset_drive(int DosDrive) {}

int generic_block_ioctl(unsigned drive, unsigned cx, unsigned char *par);

/*
 * If BIOS has got LBA extensions, after the Int 13h call BX will be 0xAA55.
 * If extended disk access functions are supported, bit 0 of CX will be set.
 */
BOOL haveLBA(void)     /* return TRUE if we have LBA BIOS, FALSE otherwise */
{
  return TRUE;
}

void lockDrive(unsigned drive) {}
void unLockDrive(unsigned drive) {}

/* returns default BPB (and other device parameters) */
int getDeviceParms(unsigned drive, FileSystem fs, unsigned char *buffer)
{
  struct {
    UBYTE filler1[11];
    UWORD bsBytesPerSec;
    UBYTE filler2[11];
    UWORD bsSecPerTrack;
    UWORD bsHeads;
    ULONG bsHiddenSecs;
  } fakeBPB = { 0 };
  HANDLE hnd;
  DWORD result;
  DISK_GEOMETRY dskGeom;
  PARTITION_INFORMATION partInfo;
  
  char *drivename = "A:\\";
  drivename[0] = 'A' + drive;
  hnd = CreateFileA(drivename, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
  if (hnd == INVALID_HANDLE_VALUE) 
  {
    printf("Unable to get handle to drive %s (%i)\n", drivename, GetLastError());
    return -1;
  }

  result = DeviceIoControl(hnd, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &dskGeom, sizeof(dskGeom), &result/*unused*/, NULL);
  if (result)
    result = DeviceIoControl(hnd, IOCTL_DISK_GET_PARTITION_INFO, NULL, 0, &partInfo, sizeof(partInfo), &result/*unused*/, NULL);
  CloseHandle(hnd);
  
  if (result)
  {
	/* truncation of types ok, values should not exceed max values of BPB types */
    fakeBPB.bsBytesPerSec = (UWORD)dskGeom.BytesPerSector;
    fakeBPB.bsSecPerTrack = (UWORD)dskGeom.SectorsPerTrack;
    fakeBPB.bsHeads = (UWORD)dskGeom.TracksPerCylinder;
    fakeBPB.bsHiddenSecs = partInfo.HiddenSectors;
    return 0;
  }

  printf("Error obtaining drive %s geometry\n", drivename);
  return -1;    
}
