export enum OscWaveType {
  Saw = 0,
  Square,
  Triangle,
  Sine,
}
export const waveTypeOptions = [
  { value: OscWaveType.Saw, label: "SAW" },
  { value: OscWaveType.Square, label: "SQUARE" },
  { value: OscWaveType.Triangle, label: "TRI" },
  { value: OscWaveType.Sine, label: "SINE" },
];

export type Parameters = {
  oscEnabled: boolean;
  oscWave: OscWaveType;
  oscPitch: number;
  oscVolume: number;
};

export const defaultParameters: Parameters = {
  oscEnabled: true,
  oscWave: OscWaveType.Saw,
  oscPitch: 0.5,
  oscVolume: 0.5,
};
