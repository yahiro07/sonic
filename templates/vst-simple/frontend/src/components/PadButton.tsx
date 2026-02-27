export const PadButton = ({ label }: { label: string }) => {
  return (
    <div className="flex flex-col items-center gap-2">
      <div
        css={{
          width: "80px",
          height: "80px",
          border: "solid 1px #fff",
          borderRadius: "8px",
          display: "flex",
          alignItems: "center",
          justifyContent: "center",
          cursor: "pointer",
        }}
      >
        <img src="/vite.svg" />
      </div>
      <div>{label}</div>
    </div>
  );
};
