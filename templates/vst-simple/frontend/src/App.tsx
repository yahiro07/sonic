import { Knob } from "@/components/Knob";
import { SelectorPad } from "@/components/SelectorPad";
import { ToneButton } from "@/components/ToneButton";
import { waveTypeOptions } from "@/presenter/parameters";
import { useUiPresenter } from "@/presenter/ui-preseter-context";
import { useState } from "react";

const ControlsContent = () => {
  const {
    requestNoteOn,
    requestNoteOff,
    parameters,
    beginEdit,
    performEdit,
    endEdit,
    instantEdit,
  } = useUiPresenter();
  return (
    <>
      <ToneButton
        label="Note(60)"
        onPress={() => requestNoteOn(60)}
        onRelease={() => requestNoteOff(60)}
      />
      <SelectorPad
        label="Wave"
        options={waveTypeOptions}
        value={parameters.wave}
        onChange={(value) => instantEdit("wave", value)}
      />
      <Knob
        label="Pitch"
        value={parameters.pitch}
        onStartEdit={() => beginEdit("pitch")}
        onChange={(value) => performEdit("pitch", value)}
        onEndEdit={() => endEdit("pitch")}
      />
      <Knob
        label="Volume"
        value={parameters.volume}
        onStartEdit={() => beginEdit("volume")}
        onChange={(value) => performEdit("volume", value)}
        onEndEdit={() => endEdit("volume")}
      />
      <Knob
        label="On"
        value={parameters.isOn ? 1 : 0}
        step={1}
        onChange={(value) => instantEdit("isOn", value ? true : false)}
      />
    </>
  );
};

const NoteIndicator = () => {
  const { hostNoteNumbers } = useUiPresenter();
  const hasHostNote = hostNoteNumbers.length > 0;
  if (!hasHostNote) return;
  return <div className="w-[18px] h-[18px] bg-teal-400 rounded-full" />;
};

const DebugPart = () => {
  const { parameters, hostNoteNumbers } = useUiPresenter();
  const [visible, setVisible] = useState(false);
  return (
    <div className="flex gap-4 px-2 items-center">
      {visible && (
        <div>
          {JSON.stringify({
            notes: hostNoteNumbers,
            wave: parameters.wave,
            pitch: parameters.pitch.toFixed(2),
            volume: parameters.volume.toFixed(2),
            isOn: parameters.isOn,
          })}
        </div>
      )}
      <button onClick={() => setVisible(!visible)} className="cursor-pointer">
        debug
      </button>
    </div>
  );
};

const MainPanel = () => {
  return (
    <div className="flex flex-col gap-2 w-[700px] border border-white p-2">
      <div className="flex items-center gap-2">
        <h1 className="text-xl px-1">MySynth</h1>
        <NoteIndicator />
        <div className="grow" />
        <DebugPart />
      </div>
      <div className="grow flex justify-between items-center px-10 py-8">
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
