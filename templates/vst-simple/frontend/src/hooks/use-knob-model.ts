import { clampValue, linearInterpolate } from "@/utils/number-utils";
import {
  usePointerCapture,
  type PointerPoint,
} from "@/hooks/use-pointer-capture";
import { useRef, useState } from "react";

export function useKnobModel(props: {
  value: number;
  onChange?: (value: number) => void;
  min?: number;
  max?: number;
  step?: number;
  dragRange?: number;
  tickDisplaySteps?: number;
  onStartEdit?: () => void;
  onEndEdit?: () => void;
}) {
  const {
    value: originalValue,
    onChange,
    min = 0,
    max = 1,
    step,
    dragRange = 100,
    tickDisplaySteps,
    onStartEdit,
    onEndEdit,
  } = props;

  const [editValue, setEditValue] = useState<number | undefined>(undefined);

  const displaySourceValue = editValue ?? originalValue;
  let normValue = linearInterpolate(displaySourceValue, min, max, 0, 1);
  if (tickDisplaySteps) {
    normValue = Math.round(normValue * tickDisplaySteps) / tickDisplaySteps;
  }
  const originalPos = useRef<PointerPoint | null>(null);

  const handlePointerDown = usePointerCapture({
    onDown(point) {
      originalPos.current = point;
      setEditValue(originalValue);
      onStartEdit?.();
    },
    onMove(point) {
      const diffXY = {
        x: point.x - originalPos.current!.x,
        y: point.y - originalPos.current!.y,
      };
      const shiftAmount = clampValue((diffXY.x - diffXY.y) / dragRange, -1, 1);
      const range = max - min;
      let newValue = clampValue(originalValue + shiftAmount * range, min, max);
      if (step) {
        newValue = Math.round(newValue / step) * step;
      }
      setEditValue(newValue);
      onChange?.(newValue);
    },
    onUp() {
      setEditValue(undefined);
      onEndEdit?.();
    },
  });

  return {
    normValue,
    handlePointerDown,
  };
}
