const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

const version = require('../package.json').version;
const releaseName = `vosk-cli-v${version}-windows-x64`;
const releaseDir = releaseName;
const tarballName = `${releaseName}.tar.gz`;
const tag = `${version}`;  // vプレフィックスなし
const isDraft = process.argv.includes('--draft');

console.log(`Creating release: ${releaseName}${isDraft ? ' (draft)' : ''}`);

// GitHub CLIがインストールされているか確認
try {
  execSync('gh --version', { stdio: 'pipe' });
} catch (error) {
  console.error('❌ Error: GitHub CLI (gh) is not installed.');
  console.error('Install from: https://cli.github.com/');
  process.exit(1);
}

// クリーンアップ
if (fs.existsSync(releaseDir)) {
  fs.rmSync(releaseDir, { recursive: true, force: true });
}
if (fs.existsSync(tarballName)) {
  fs.unlinkSync(tarballName);
}

// リリースディレクトリを作成
fs.mkdirSync(releaseDir, { recursive: true });

// ファイルをコピー
console.log('📂 Copying files:');
const filesToCopy = ['package.json', 'LICENSE'];

filesToCopy.forEach(file => {
  if (fs.existsSync(file)) {
    fs.copyFileSync(file, path.join(releaseDir, file));
    console.log(`  ✓ ${file}`);
  }
});

// フォルダをコピー
['bin', 'src'].forEach(folder => {
  const srcPath = folder;
  const destPath = path.join(releaseDir, folder);
  if (fs.existsSync(srcPath)) {
    console.log(`  ✓ ${folder}/`);
    fs.cpSync(srcPath, destPath, { recursive: true });
  } else {
    console.error(`❌ Error: ${folder}/ not found`);
    process.exit(1);
  }
});

// tarballを作成
console.log('\n📦 Creating tarball...');
try {
  execSync(`tar -czf ${tarballName} ${releaseDir}`, { stdio: 'inherit' });
  const stats = fs.statSync(tarballName);
  const fileSizeInMB = (stats.size / (1024 * 1024)).toFixed(2);
  console.log(`  ✓ Created: ${tarballName} (${fileSizeInMB} MB)`);
} catch (error) {
  console.error('❌ Error creating tarball:', error.message);
  process.exit(1);
}

// クリーンアップ
fs.rmSync(releaseDir, { recursive: true, force: true });

// GitHubリリースを作成
console.log('\n🚀 Creating GitHub release...');
try {
  // タグが存在するか確認
  try {
    execSync(`git rev-parse ${tag}`, { stdio: 'pipe' });
    console.log(`  ℹ Tag ${tag} already exists`);
  } catch {
    console.log(`  ⚡ Creating tag ${tag}...`);
    execSync(`git tag ${tag}`, { stdio: 'inherit' });
    execSync(`git push origin ${tag}`, { stdio: 'inherit' });
  }

  // リリースが既に存在するか確認
  let releaseExists = false;
  try {
    execSync(`gh release view ${tag}`, { stdio: 'pipe' });
    releaseExists = true;
    console.log(`  ℹ Release ${tag} already exists`);
  } catch {
    // リリースが存在しない
  }

  const releaseTitle = `vosk-cli ${tag}`;
  const releaseNotes = `Release ${tag}

## Installation

Download \`${tarballName}\` and extract it.

## Usage

\`\`\`bash
vosk-cli -h
\`\`\`
`;
  
  const draftFlag = isDraft ? '--draft' : '';

  if (releaseExists) {
    // 既存リリースにファイルをアップロード（上書き）
    console.log(`  ⚡ Uploading ${tarballName} to existing release...`);
    execSync(
      `gh release upload ${tag} ${tarballName} --clobber`,
      { stdio: 'inherit' }
    );
  } else {
    // 新規リリースを作成
    console.log(`  ⚡ Creating new release...`);
    execSync(
      `gh release create ${tag} ${tarballName} --title "${releaseTitle}" --notes "${releaseNotes}" ${draftFlag}`,
      { stdio: 'inherit' }
    );
  }
  
  console.log('\n✅ GitHub release created successfully!');
  
  // リリースURLを取得
  try {
    const repoInfo = execSync('gh repo view --json nameWithOwner -q .nameWithOwner', { encoding: 'utf8' }).trim();
    const releaseUrl = `https://github.com/${repoInfo}/releases/download/${tag}/${tarballName}`;
    console.log(`🔗 View release: https://github.com/${repoInfo}/releases/tag/${tag}`);
    console.log(`\n📦 To use this package in your project, add to package.json:`);
    console.log(`\n  "dependencies": {`);
    console.log(`    "vosk-cli": "${releaseUrl}"`);
    console.log(`  }`);
  } catch {}
  
} catch (error) {
  console.error('\n❌ Error creating GitHub release:', error.message);
  console.log('\n💡 You can manually create the release:');
  console.log(`  gh release create ${tag} ${tarballName} --title "vosk-cli ${tag}"`);
  console.log(`Or upload to existing release:`);
  console.log(`  gh release upload ${tag} ${tarballName} --clobber`);
  process.exit(1);
}
