import SpeechToText from "stt_cli";

console.log(SpeechToText.getExePath());
console.log(SpeechToText.getVersion());
console.log(SpeechToText.getDevices());

// 音声認識のテスト
console.log("\n音声認識を開始します...");
const child = SpeechToText.start({
  deviceIndex: 0,
  modelPath: "../model/vosk-model-small-ja-0.22",
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
