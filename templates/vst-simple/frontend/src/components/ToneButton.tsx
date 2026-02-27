import { usePointerCapture } from "@/hooks/use-pointer-capture";
import { css } from "@emotion/react";
import { useState, type FC } from "react";

type Props = {
  label: string;
  onPress?: () => void;
  onRelease?: () => void;
};

export const ToneButton: FC<Props> = ({ label, onPress, onRelease }) => {
  const [isPressed, setIsPressed] = useState(false);
  const handlePointerDown = usePointerCapture({
    onDown() {
      onPress?.();
      setIsPressed(true);
    },
    onUp() {
      onRelease?.();
      setIsPressed(false);
    },
  });
  return (
    <div css={styles.frame}>
      <div
        css={[styles.pad, isPressed && styles.padPressed]}
        onPointerDown={handlePointerDown}
      >
        <img src="/vite.svg" />
      </div>
      <div css={styles.label}>{label}</div>
    </div>
  );
};
const styles = {
  frame: css({
    display: "flex",
    flexDirection: "column",
    alignItems: "center",
    gap: "8px",
  }),
  pad: css({
    width: "80px",
    height: "80px",
    border: "solid 2px #fff",
    borderRadius: "8px",
    display: "flex",
    alignItems: "center",
    justifyContent: "center",
    cursor: "pointer",
  }),
  padPressed: css({
    background: "#fff2",
  }),
  label: css({
    fontWeight: "600",
    color: "#fff",
  }),
};
