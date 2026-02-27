export enum WaveType {
  Saw = 0,
  Rect,
  Tri,
  Sine,
}
export const waveTypeOptions = [
  { value: WaveType.Saw, label: "SAW" },
  { value: WaveType.Rect, label: "RECT" },
  { value: WaveType.Tri, label: "TRI" },
  { value: WaveType.Sine, label: "SINE" },
];

export type Parameters = {
  wave: WaveType;
  pitch: number;
  volume: number;
  enabled: boolean;
};

export const defaultParameters: Parameters = {
  wave: WaveType.Saw,
  pitch: 0.5,
  volume: 0.5,
  enabled: true,
};
