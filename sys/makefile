#
# makefile for bin2c.com and sys.com
#
# $Id: makefile,v 1.1 2009-07-10 14:59:02 perditionc Exp $
#

!include "../mkfiles/generic.mak"

DOS_FILES=diskio_d.c prf.c getenv.c talloc.c

WIN_FILES=diskio_w.c

SYS_C=sys.c usage.c initopts.c fdkrncfg.c putboot.c copy.c

########################################################################

all:		bin2c.com ..\bin\sys.com

win:        bin2c.com ..\bin\sys.exe

bin2c.com:	bin2c.c $(DEPENDS)

fat12com.h:	..\boot\fat12.bin bin2c.com
		bin2c ..\boot\fat12.bin $*.h $*

fat16com.h:	..\boot\fat16.bin bin2c.com
		bin2c ..\boot\fat16.bin $*.h $*

fat32chs.h:	..\boot\fat32chs.bin bin2c.com
		bin2c ..\boot\$*.bin $*.h $*

fat32lba.h:	..\boot\fat32lba.bin bin2c.com
		bin2c ..\boot\$*.bin $*.h $*

oemfat12.h:	..\boot\oemfat12.bin bin2c.com
		bin2c ..\boot\$*.bin $*.h $*

oemfat16.h:	..\boot\oemfat16.bin bin2c.com
		bin2c ..\boot\$*.bin $*.h $*

ntfs.h:	..\boot\ntfs.bin bin2c.com
		bin2c ..\boot\$*.bin $*.h $*

..\bin\sys.com:	$(SYS_C) sys.h ..\hdr\*.h fat12com.h fat16com.h fat32chs.h fat32lba.h oemfat12.h oemfat16.h config.h
		$(CL) $(CFLAGST) $(SYS_C) $(DOS_FILES)
		copy sys.com ..\bin
		del sys.com

..\bin\sys.exe:	$(SYS_C) sys.h ..\hdr\*.h fat12com.h fat16com.h fat32chs.h fat32lba.h oemfat12.h oemfat16.h config.h
		$(CL) $(CFLAGSC) $(SYS_C) $(DOS_FILES)
		ECHO $(CL) $(CFLAGSW) $(SYS_C) $(WIN_FILES)
        ..\..\..\TOOLS\FIXHDR\fixstub sys.exe ..\bin\sysstub.exe
		$(CLW) $(CFLAGSW) -D_WIN32 $(SYS_C) $(WIN_FILES)
		copy sys.exe ..\bin
		del sys.exe

########################################################################

clean:
		-$(RM) *.bak *.cod *.crf *.err *.las *.lst *.map *.obj *.xrf

clobber:	clean
		-$(RM) bin2c.com ..\bin\sys.com fat*.h oem*.h status.me *.exe
