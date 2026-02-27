const PadButton = ({ label }: { label: string }) => {
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

const Knob = ({ label }: { label: string }) => {
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

const MainPanel = () => {
  return (
    <div className="flex flex-col gap-4 w-[500px] h-[200px] border border-white p-2">
      <h1 className="text-xl">MySynth</h1>
      <div className="grow flex justify-between items-center px-10 mt-[-10px]">
        <PadButton label="Note(60)" />
        <Knob label="Pitch" />
        <Knob label="Volume" />
      </div>
    </div>
  );
};

export const App = () => {
  return (
    <div className="h-dvh bg-gray-400 flex items-center justify-center text-white">
      <MainPanel />
    </div>
  );
};
