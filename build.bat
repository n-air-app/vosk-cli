@echo off
msbuild vosk-cli.sln /p:Configuration=Release /p:Platform=x64 /t:Build
if errorlevel 1 exit /b %errorlevel%

if not exist bin mkdir bin
if errorlevel 1 exit /b %errorlevel%

copy /Y x64\Release\vosk-cli.exe bin\vosk-cli.exe
if errorlevel 1 exit /b %errorlevel%
