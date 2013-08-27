@echo off

for /d /r %%a in (*vs2010) do rmdir /s /q "%%a"

for /d /r %%a in (*vs2012) do mkdir "%%a/../vs2010"

for /R %%a in (*.filters) do (
  echo %%a
  if exist %%~dpa..\vs2010 (
	copy %%a %%~dpa..\vs2010\%%~nxa
  )
)

for /R %%a in (*.vcxproj) do (
  echo %%a
  if exist %%~dpa..\vs2010 (
  	type "%%a" | findstr /v /C:"<PlatformToolset>v110" > %%~dpa..\vs2010\%%~nxa
	if exist %%~dpa%%~na.sln (
rem		copy %%~dpa%%~na.sln %%~dpa..\vs2010\%%~na.sln
		replace.bat "2012" "2010" %%~dpa%%~na.sln > %%~dpa%%~na.sln2
		replace.bat "Format Version 12.00" "Format Version 11.00" %%~dpa%%~na.sln2 > %%~dpa..\vs2010\%%~na.sln
		del %%~dpa%%~na.sln2
	)
  )
)
