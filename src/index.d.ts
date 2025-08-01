import { ChildProcess } from "child_process";

export interface AudioDevice {
  index: number;
  id: string;
  name: string;
}

export interface SpeechToTextOutput {
  text?: string;
  partial?: string;
  info?: string;
  error?: string;
}

export interface SpeechToTextOptions {
  deviceIndex?: number;
  modelPath?: string;
  onData: (output: SpeechToTextOutput) => void;
}


declare const SpeechToText: {
  getExePath: () => string;
  getVersion: () => string;
  getDevices: () => AudioDevice[];
  start: (options: SpeechToTextOptions) => ChildProcess;

  downloadModel: (
    url: string,
    modelPath: string,
    tempDir: string,
    force?: boolean
  ) => Promise<void>;
  isExistModel: (modelPath: string) => boolean;
  models: Models;
};

export default SpeechToText;
