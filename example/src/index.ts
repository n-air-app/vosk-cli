
import {getExePath, getVersion, getDevices, startSpeechRecognition} from 'stt_cli';

console.log(getExePath());  
console.log(getVersion());
console.log(getDevices()); 

// 音声認識のテスト
console.log('\n音声認識を開始します...');
const child = startSpeechRecognition(0, '../model/vosk-model-small-ja-0.22',
  (jsonOutput) => {
    console.log('受信:', JSON.stringify(jsonOutput));
    
    // 特定の出力に応じて処理
    if (jsonOutput.info === 'start') {
      console.log('音声認識が開始されました');
    } else if (jsonOutput.text) {
      console.log('認識結果:', jsonOutput.text);
    } else if (jsonOutput.partial) {
      console.log('部分認識:', jsonOutput.partial);
    }
  }
);

setTimeout(() => {
  console.log('\n音声認識を終了します...');
  child.kill();
}, 30 * 1000); 
