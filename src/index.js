const path = require("path");
const { execSync, spawn } = require("child_process");
const {
  downloadModel,
  isExistModel,
} = require("./download");

function getExePath() {
  return path.resolve(__dirname, "../bin/stt_cli.exe");
}

function getVersion() {
  try {
    const exePath = getExePath();
    const result = execSync(`"${exePath}" -l`, { encoding: "utf8" });
    const parsed = JSON.parse(result);
    return parsed.version;
  } catch (error) {
    return "";
  }
}

function getDevices() {
  try {
    const exePath = getExePath();
    const result = execSync(`"${exePath}" -l`, { encoding: "utf8" });
    const parsed = JSON.parse(result);
    return parsed.devices;
  } catch (error) {
    return [];
  }
}

function start({ deviceIndex, modelPath, onData } = {}) {
  const args = ["-d", (deviceIndex ?? 0).toString()];
  if (modelPath) args.push("-m", modelPath);

  const child = spawn(getExePath(), args, { stdio: ["pipe", "pipe", "pipe"] });
  let buffer = "";

  child.stdout.on("data", (data) => {
    buffer += data.toString();

    const lines = buffer.split("\n");
    buffer = lines.pop();

    lines.forEach((line) => {
      line = line.trim();
      if (line) {
        try {
          const parsed = JSON.parse(line);
          if (onData) onData(parsed);
        } catch (error) {
          // JSONパースエラーは無視（不完全なデータの可能性）
        }
      }
    });
  });

  child.on("close", (code) => {
    if (buffer.trim()) {
      try {
        const parsed = JSON.parse(buffer.trim());
        if (onData) onData(parsed);
      } catch (error) {
        // JSONパースエラーは無視
      }
    }
  });

  return child;
}

const SpeechToText = {
  getExePath,
  getVersion,
  getDevices,
  start,
  downloadModel,
  isExistModel,
};

module.exports = SpeechToText;
module.exports.default = SpeechToText;
