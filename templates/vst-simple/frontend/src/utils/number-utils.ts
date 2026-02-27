export function clampValue(value: number, min: number, max: number) {
  return Math.max(min, Math.min(max, value));
}

export function linearInterpolate(
  val: number,
  s0: number,
  s1: number,
  d0: number,
  d1: number,
  applyClamp?: boolean,
) {
  const res = ((val - s0) / (s1 - s0)) * (d1 - d0) + d0;
  if (applyClamp) {
    const lo = Math.min(d0, d1);
    const hi = Math.max(d0, d1);
    return clampValue(res, lo, hi);
  }
  return res;
}
