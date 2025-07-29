const path = require('path');

function getExePath() {
  // __dirname は /node_modules/stt_cli/src になるので、../bin/stt_cli.exe で正しいパスになる
  return path.resolve(__dirname, '../bin/stt_cli.exe');
}

module.exports = {
  getExePath
};
