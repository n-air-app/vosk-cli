msbuild vosk_cli.sln /p:Configuration=Release /p:Platform=x64 /t:Build
copy x64\Release\vosk_cli.exe out/vosk_cli.exe