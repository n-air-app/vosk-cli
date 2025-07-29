
import { execSync } from 'child_process';
import {getExePath} from 'stt_cli';

function main(): void {
    const exePath: string = getExePath();
    console.log('stt_cli executable path:', exePath);
    
    console.log('\nExecuting stt_cli...');
    
      const output = execSync(`"${exePath}" -l`, {         encoding: 'utf8'
      });
      console.log(output);
}

main();