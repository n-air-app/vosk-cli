const path = require("path");
const fs = require("fs");
const https = require("https");
const AdmZip = require("adm-zip");

function downloadFile(url, filePath) {
  return new Promise((resolve, reject) => {
    const file = fs.createWriteStream(filePath);

    https
      .get(url, (response) => {
        if (response.statusCode !== 200) {
          reject(new Error(`HTTP Error: ${response.statusCode}`));
          return;
        }

        response.pipe(file);

        file.on("finish", () => {
          file.close();
          resolve();
        });

        file.on("error", (error) => {
          reject(error);
        });
      })
      .on("error", (error) => {
        reject(error);
      });
  });
}

async function downloadAndExtract(url, modelPath, tempDir) {
  const tempZipPath = path.join(tempDir, "temp.zip");
  const tempExtractDir = path.join(tempDir, "extracted");

  await downloadFile(url, tempZipPath);

  // 展開
  const zip = new AdmZip(tempZipPath);
  zip.extractAllTo(tempExtractDir, true);

  // 一時ディレクトリ内の最初のディレクトリを探す
  const extractedItems = fs.readdirSync(tempExtractDir);
  const extractedDirBase = extractedItems.find((item) =>
    fs.statSync(path.join(tempExtractDir, item)).isDirectory()
  );
  if (!extractedDirBase)
    throw new Error("No directory found in extracted archive");
  const extractedDir = path.join(tempExtractDir, extractedDirBase);

  // モデルのパスを設定
  const modelParentDir = path.dirname(modelPath);
  if (!fs.existsSync(modelParentDir))
    fs.mkdirSync(modelParentDir, { recursive: true });

  // 移動
  fs.renameSync(extractedDir, modelPath);
}

async function downloadModel(url, modelPath, tempDir, force = false) {
  if (isExistModel(modelPath) && !force) return;

  const temp = path.join(tempDir, `temp_${Date.now()}`);

  try {
  if (!fs.existsSync(tempDir)) fs.mkdirSync(tempDir, { recursive: true });
    await downloadAndExtract(url, modelPath, temp);
  } catch (error) {
    throw error;
  } finally {
    if (fs.existsSync(temp)) fs.rmSync(temp, { recursive: true, force: true });
  }
}

function isExistModel(modelPath) {
  // モデルのconfigが存在するか確認
  try {
    const conf = path.join(modelPath, "conf/model.conf");
    return fs.existsSync(conf);
  } catch (error) {
    return false;
  }
}

module.exports = {
  downloadModel,
  isExistModel,
};
