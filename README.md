# vosk-cli アプリケーション

このプロジェクトは、音声認識エンジン [VOSK](https://alphacephei.com/vosk/) を利用したコマンドラインインターフェイス（CLI）アプリケーションです。
マイクからの音声入力をリアルタイムでテキスト化します。

VOSKの詳細や対応モデルについては、公式サイトもあわせてご参照ください。

Node.jsプロジェクトの依存ライブラリとしても利用できます。

## 機能

- マイクからの音声をリアルタイムで認識
- 日本語と英語のモデルサポート
- 利用可能なオーディオ入力デバイスのリスト表示
- テストモードでの録音とWAVファイルの保存
- JSON形式での出力
- Node.jsライブラリとしての統合
- 自動モデルダウンロード機能


## CLIとしての使い方

```
bin/vosk-cli [options]
```

### オプション

- `-l` - 利用可能な入力オーディオデバイスをJSON形式で一覧表示
- `-d index` - 使用するオーディオデバイスのインデックスを指定
- `-m path` - 音声認識モデルのパスを指定（デフォルト：model/vosk-model-small-ja-0.22）
- `-test` - 10秒間の音声を録音し、「recorded_converted.wav」としてWAVファイルに保存
- `-textonly` - 最終認識結果のみを表示（部分的な中間結果を表示しない）
- `-h` - ヘルプメッセージを表示

いずれか有効な引数を指定しない場合はヘルプを表示します。

### 例


利用可能なオーディオデバイスのリストを表示:
```
vosk-cli -l
```

オーディオデバイス インデックス=0で 実行:
```
vosk-cli -d 0
```

軽量版モデルを使用:
```
vosk-cli -m model/vosk-model-small-ja-0.22
```

通常版モデルを使用:
```
vosk-cli -m model/vosk-model-ja-0.22
```

英語のモデルを使用:
```
vosk-cli -m model/vosk-model-small-en-us-0.15
```


テストモードで実行（10秒間録音してWAVファイルを保存）:
```
vosk-cli -test
```

## nodejsライブラリとしての使い方

### NPMからのインストール

```bash
npm install github:n-air-app/vosk-cli
```

もしくは

package.jsonのdependenciesに追加:
```json
{
  "dependencies": {
    "vosk-cli": "github:n-air-app/vosk-cli"
  }
}
```

使用例:
```javascript
import Vosk from "vosk-cli";

// システム情報の取得
console.log(Vosk.getExePath());
console.log(Vosk.getVersion());
console.log(Vosk.getDevices());


// 音声認識の開始
const child = Vosk.start({
  deviceIndex: 0,
  modelPath: "./model/vosk-model-small-ja-0.22",
  onData: (data) => {
    if (data.text) {
      console.log("認識結果:", data.text);
    } else if (data.partial) {
      console.log("部分認識:", data.partial);
    }
  },
});

// 終了
setTimeout(() => {
  child.kill();
}, 30000);
```


## 必要条件

### CLIアプリケーションとして使用する場合
- Windows OS
- オーディオ入力デバイス（マイク）
- VOSKモデル（下記参照）

### Node.jsライブラリとして使用する場合
- Node.js (v14.0.0以上推奨)
- Windows OS
- オーディオ入力デバイス（マイク）
- VOSKモデル（下記参照）

## モデルのダウンロード

モデルは以下のサイトからダウンロードできます。

[https://alphacephei.com/vosk/models](https://alphacephei.com/vosk/models)


### 自動ダウンロード（推奨）
`download_model.bat`を実行すると、日本語モデルを自動的にダウンロードできます：
```
download_model.bat
```

このバッチファイルは以下のモデルをダウンロードします：
- `vosk-model-small-ja-0.22` - 軽量版（約50MB）
- `vosk-model-ja-0.22` - 通常版（約1.5GB、高精度）

### 手動ダウンロード
VOSK Modelsから直接ダウンロードして、`model`フォルダに展開することも可能です。

## ビルド方法

このプロジェクトをビルドするには、Visual Studioを使用してソリューションファイル（`vosk-cli.sln`）を開き、ビルドしてください。

## API リファレンス（Node.jsライブラリとして使用する場合）

### Vosk.getExePath()
実行ファイルのパスを取得します。

```javascript
const exePath = Vosk.getExePath();
console.log(exePath); // "C:\\path\\to\\vosk-cli.exe"
```

### Vosk.getVersion()
バージョン情報を取得します。

```javascript
const version = Vosk.getVersion();
console.log(version); // バージョン文字列
```

### Vosk.getDevices()
利用可能なオーディオデバイスの一覧を取得します。

```javascript
const devices = Vosk.getDevices();
console.log(devices); // デバイス情報のJSON配列
```

### Vosk.start(options)
音声認識を開始します。

```javascript
const child = Vosk.start({
  deviceIndex: 0,                    // オーディオデバイスのインデックス
  modelPath: "./model/vosk-model-small-ja-0.22", // モデルのパス
  onData: (data) => {               // データ受信時のコールバック
    console.log(data);
  },
});

// 終了時
child.kill();
```

#### オプション

- `deviceIndex` (number): 使用するオーディオデバイスのインデックス
- `modelPath` (string): 音声認識モデルのパス
- `onData` (function): データ受信時のコールバック関数

#### データフォーマット

コールバック関数には以下の形式のオブジェクトが渡されます：

```javascript
{
  text: "最終的な認識結果",      // 確定した認識結果
  partial: "部分的な認識結果",   // 認識途中の結果
  error: "エラーメッセージ",     // エラーが発生した場合
  info: "情報メッセージ"        // その他の情報
}
```

## サンプルコード

完全なサンプルコードは `example` フォルダに含まれています。詳細は [example/readme.md](example/readme.md) を参照してください。



## 依存DLLについて

`bin` ディレクトリ内の以下の DLL ファイルは、[vosk-api v0.3.45 リリース](https://github.com/alphacep/vosk-api/releases/tag/v0.3.45) から取得しています。

各DLLの詳細やライセンスについては、上記リリースページをご参照ください。

- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libvosk.dll`
- `libwinpthread-1.dll`



## トラブルシューティング

### Node.jsライブラリとして使用する場合

**モジュールが見つからないエラー**
```bash
npm install
```
を実行して依存関係を再インストールしてください。

**音声認識が開始されない**
- `Vosk.getDevices()` でデバイス一覧を確認し、正しいインデックスを指定してください

**TypeScriptエラー**
型定義ファイルが正しくインポートされているか確認してください：
```typescript
import Vosk from "vosk-cli";
```
