msbuild stt_cli.sln /p:Configuration=Release /p:Platform=x64 /t:Build
mkdir bin
copy x64\Release\stt_cli.exe bin\stt_cli.exe
