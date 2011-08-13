@ECHO OFF
ECHO cleaning...

:BOOTSECT
CD BOOT
wmake -h -ms clobber
CD ..

:SYS
ECHO building sys program
CD SYS
wmake -h -ms clobber
CD ..

ECHO done
