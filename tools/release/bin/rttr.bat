REM Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
REM
REM SPDX-License-Identifier: GPL-2.0-or-later

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
