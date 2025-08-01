import SpeechToText from "stt_cli";

const models = {
  small: {
    base: "vosk-model-small-ja-0.22",
    url: "https://alphacephei.com/vosk/models/vosk-model-small-ja-0.22.zip",
  },
  large: {
    base: "vosk-model-ja-0.22",
    url: "https://alphacephei.com/vosk/models/vosk-model-ja-0.22.zip",
  },
};

async function main() {
  console.log(SpeechToText.getExePath());
  console.log(SpeechToText.getVersion());
  console.log(SpeechToText.getDevices());

  // URLおよびmodelPathはコール元で管理
  const url = models.small.url;
  const modelPath = `./temp/model/${models.small.base}`;

  // モデルの存在確認
  const exist = SpeechToText.isExistModel(modelPath);
  console.log(`モデルの存在確認: ${modelPath} - ${exist}`);

  // モデルのダウンロード(存在すればスキップ)
  console.log(`モデルのダウンロード: ${url}`);
  await SpeechToText.downloadModel(url, modelPath, "./temp/temp");
  console.log("ダウンロード完了:", modelPath);

  // 音声認識のテスト
  console.log("\n音声認識を開始します...");
  const child = SpeechToText.start({
    deviceIndex: 0,
    modelPath,
    onData: (data) => {
      console.log("受信:", JSON.stringify(data));

      // 特定の出力に応じて処理
      if (data.info === "start") {
        console.log("音声認識が開始されました");
      } else if (data.text) {
        console.log("認識結果:", data.text);
      } else if (data.partial) {
        console.log("部分認識:", data.partial);
      } else if (data.error) {
        console.log("エラー:", data.error);
      }
    },
  });

  setTimeout(() => {
    console.log("\n音声認識を終了します...");
    child.kill();
  }, 30 * 1000);
}

main()
  .then()
  .catch((err) => {
    console.error("エラー:", err);
  });
