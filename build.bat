msbuild vosk-cli.sln /p:Configuration=Release /p:Platform=x64 /t:Build
mkdir bin
copy x64\Release\vosk-cli.exe bin\vosk-cli.exe
