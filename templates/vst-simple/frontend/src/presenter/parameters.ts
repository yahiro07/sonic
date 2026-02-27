export enum WaveType {
  Saw = 0,
  Rect,
  Tri,
  Sine,
}

export type Parameters = {
  wave: WaveType;
  pitch: number;
  volume: number;
  isOn: boolean;
};

export const defaultParameters: Parameters = {
  wave: WaveType.Saw,
  pitch: 0.5,
  volume: 0.5,
  isOn: true,
};
