import { Knob } from "@/components/Knob";
import { ToneButton } from "@/components/ToneButton";
import { useUiPresenter } from "@/presenter/ui-preseter-context";

const ControlsContent = () => {
  const {
    requestNoteOn,
    requestNoteOff,
    parameters,
    beginEdit,
    performEdit,
    endEdit,
  } = useUiPresenter();
  return (
    <>
      <ToneButton
        label="Note(60)"
        onPress={() => requestNoteOn(60)}
        onRelease={() => requestNoteOff(60)}
      />
      <Knob
        label="Pitch"
        value={parameters.oscPitch}
        onStartEdit={() => beginEdit("oscPitch")}
        onChange={(value) => performEdit("oscPitch", value)}
        onEndEdit={() => endEdit("oscPitch")}
      />
      <Knob
        label="Volume"
        value={parameters.oscVolume}
        onStartEdit={() => beginEdit("oscVolume")}
        onChange={(value) => performEdit("oscVolume", value)}
        onEndEdit={() => endEdit("oscVolume")}
      />
    </>
  );
};

const MainPanel = () => {
  return (
    <div className="flex flex-col gap-4 w-[500px] h-[200px] border border-white p-2">
      <h1 className="text-xl px-1">MySynth</h1>
      <div className="grow flex justify-between items-center px-10 mt-[-10px]">
        <ControlsContent />
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
