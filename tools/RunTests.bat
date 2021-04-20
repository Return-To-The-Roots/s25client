REM Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
REM
REM SPDX-License-Identifier: GPL-2.0-or-later

echo off

set numVersions=0
if exist Debug\s25client.exe set /a numVersions=numVersions+1 & set version=1
if exist ReWithDebInfo\s25client.exe set /a numVersions=numVersions+1 & set version=2
if exist Release\s25client.exe set /a numVersions=numVersions+1 & set version=3

if %numVersions%==1 goto EXECUTE

echo "Which version shall I execute?"
echo "1 -> Debug"
echo "2 -> ReWithDebInfo (default)"
echo "3 -> Release"
set version=2
set /p version="Type number and enter"

if %version%==1 (
  set version=Debug
) else (
  if %version%==2 (
    set version=RelWithDebInfo
  ) else (
    set version=Release
  )
)

:EXECUTE

set cmd=%version%\s25client.exe --test ..\tests\maps\LuaFunctions.SWD
echo %cmd%
%cmd%
pause

