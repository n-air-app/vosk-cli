@echo off
echo Downloading VOSK Japanese models...
echo.

rem Create model folder if it doesn't exist
if not exist "model" (
    echo Creating model folder...
    mkdir model
)

echo Downloading small model (vosk-model-small-ja-0.22)...
set TEMP_ZIP1=vosk-model-small-ja-0.22.zip
set MODEL_URL1=https://alphacephei.com/vosk/models/vosk-model-small-ja-0.22.zip

powershell -Command "Invoke-WebRequest -Uri '%MODEL_URL1%' -OutFile '%TEMP_ZIP1%'"
powershell -Command "Expand-Archive -Path '%TEMP_ZIP1%' -DestinationPath 'model' -Force"
del "%TEMP_ZIP1%"
echo Small model download completed
echo.

echo Downloading full model (vosk-model-ja-0.22)...
echo This is a large file (about 1.5GB) and will take time...
set TEMP_ZIP2=vosk-model-ja-0.22.zip
set MODEL_URL2=https://alphacephei.com/vosk/models/vosk-model-ja-0.22.zip

powershell -Command "Invoke-WebRequest -Uri '%MODEL_URL2%' -OutFile '%TEMP_ZIP2%'"
powershell -Command "Expand-Archive -Path '%TEMP_ZIP2%' -DestinationPath 'model' -Force"
del "%TEMP_ZIP2%"
echo Full model download completed
echo.

echo All models downloaded successfully!
echo.
echo Usage:
echo   Small model: vosk-cli -m model/vosk-model-small-ja-0.22
echo   Full model:  vosk-cli -m model/vosk-model-ja-0.22
echo.
