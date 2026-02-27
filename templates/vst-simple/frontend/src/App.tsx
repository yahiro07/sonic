import { logger } from "@/bridge/logger";
import { Knob } from "@/components/Knob";
import { ToneButton } from "@/components/ToneButton";
import { useEffect } from "react";

const MainPanel = () => {
  return (
    <div className="flex flex-col gap-4 w-[500px] h-[200px] border border-white p-2">
      <h1 className="text-xl px-1">MySynth</h1>
      <div className="grow flex justify-between items-center px-10 mt-[-10px]">
        <ToneButton
          label="Note(60)"
          onPress={() => console.log("pressed")}
          onRelease={() => console.log("released")}
        />
        <Knob label="Pitch" value={0.5} />
        <Knob label="Volume" value={1} />
      </div>
    </div>
  );
};

export const App = () => {
  useEffect(() => {
    logger.mark("frontend start");
  }, []);
  return (
    <div className="h-dvh bg-gray-400 flex items-center justify-center text-white">
      <MainPanel />
    </div>
  );
};
