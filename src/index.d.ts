import { ChildProcess } from "child_process";

export interface AudioDevice {
  index: number;
  id: string;
  name: string;
}

export interface VoskOutput {
  text?: string;
  partial?: string;
  info?: string;
  error?: string;
}

export interface VoskOptions {
  deviceIndex?: number;
  modelPath?: string;
  onData: (output: VoskOutput) => void;
}


declare const Vosk: {
  getExePath: () => string;
  getVersion: () => string;
  getDevices: () => AudioDevice[];
  start: (options: VoskOptions) => ChildProcess;
};

export default Vosk;
