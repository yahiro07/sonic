export const Knob = ({ label }: { label: string }) => {
  return (
    <div className="flex flex-col items-center gap-2">
      <div
        css={{
          width: "80px",
          height: "80px",
          border: "solid 1px #fff",
          borderRadius: "50%",
          cursor: "pointer",
        }}
      ></div>
      <div>{label}</div>
    </div>
  );
};
