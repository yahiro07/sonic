import { Knob } from "@/components/Knob";
import { PadButton } from "@/components/PadButton";

const MainPanel = () => {
  return (
    <div className="flex flex-col gap-4 w-[500px] h-[200px] border border-white p-2">
      <h1 className="text-xl px-1">MySynth</h1>
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
