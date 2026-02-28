import { css } from "@emotion/react";
import { useState, type FC } from "react";

type Props = {
  label: string;
  onPress?: () => void;
  onRelease?: () => void;
};

export const ToneButton: FC<Props> = ({ label, onPress, onRelease }) => {
  const [isPressed, setIsPressed] = useState(false);
  return (
    <div css={styles.frame}>
      <div
        css={[styles.pad, isPressed && styles.padPressed]}
        onPointerDown={() => {
          onPress?.();
          setIsPressed(true);
        }}
        onPointerUp={() => {
          onRelease?.();
          setIsPressed(false);
        }}
      >
        {/* grommet-icons:trigger */}
        <svg
          xmlns="http://www.w3.org/2000/svg"
          width={32}
          height={32}
          viewBox="0 0 24 24"
        >
          <path
            fill="none"
            stroke="currentColor"
            strokeWidth={2}
            d="M4 14h6l-3 9h2L20 9h-6l4-8H7z"
          ></path>
        </svg>
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
    borderRadius: "10px",
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
