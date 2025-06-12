sox -t waveaudio -d -r 16000 -c 1 -b 16 -e signed-integer -t wav - | .\out\vosk_cli.exe -m .\model\vosk-model-ja-0.22\ -f -
