@ECHO OFF

IF EXIST RTTR\s25update.exe GOTO UPDATE
GOTO START

:UPDATE
RTTR\s25update.exe --dir %CD%

:START
s25client.exe

IF %ERRORLEVEL% EQU 0 GOTO END
PAUSE

:END
