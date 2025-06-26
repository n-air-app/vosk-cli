msbuild stt_cli.sln /p:Configuration=Release /p:Platform=x64 /t:Build
copy x64\Release\stt_cli.exe out\stt_cli.exe
