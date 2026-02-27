import { css } from "@emotion/react";
import type { FC } from "react";

type Props = {
  label: string;
  value: number;
};

export const Knob: FC<Props> = ({ label, value }) => {
  const tickAngle = (value * 2 - 1) * 135;
  return (
    <div css={styles.frame}>
      <div css={styles.knobBase}>
        <div css={styles.knobBody} />
        <div
          css={styles.tickPlane}
          style={{ transform: `rotate(${tickAngle}deg)` }}
        >
          <div css={styles.tick} />
        </div>
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
  knobBase: css({
    display: "grid",
    placeItems: "center",
    " > *": {
      gridArea: "1 / 1",
    },
    cursor: "pointer",
  }),
  knobBody: css({
    width: "80px",
    height: "80px",
    border: "solid 2px #fff",
    borderRadius: "50%",
  }),
  tickPlane: css({
    width: "100%",
    height: "100%",
    display: "flex",
    flexDirection: "column",
    alignItems: "center",
  }),
  tick: css({
    width: "5px",
    height: "18px",
    backgroundColor: "#fff",
  }),
  label: css({
    fontWeight: "600",
    color: "#fff",
  }),
};
