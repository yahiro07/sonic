import { css } from "@emotion/react";
import type { FC } from "react";

type Props = {
  label: string;
  options: { value: number; label: string }[];
  value: number;
  onChange(value: number): void;
};

export const SelectorPad: FC<Props> = ({ label, options, value, onChange }) => {
  const onClick = () => {
    const index = options.findIndex((option) => option.value === value);
    const nextIndex = (index + 1) % options.length;
    onChange(options[nextIndex].value);
  };
  const currentLabel = options.find((option) => option.value === value)?.label;
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
