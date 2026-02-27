import { css } from "@emotion/react";
import type { FC } from "react";

type Props = {
  label: string;
};

export const PadButton: FC<Props> = ({ label }) => {
  return (
    <div css={styles.frame}>
      <div css={styles.pad}>
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
  label: css({
    fontWeight: "600",
    color: "#fff",
  }),
};
