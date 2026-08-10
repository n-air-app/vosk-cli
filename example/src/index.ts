import Vosk from "vosk-cli";


async function main() {
  console.log(Vosk.getExePath());
  console.log(Vosk.getVersion());
  const devices = Vosk.getDevices();
  console.log(devices);

  if (devices.length === 0) {
    throw new Error("利用可能なオーディオデバイスがありません");
  }

  // 音声認識のテスト
  console.log("\n音声認識を開始します...");
  const child = Vosk.start({
    deviceId: devices[0].id,
    modelPath:`../model/vosk-model-small-ja-0.22`,
    onData: (data) => {
      console.log("受信:", JSON.stringify(data));

      // 特定の出力に応じて処理
      if (data.info === "start") {
        console.log("音声認識が開始されました");
      } else if (data.info === "device_reconnecting") {
        console.log("オーディオデバイスへの再接続を試行しています");
      } else if (data.info === "device_reconnected") {
        console.log("オーディオデバイスへ再接続しました");
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
