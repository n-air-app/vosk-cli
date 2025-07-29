import { ChildProcess } from "child_process";

export interface AudioDevice {
  index: number;
  id: string;
  name: string;
}

export interface SpeechRecognitionOutput {
  text?: string;
  partial?: string;
  info?: string;
  error?: string;
}

export declare function getExePath(): string;
export declare function getVersion(): string;
export declare function getDevices(): AudioDevice[];
export declare function startSpeechRecognition(
  deviceIndex: number,
  modelPath: string,
  onOutput: (jsonOutput: SpeechRecognitionOutput) => void
): ChildProcess;
