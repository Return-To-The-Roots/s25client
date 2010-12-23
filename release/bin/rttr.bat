@ECHO OFF

IF EXIST RTTR\s25update.exe GOTO UPDATE
GOTO START

:UPDATE
mkdir update
xcopy /Y RTTR\s25update.exe update
xcopy /Y RTTR\libcurl-4.dll update
xcopy /Y RTTR\zlib1.dll update

update\s25update.exe --stable -d %CD%

:START
s25client.exe

IF %ERRORLEVEL% EQU 0 GOTO END
PAUSE

:END
