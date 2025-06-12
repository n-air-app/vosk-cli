.\out\vosk_cli.exe -m .\model\vosk-model-small-en-us-0.15\ -f .\test.wav


@REM Get-Content -Path .\test.wav -AsByteStream | .\out\vosk_cli.exe -m .\model\vosk-model-small-en-us-0.15 -f -