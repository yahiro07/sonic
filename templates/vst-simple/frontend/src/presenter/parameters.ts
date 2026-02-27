export enum WaveType {
  Saw = 0,
  Rect,
  Tri,
  Sine,
}
export const waveTypeOptions = [
  { value: WaveType.Saw, label: "Saw" },
  { value: WaveType.Rect, label: "Rect" },
  { value: WaveType.Tri, label: "Tri" },
  { value: WaveType.Sine, label: "Sine" },
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
