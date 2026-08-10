const fs = require('fs');
const path = require('path');

// Get version from command line arguments
const newVersion = process.argv[2];

if (!newVersion) {
    console.log('Error: Please specify version number');
    console.log('Usage: node set-version.js <version>');
    console.log('Example: node set-version.js 1.2.3');
    process.exit(1);
}

console.log(`Setting version to ${newVersion}...`);

// Update package.json
const packageJsonPath = 'package.json';
if (fs.existsSync(packageJsonPath)) {
    try {
        const packageData = JSON.parse(fs.readFileSync(packageJsonPath, 'utf8'));
        packageData.version = newVersion;
        fs.writeFileSync(packageJsonPath, JSON.stringify(packageData, null, 2), 'utf8');
        console.log('+ package.json updated');
    } catch (error) {
        console.log('! Error updating package.json:', error.message);
    }
} else {
    console.log('! package.json not found');
}

// Update package-lock.json
const packageLockPath = 'package-lock.json';
if (fs.existsSync(packageLockPath)) {
    try {
        const lockData = JSON.parse(fs.readFileSync(packageLockPath, 'utf8'));
        lockData.version = newVersion;
        if (lockData.packages?.['']) {
            lockData.packages[''].version = newVersion;
        }
        fs.writeFileSync(packageLockPath, JSON.stringify(lockData, null, 2), 'utf8');
        console.log('+ package-lock.json updated');
    } catch (error) {
        console.log('! Error updating package-lock.json:', error.message);
    }
} else {
    console.log('! package-lock.json not found');
}

// Update the local package entry in the example lockfile
const examplePackageLockPath = 'example/package-lock.json';
if (fs.existsSync(examplePackageLockPath)) {
    try {
        const lockData = JSON.parse(fs.readFileSync(examplePackageLockPath, 'utf8'));
        if (lockData.packages?.['..']) {
            lockData.packages['..'].version = newVersion;
        }
        fs.writeFileSync(examplePackageLockPath, JSON.stringify(lockData, null, 2), 'utf8');
        console.log('+ example/package-lock.json updated');
    } catch (error) {
        console.log('! Error updating example/package-lock.json:', error.message);
    }
} else {
    console.log('! example/package-lock.json not found');
}

// Update vosk-cli.cpp
const cppPath = 'vosk-cli/vosk-cli.cpp';
if (fs.existsSync(cppPath)) {
    try {
        let content = fs.readFileSync(cppPath, 'utf8');
        content = content.replace(
            /VOSK_CLI_VERSION ".*"/,
            `VOSK_CLI_VERSION "${newVersion}"`
        );
        fs.writeFileSync(cppPath, content, 'utf8');
        console.log('+ vosk-cli/vosk-cli.cpp updated');
    } catch (error) {
        console.log('! Error updating vosk-cli.cpp:', error.message);
    }
} else {
    console.log('! vosk-cli/vosk-cli.cpp not found');
}

console.log('');
console.log(`Version ${newVersion} update completed!`);