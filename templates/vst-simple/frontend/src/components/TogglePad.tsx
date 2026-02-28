import { css } from "@emotion/react";
import type { FC } from "react";

type Props = {
  label: string;
  value: boolean;
  onChange(value: boolean): void;
};

export const TogglePad: FC<Props> = ({ label, value, onChange }) => {
  const onClick = () => {
    onChange(!value);
  };
  const currentLabel = value ? "ON" : "OFF";
  return (
    <div css={styles.frame}>
      <div css={styles.pad} onClick={onClick}>
        {currentLabel}
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
    display: "flex",
    alignItems: "center",
    justifyContent: "center",
    cursor: "pointer",
    fontWeight: "600",
    fontSize: "20px",
  }),
  label: css({
    fontWeight: "600",
    color: "#fff",
  }),
};
