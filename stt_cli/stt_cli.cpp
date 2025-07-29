//-----------------------------------------------------------------------------
// VOSKライブラリを使用した音声認識CLIアプリケーション
// 指定されたモデルを使用してwindows音声入力から認識します
//-----------------------------------------------------------------------------

// バージョン情報
#define STT_CLI_VERSION "1.0.0"
#define STT_CLI_BUILD_DATE __DATE__

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <atlbase.h>
//--
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
//--
#include <codecvt>
#include <locale>
#include <vector>
#include <string>
#include <regex>
//--
#include "vosk_api.h"

// VOSKライブラリ
#pragma comment(lib, "libvosk.lib")

/**
 * @brief JSON形式でエラーメッセージを出力する関数
 *
 * @param message 出力するエラーメッセージ
 */
void outputJsonError(const std::string &message) {
  printf("{\"error\":\"%s\"}\n", message.c_str());
  fflush(stdout);
}

/**
 * @brief オーディオデバイスの情報を保持する構造体
 */
struct AudioDeviceInfo {
  std::wstring id;    // デバイスID
  std::wstring name;  // デバイス名
};

/**
 * @brief 利用可能な入力オーディオデバイスを列挙する関数
 *
 * @return std::vector<AudioDeviceInfo> 利用可能なオーディオデバイスの一覧
 */
std::vector<AudioDeviceInfo> EnumerateInputDevices() {
  std::vector<AudioDeviceInfo> devices;
  CComPtr<IMMDeviceEnumerator> enumerator;
  CComPtr<IMMDeviceCollection> collection;

  CoInitialize(nullptr);
  HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
  if (FAILED(hr)) return devices;

  hr = enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE,
                                      &collection);
  if (FAILED(hr)) {
    outputJsonError("EnumAudioEndpoints failed: " + std::to_string(hr));
    return devices;
  }

  UINT count = 0;
  collection->GetCount(&count);
  for (UINT i = 0; i < count; ++i) {
    CComPtr<IMMDevice> device;
    collection->Item(i, &device);

    LPWSTR deviceId;
    device->GetId(&deviceId);

    CComPtr<IPropertyStore> props;
    device->OpenPropertyStore(STGM_READ, &props);
    PROPVARIANT varName;
    PropVariantInit(&varName);
    props->GetValue(PKEY_Device_FriendlyName, &varName);

    if (varName.vt != VT_LPWSTR) {
      PropVariantClear(&varName);
      CoTaskMemFree(deviceId);
      continue;  // 名前が取得できない場合はスキップ
    }
    devices.push_back({deviceId, varName.pwszVal});

    CoTaskMemFree(deviceId);
    PropVariantClear(&varName);
  }

  return devices;
}

/**
 * @brief オーディオデバイスの一覧をJSON形式で出力する関数
 *
 * @param devices 出力するデバイス情報の配列
 */
void OutputDevicesAsJson() {
  std::vector<AudioDeviceInfo> devices = EnumerateInputDevices();
  std::string json = "{\"devices\":[";

  for (size_t i = 0; i < devices.size(); ++i) {
    // ワイド文字列をUTF-8に変換
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string id = converter.to_bytes(devices[i].id);
    std::string name = converter.to_bytes(devices[i].name);

    json += "{";
    json += "\"index\":" + std::to_string(i) + ",";
    json += "\"id\":\"" + id + "\",";
    json += "\"name\":\"" + name + "\"";
    json += "}";

    if (i < devices.size() - 1) json += ",";
  }

  json += "],\"version\":\"" + std::string(STT_CLI_VERSION) + "\"}";
  puts(json.c_str());
}

/**
 * @brief 文字列からすべてのスペースを削除する関数
 *
 * @param input 処理する文字列
 * @return std::string スペースが削除された文字列
 */
std::string RemoveSpaces(const char *input) {
  if (!input) return "";
  auto a = std::string(input);
  std::regex space_pattern("\\s+");

  // スペースを空文字列に置換
  return std::regex_replace(a, space_pattern, "");
}

/**
 * @brief オーディオバッファを16kHzモノラルに変換する関数
 *
 * @param buffer 変換するオーディオバッファ
 * @param numFrames フレーム数
 * @param sampleRate 元のサンプリングレート
 * @param channels チャンネル数
 * @param bitsPerSample ビット深度
 * @return std::vector<short> 変換後の16kHzモノラルPCMデータ
 */
std::vector<short> ConvertBufferToMono16k(const BYTE *buffer, UINT32 numFrames,
                                          int sampleRate, int channels,
                                          int bitsPerSample) {
  // 出力バッファ
  std::vector<short> result;

  // 入力バッファが空なら空のベクトルを返す
  if (buffer == nullptr || numFrames == 0) {
    return result;
  }

  // IEEE浮動小数点形式かどうかを判定
  bool isFloat = (bitsPerSample == 32);  // 通常、32ビットはfloat

  // リサンプリング比率の計算
  double ratio = 16000.0 / sampleRate;
  int bytesPerSample = bitsPerSample / 8;
  int bytesPerFrame = channels * bytesPerSample;

  // 出力サイズの見積もり
  size_t outputSamples = static_cast<size_t>(numFrames * ratio);
  result.reserve(outputSamples);

  // 各出力サンプルにつき、最も近い入力サンプルを計算する
  for (size_t outSample = 0; outSample < outputSamples; outSample++) {
    // 入力サンプル位置を計算
    double inSamplePos = outSample / ratio;
    size_t inSample = static_cast<size_t>(inSamplePos);

    // バウンダリチェック
    if (inSample >= numFrames) {
      inSample = numFrames - 1;
    }

    // すべてのチャネルの平均を取る
    int sum = 0;
    float sumFloat = 0.0f;

    for (int ch = 0; ch < channels; ch++) {
      // サンプル位置を計算
      size_t pos = inSample * bytesPerFrame + ch * bytesPerSample;

      if (isFloat) {
        // IEEE浮動小数点形式の場合
        float value = 0.0f;
        // 正しいバイトオーダーでfloat値を構築
        memcpy(&value, buffer + pos, sizeof(float));
        sumFloat += value;
      } else {
        // PCM整数形式の場合
        int value = 0;
        switch (bitsPerSample) {
          case 8:
            // 8ビットPCMは通常0-255の範囲（符号なし）
            value = buffer[pos] - 128;
            break;
          case 16:
            // 16ビットPCMは-32768〜32767の範囲（符号あり）
            value = static_cast<short>(buffer[pos] | (buffer[pos + 1] << 8));
            break;
          case 24:
            // 24ビットPCM
            value =
                buffer[pos] | (buffer[pos + 1] << 8) | (buffer[pos + 2] << 16);
            // 符号ビット拡張
            if (value & 0x800000) {
              value |= 0xFF000000;
            }
            break;
          case 32:
            // 32ビット整数PCM（非浮動小数点）
            value = buffer[pos] | (buffer[pos + 1] << 8) |
                    (buffer[pos + 2] << 16) | (buffer[pos + 3] << 24);
            // 16ビットにスケールダウン
            value >>= 16;
            break;
        }
        sum += value;
      }
    }

    // 平均値を計算（モノラル化）
    short monoSample;
    if (isFloat) {
      float avg = sumFloat / channels;
      float clipped = avg * 32767.0f;
      if (clipped > 32767.0f) clipped = 32767.0f;
      if (clipped < -32768.0f) clipped = -32768.0f;
      monoSample = static_cast<short>(clipped);
    } else {
      monoSample = static_cast<short>(sum / channels);
    }

    // 変換したサンプルを結果に追加
    result.push_back(monoSample);
  }

  return result;
}

/**
 * @brief オーディオデバイスのフォーマット情報をJSON形式で出力する関数(確認用)
 *
 * @param deviceFormat 表示するWAVEFORMATEX構造体へのポインタ
 */
void PrintDeviceFormat(WAVEFORMATEX *deviceFormat) {
  if (!deviceFormat) return;

  printf("{\n");
  printf("  \"format\": {\n");
  printf("    \"formatTag\": \"0x%04X\",\n", deviceFormat->wFormatTag);
  printf("    \"sampleRate\": %u,\n", deviceFormat->nSamplesPerSec);
  printf("    \"channels\": %u,\n", deviceFormat->nChannels);
  printf("    \"bitDepth\": %u,\n", deviceFormat->wBitsPerSample);
  printf("    \"bytesPerFrame\": %u,\n", deviceFormat->nBlockAlign);
  printf("    \"bytesPerSecond\": %u,\n", deviceFormat->nAvgBytesPerSec);
  printf("    \"extraSize\": %u", deviceFormat->cbSize);

  if (deviceFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
      deviceFormat->cbSize >= 22) {
    WAVEFORMATEXTENSIBLE *wfext =
        reinterpret_cast<WAVEFORMATEXTENSIBLE *>(deviceFormat);
    printf(",\n    \"extensible\": {\n");
    printf("      \"validBitsPerSample\": %u,\n",
           wfext->Samples.wValidBitsPerSample);
    printf("      \"channelMask\": \"0x%08X\",\n", wfext->dwChannelMask);

    // GUID文字列形式に変換
    char guidString[100];
    sprintf_s(guidString, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
              wfext->SubFormat.Data1, wfext->SubFormat.Data2,
              wfext->SubFormat.Data3, wfext->SubFormat.Data4[0],
              wfext->SubFormat.Data4[1], wfext->SubFormat.Data4[2],
              wfext->SubFormat.Data4[3], wfext->SubFormat.Data4[4],
              wfext->SubFormat.Data4[5], wfext->SubFormat.Data4[6],
              wfext->SubFormat.Data4[7]);

    printf("      \"subFormat\": \"%s\",\n", guidString);

    // サブフォーマットの種類
    if (wfext->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
      printf("      \"subFormatType\": \"PCM\"\n");
    else if (wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
      printf("      \"subFormatType\": \"IEEE FLOAT\"\n");
    else
      printf("      \"subFormatType\": \"UNKNOWN\"\n");

    printf("    }\n");
  } else {
    printf("\n");
  }

  printf("  }\n");
  printf("}\n");
  fflush(stdout);
}

/**
 * @brief PCMデータをWAVファイルとして保存する関数(テスト用)
 *
 * @param samples 保存するPCMサンプルデータ（16ビット）
 * @param filename 保存するファイル名
 * @param sampleRate サンプリングレート（デフォルト: 16000Hz）
 * @param numChannels チャンネル数（デフォルト: 1=モノラル）
 * @return bool 保存成功時はtrue、失敗時はfalse
 */
bool SaveAsWav(const std::vector<short> &samples, const char *filename,
               int sampleRate = 16000, int numChannels = 1) {
  struct WavHeader {
    char riff[4];            // "RIFF"
    uint32_t chunkSize;      // ファイルサイズ - 8
    char wave[4];            // "WAVE"
    char fmt[4];             // "fmt "
    uint32_t fmtSize;        // フォーマットチャンクのサイズ（通常16）
    uint16_t audioFormat;    // フォーマットタイプ（1 = PCM）
    uint16_t numChannels;    // チャンネル数
    uint32_t sampleRate;     // サンプリングレート
    uint32_t byteRate;       // サンプルレート * ブロックサイズ
    uint16_t blockAlign;     // ブロックサイズ（チャンネル数 * ビット深度 / 8）
    uint16_t bitsPerSample;  // ビット深度
    char data[4];            // "data"
    uint32_t dataSize;       // オーディオデータのサイズ
  };

  FILE *fp = nullptr;
  errno_t err = fopen_s(&fp, filename, "wb");
  if (err != 0 || fp == nullptr) {
    fprintf(stderr, "WAVファイルを開けませんでした: %s\n", filename);
    return false;
  }

  // データサイズ計算
  uint32_t dataSize = static_cast<uint32_t>(samples.size() * sizeof(short));

  // WAVヘッダー準備
  WavHeader header;
  memcpy(header.riff, "RIFF", 4);
  header.chunkSize = dataSize + sizeof(WavHeader) - 8;
  memcpy(header.wave, "WAVE", 4);
  memcpy(header.fmt, "fmt ", 4);
  header.fmtSize = 16;
  header.audioFormat = 1;  // PCM
  header.numChannels = static_cast<uint16_t>(numChannels);
  header.sampleRate = static_cast<uint32_t>(sampleRate);
  header.bitsPerSample = 16;  // 16-bit
  header.blockAlign =
      static_cast<uint16_t>(header.numChannels * header.bitsPerSample / 8);
  header.byteRate = header.sampleRate * header.blockAlign;
  memcpy(header.data, "data", 4);
  header.dataSize = dataSize;

  // ヘッダーの書き込み
  fwrite(&header, sizeof(WavHeader), 1, fp);
  // データの書き込み
  fwrite(samples.data(), sizeof(short), samples.size(), fp);

  fclose(fp);
  return true;
}
/**
 * @brief リソースを管理するクラス
 *
 * スコープを抜けるときに自動的にリソースを解放するRAIIパターンを実装
 */
class ResourceGuard {
 public:
  ResourceGuard(VoskRecognizer *r = nullptr, VoskModel *m = nullptr,
                WAVEFORMATEX *f = nullptr)
      : recognizer(r), model(m), deviceFormat(f) {}

  ~ResourceGuard() { cleanup(); }

  void setRecognizer(VoskRecognizer *r) { recognizer = r; }
  void setModel(VoskModel *m) { model = m; }
  void setDeviceFormat(WAVEFORMATEX *f) { deviceFormat = f; }

  // リソースを解放し、nullptrにリセット
  void cleanup() {
    if (recognizer) {
      vosk_recognizer_free(recognizer);
      recognizer = nullptr;
    }
    if (model) {
      vosk_model_free(model);
      model = nullptr;
    }
    if (deviceFormat) {
      CoTaskMemFree(deviceFormat);
      deviceFormat = nullptr;
    }
  }

  // 所有権を放棄（解放せずにnullptrにする）
  void release() {
    recognizer = nullptr;
    model = nullptr;
    deviceFormat = nullptr;
  }

 private:
  VoskRecognizer *recognizer;
  VoskModel *model;
  WAVEFORMATEX *deviceFormat;
};

/**
 * @brief マイクからのオーディオストリームを開始し音声認識を実行する関数
 *
 * @param deviceIndex 使用するオーディオデバイスのインデックス
 * @param modelPath 音声認識モデルのパス
 * @param isTest
 * テストモードフラグ（trueの場合、10秒間録音してWAVファイルを保存）
 */
void StartAudioStream(int deviceIndex, const char *modelPath, bool isTest,
                      bool textOnly) {
  ResourceGuard resources;  // スコープを抜ける際に自動的にリソースを解放

  // VOSKモデルのロード
  vosk_set_log_level(-1);
  VoskModel *model = vosk_model_new(modelPath);
  if (model == nullptr) {
    outputJsonError("Failed to load model: " + std::string(modelPath));
    return;
  }
  resources.setModel(model);

  // 認識器の作成（16kHzサンプルレート用）
  VoskRecognizer *recognizer = vosk_recognizer_new(model, 16000.0);
  resources.setRecognizer(recognizer);

  // リサンプル用バッファの事前確保
  std::vector<short> convertedData;
  convertedData.reserve(16000);  // 1秒分のバッファを事前確保

  // 前回の部分認識結果を保持する変数
  std::string lastPartialStr;

  CoInitialize(nullptr);  // COMを初期化

  CComPtr<IMMDeviceEnumerator> enumerator;
  CComPtr<IMMDevice> device;
  HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
  if (FAILED(hr)) {
    outputJsonError("MMDeviceEnumerator creation failed: " +
                    std::to_string(hr));
    return;
  }

  CComPtr<IMMDeviceCollection> collection;
  // EnumAudioEndpoints エラー処理の修正
  hr = enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE,
                                      &collection);
  if (FAILED(hr)) {
    outputJsonError("EnumAudioEndpoints failed: " + std::to_string(hr));
    return;
  }

  hr = collection->Item(deviceIndex, &device);
  if (FAILED(hr)) {
    outputJsonError("Failed to get device: " + std::to_string(hr));
    return;
  }

  CComPtr<IAudioClient> audioClient;
  hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                        (void **)&audioClient);
  if (FAILED(hr)) {
    outputJsonError("Failed to create IAudioClient: " + std::to_string(hr));
    return;
  }

  WAVEFORMATEX *deviceFormat;
  hr = audioClient->GetMixFormat(&deviceFormat);
  if (FAILED(hr)) {
    outputJsonError("GetMixFormat failed: " + std::to_string(hr));
    return;
  }
  resources.setDeviceFormat(deviceFormat);  // 自動解放の対象に追加

  // フォーマット情報を表示
  // PrintDeviceFormat(deviceFormat); 

  int sample_rate = deviceFormat->nSamplesPerSec;
  int channels = deviceFormat->nChannels;
  int bits_per_sample = deviceFormat->wBitsPerSample;  // 初期化パラメータを設定
  REFERENCE_TIME hnsRequestedDuration = 10000000;      // 1秒

  hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0,
                               hnsRequestedDuration, 0, deviceFormat, nullptr);
  if (FAILED(hr)) {
    outputJsonError("Initialize failed: " + std::to_string(hr));
    return;
  }

  // キャプチャクライアントの取得
  CComPtr<IAudioCaptureClient> captureClient;
  hr = audioClient->GetService(__uuidof(IAudioCaptureClient),
                               (void **)&captureClient);
  if (FAILED(hr)) {
    outputJsonError("GetService failed: " + std::to_string(hr));
    return;
  }

  // 録音処理
  hr = audioClient->Start();
  if (FAILED(hr)) {
    outputJsonError("Start failed: " + std::to_string(hr));
    return;
  }

  DWORD startTime = GetTickCount();
  DWORD endTime = startTime + (10 * 1000);
  std::vector<short> convertedBuffer;

  puts("{\"info\":\"start\"}");
  fflush(stdout);

  while (!isTest || GetTickCount() < endTime) {
    UINT32 packetLength = 0;
    hr = captureClient->GetNextPacketSize(&packetLength);
    if (FAILED(hr)) {
      outputJsonError("GetNextPacketSize failed: " + std::to_string(hr));
      break;
    }

    if (packetLength == 0) {
      Sleep(10);  // パケットがない場合は少し待つ
      continue;
    }

    BYTE *data;
    UINT32 numFrames;
    DWORD flags;
    hr = captureClient->GetBuffer(&data, &numFrames, &flags, nullptr, nullptr);
    if (FAILED(hr)) {
      outputJsonError("GetBuffer failed: " + std::to_string(hr));
      break;
    }

    // サイレンスでない場合のみ処理
    if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
      // このパケットのデータを16kHzモノラルに変換
      convertedData = ConvertBufferToMono16k(data, numFrames, sample_rate,
                                             channels, bits_per_sample);

      if (isTest)
        convertedBuffer.insert(convertedBuffer.end(), convertedData.begin(),
                               convertedData.end());

      // VOSKに渡す
      if (!convertedData.empty()) {
        bool isFinal = vosk_recognizer_accept_waveform(
            recognizer, reinterpret_cast<const char *>(convertedData.data()),
            static_cast<int>(convertedData.size() * sizeof(short)));

        if (isFinal) {
          // 文の区切りで結果を表示
          const char *result = vosk_recognizer_result(recognizer);
          std::string resultStr = RemoveSpaces(result);
          if (!resultStr.empty() && resultStr != "{\"text\":\"\"}") {
            puts(resultStr.c_str());
            fflush(stdout);
          }

          // 最終結果が出力されたら部分認識結果をリセット
          lastPartialStr.clear();
        } else if (!textOnly) {
          const char *partial = vosk_recognizer_partial_result(recognizer);
          // 部分認識結果を取得
          std::string partialStr = RemoveSpaces(partial);

          // 空または前回と同じ結果は出力しない
          if (!partialStr.empty() && partialStr != "{\"partial\":\"\"}" && partialStr != lastPartialStr) {
            puts(partialStr.c_str());
            fflush(stdout);

            lastPartialStr = partialStr;  // 最後の部分結果を更新
          }
        }
      }
    }
    hr = captureClient->ReleaseBuffer(numFrames);
    if (FAILED(hr)) {
      outputJsonError("ReleaseBuffer failed: " + std::to_string(hr));
      break;
    }
  }

  audioClient->Stop();
  // 最終結果を取得
  const char *finalResult = vosk_recognizer_final_result(recognizer);
  std::string finalResultStr = RemoveSpaces(finalResult);
  puts(finalResultStr.c_str());

  if (isTest) SaveAsWav(convertedBuffer, "recorded_converted.wav", 16000, 1);
  // リソースは自動的に解放される（ResourceGuardのデストラクタで）
}

/**
 * @brief プログラムの使用方法を表示する関数
 *
 * コマンドライン引数の詳細と使用例を標準出力に表示します。
 */
void printUsage() {
  printf("STT CLI - Speech To Text Command Line Interface\n");
  printf("Version: %s (Built: %s)\n", STT_CLI_VERSION, STT_CLI_BUILD_DATE);
  printf("\n");
  printf("Usage: vosk_cli [options]\n");
  printf("Options:\n");
  printf("  -l          List input audio devices in JSON format\n");
  printf("  -d index    Specify the audio device index (default: 0)\n");
  printf("  -m path     Specify the path to the speech recognition model\n");
  printf("              (default: model/vosk-model-small-ja-0.22)\n");
  printf("  -test       Test record 10sec and output wav\n");
  printf(
      "  -textonly   Show only final recognition results (no partial "
      "results)\n");
  printf("  -h          Show this help message\n");
}

/**
 * @brief コマンドライン引数を解析する関数
 *
 * @param argc 引数の数
 * @param argv 引数の配列
 * @param modelPath モデルパスを格納するポインタ
 * @param listDevices デバイス一覧表示フラグを格納するポインタ
 * @param deviceIndex オーディオデバイスのインデックスを格納するポインタ
 * @param isTest テストモードフラグを格納するポインタ
 * @param textOnly テキストのみモードフラグを格納するポインタ
 * @return int 成功時は0、エラー時は1を返す
 */
int parseArguments(int argc, char *argv[], char **modelPath, bool *listDevices,
                   int *deviceIndex, bool *isTest, bool *textOnly) {  // 初期化

  if (argc <= 1) return 1;

  for (int i = 1; i < argc; i++) {
    // -l オプション: デバイス一覧表示
    if (!strcmp(argv[i], "-l")) {
      *listDevices = true;
      continue;
    }

    // -h オプション: ヘルプ表示
    if (!strcmp(argv[i], "-h")) {
      return 1;
    }

    // -test オプション: テストモード有効化
    if (!strcmp(argv[i], "-test")) {
      *isTest = true;
      continue;
    }

    // -textonly オプション: テキストのみモード有効化
    if (!strcmp(argv[i], "-textonly")) {
      *textOnly = true;
      continue;
    }

    // -d オプション: デバイスインデックスの設定
    if (!strcmp(argv[i], "-d")) {
      if (i + 1 >= argc) {
        outputJsonError("No value specified for option " +
                        std::string(argv[i]));
        return 1;
      }

      // 数値変換
      try {
        *deviceIndex = std::stoi(argv[i + 1]);
      } catch (const std::exception &) {
        outputJsonError("Invalid device index: " + std::string(argv[i + 1]));
        return 1;
      }
      i++;
      continue;
    }
    // -m オプション: モデルパスの設定
    if (!strcmp(argv[i], "-m")) {
      if (i + 1 >= argc) {
        outputJsonError("No value specified for option " +
                        std::string(argv[i]));
        return 1;
      }

      *modelPath = argv[i + 1];
      i++;
      continue;
    }
    // 不明なオプション
    else {
      outputJsonError("Unknown option: " + std::string(argv[i]));
      return 1;
    }
  }

  return 0;  // 成功
}

/**
 * @brief メイン関数
 *
 * コマンドライン引数を解析し、指定されたモデルとWAVファイルを使用して音声認識を実行します。
 *
 * @param argc 引数の数
 * @param argv 引数の配列
 * @return int プログラムの終了コード（成功時は0、エラー時は1）
 */
int main(int argc, char *argv[]) {
  // コンソールをUTF-8に設定（VOSKの出力と一致させる）
  SetConsoleOutputCP(65001);  // UTF-8のコードページ
  SetConsoleCP(65001);

  // UTF-8ロケールを明示的に指定
  setlocale(LC_ALL, ".UTF8");

  char *modelPath =
      _strdup("model/vosk-model-small-ja-0.22");  // 音声認識モデルのパス
  bool listDevices = false;                       // デバイス一覧表示フラグ
  int deviceIndex = 0;    // オーディオデバイスのインデックス
  bool isTest = false;    // テストモードフラグ
  bool textOnly = false;  // テキストのみフラグ（部分結果を表示しない）

  // 引数の解析
  if (parseArguments(argc, argv, &modelPath, &listDevices, &deviceIndex,
                     &isTest, &textOnly) != 0) {
    printUsage();
    return 1;
  }

  // listDevicesがtrueの場合はデバイス一覧をJSON形式で出力して終了
  if (listDevices) {
    OutputDevicesAsJson();
    return 0;
  }

  // モデルパスとデバイスインデックスを指定して音声ストリームを開始
  StartAudioStream(deviceIndex, modelPath, isTest, textOnly);

  return 0;
}
