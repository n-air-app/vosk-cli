
//-----------------------------------------------------------------------------
// VOSKライブラリを使用した音声認識CLIアプリケーション
// 指定されたモデルを使用して16kHz、モノラルのWAVファイルまたは標準入力から音声を認識します
//-----------------------------------------------------------------------------

#include "vosk_api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>      // _setmode, _fileno, _O_BINARYに必要
#include <fcntl.h>   // _O_BINARYに必要
#include <wchar.h>
#include <locale.h>
#include <windows.h>
// VOSKライブラリをリンク
#pragma comment(lib, "libvosk.lib")

/**
 * @brief プログラムの使用方法を表示する関数
 * 
 * コマンドライン引数の詳細と使用例を標準出力に表示します。
 */
void printUsage()
{
    printf("使用方法: vosk_cli -m <モデルのパス> -f <WAVファイルのパス>\n");
    printf("オプション:\n");
    printf("  -m    音声認識モデルのパスを指定します\n");
    printf("  -f    WAVファイルのパスを指定します（\"-\"を指定すると標準入力から読み込みます）\n");
    printf("        ※WAVファイルは16kHz、モノラルである必要があります\n");
    printf("例:\n");
    printf("  vosk_cli -m model/vosk-model-ja-0.22 -f test.wav\n");
    printf("  type test.wav | vosk_cli -m model/vosk-model-ja-0.22 -f -\n");
}

/**
 * @brief コマンドライン引数を解析する関数
 * 
 * @param argc 引数の数
 * @param argv 引数の配列
 * @param modelPath モデルパスを格納するポインタ
 * @param wavFilePath WAVファイルパスを格納するポインタ
 * @return int 成功時は0、エラー時は1を返す
 */
int parseArguments(int argc, char *argv[], char **modelPath, char **wavFilePath)
{
    // 初期化
    *modelPath = NULL;
    *wavFilePath = NULL;

    // コマンドライン引数の解析（-mと-fオプションの処理）
    for (int i = 1; i < argc; i += 2)
    {
        // オプションの後に値がない場合
        if (i + 1 >= argc)
        {
            printf("エラー: オプション %s に値が指定されていません\n", argv[i]);
            printUsage();
            return 1;
        }

        // -m オプション: モデルパスの設定
        if (strcmp(argv[i], "-m") == 0)
        {
            *modelPath = argv[i + 1];
        }
        // -f オプション: WAVファイルパスの設定
        else if (strcmp(argv[i], "-f") == 0)
        {
            *wavFilePath = argv[i + 1];
        }
        // 不明なオプション
        else
        {
            printf("不明なオプション: %s\n", argv[i]);
            printUsage();
            return 1;
        }
    }

    // 必須パラメータ（モデルパスとWAVファイルパス）のチェック
    if (*modelPath == NULL || *wavFilePath == NULL)
    {
        printf("エラー: モデルパスとWAVファイルパスの両方を指定してください\n");
        printUsage();
        return 1;
    }
    
    return 0; // 成功
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
int main(int argc, char *argv[])
{
      SetConsoleOutputCP(CP_UTF8);
  
    FILE *wavin;                // WAVファイルのハンドル
    char buf[3200];             // 音声データ読み込み用バッファ
    int nread, final;           // 読み込みバイト数と最終認識フラグ
    char *modelPath = NULL;     // 音声認識モデルのパス
    char *wavFilePath = NULL;   // WAVファイルのパス

    // コマンドライン引数の解析
    if (parseArguments(argc, argv, &modelPath, &wavFilePath) != 0)
    {
        // 引数解析エラーの場合は終了
        return 1;
    }

    // 音声認識モデルの読み込み
    VoskModel *model = vosk_model_new(modelPath);
    if (model == NULL)
    {
        printf("モデルの読み込みに失敗しました: %s\n", modelPath);
        return 1;
    }    // 音声認識エンジンの初期化（サンプリングレート16kHz、モノラル音声用）
    VoskRecognizer *recognizer = vosk_recognizer_new(model, 16000.0);

    // 入力ソースの設定（ファイルまたは標準入力）
    if (strcmp(wavFilePath, "-") == 0)
    {
        // 標準入力を使用する場合
        wavin = stdin;
        _setmode(_fileno(stdin), _O_BINARY); // 標準入力をバイナリモードに設定
        
    }
    else
    {
        // ファイルを開く
        errno_t err = fopen_s(&wavin, wavFilePath, "rb");
        if (err != 0)
        {
            fprintf(stderr, "ファイルを開けませんでした: %s\n", wavFilePath);
            vosk_recognizer_free(recognizer);
            vosk_model_free(model);
            return 1; // エラーコードを返す
        }
        fseek(wavin, 44, SEEK_SET); // WAVヘッダー(44バイト)をスキップ
    }    // 音声認識処理のメインループ
    while (!feof(wavin))
    {
        // バッファにデータを読み込む
        nread = fread(buf, 1, sizeof(buf), wavin);
        
        // 音声データを認識エンジンに渡す
        final = vosk_recognizer_accept_waveform(recognizer, buf, nread);
        
        if (final)
        {
            // 最終認識結果の出力（文の区切りなど）
            printf("%s\n", vosk_recognizer_result(recognizer));
        }
        else
        {
            // 部分的な認識結果の出力（リアルタイム認識用）
      //      printf("%s\n", vosk_recognizer_partial_result(recognizer));
        }
    }
    
    // 残りのバッファデータに対する最終認識結果を出力
    printf("%s\n", vosk_recognizer_final_result(recognizer));

    // リソースの解放
    vosk_recognizer_free(recognizer);
    vosk_model_free(model);

    // 標準入力でない場合のみファイルをクローズ
    if (strcmp(wavFilePath, "-") != 0)
    {
        fclose(wavin);
    }

    return 0; // 正常終了
}