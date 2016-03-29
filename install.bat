@echo off
echo About to copy libraries to /Documents/Arduino/libraries.
setlocal
:PROMPT
SET /P AREYOUSURE=Are you sure (Y/[N])?
IF /I "%AREYOUSURE%" NEQ "Y" GOTO END
	echo on
	xcopy /s lib\* %USERPROFILE%\Documents\Arduino\libraries\
	@echo off
	echo Copying done.
:END
@echo off
endlocal
echo Exiting.
PAUSE
