import { coreBridge } from "@/bridge/core-bridge";
import { useEffect, useMemo, useState } from "react";

type ParametersRecord = Record<string, number | boolean>;

export type EditorPresenterBase<T extends ParametersRecord> = {
  beginEdit(paramKey: keyof T): void;
  performEdit(paramKey: keyof T, value: number): void;
  endEdit(paramKey: keyof T): void;
  instantEdit(paramKey: keyof T, value: number): void;
  requestNoteOn(noteNumber: number): void;
  requestNoteOff(noteNumber: number): void;
  parameters: T;
  hostNoteNumbers: number[];
};

export function useEditorPresenterBase<T extends ParametersRecord>(
  initialParameters: T,
): EditorPresenterBase<T> {
  const [parameters, setParameters] = useState(initialParameters);
  const [hostNoteNumbers, setHostNoteNumbers] = useState<number[]>([]);

  const boolFlags = useMemo(() => {
    const flags: Record<string, boolean> = {};
    for (const [key, value] of Object.entries(initialParameters)) {
      if (typeof value === "boolean") {
        flags[key] = value;
      }
    }
    return flags;
  }, [initialParameters]);

  useEffect(() => {
    const unsubscribe = coreBridge.subscribe((msg) => {
      if (msg.type === "setParameter") {
        const isBoolean = boolFlags[msg.paramKey];
        const newValue = isBoolean ? msg.value > 0.5 : msg.value;
        setParameters((prev) => ({
          ...prev,
          [msg.paramKey]: newValue,
        }));
      } else if (msg.type === "bulkSendParameters") {
        setParameters((prev) => {
          const newParameters = { ...prev } as Record<string, number | boolean>;
          for (const [paramKey, value] of Object.entries(msg.parameters)) {
            const isBoolean = boolFlags[paramKey];
            const newValue = isBoolean ? value > 0.5 : value;
            newParameters[paramKey] = newValue;
          }
          return newParameters as T;
        });
      } else if (msg.type === "hostNoteOn") {
        setHostNoteNumbers((prev) => [...prev, msg.noteNumber]);
      } else if (msg.type === "hostNoteOff") {
        setHostNoteNumbers((prev) =>
          prev.filter((noteNumber) => noteNumber !== msg.noteNumber),
        );
      }
    });
    return unsubscribe;
  }, [initialParameters, boolFlags]);

  const actionsInternal = {
    performEdit(
      paramKey: string,
      value: number | boolean,
      isInstantEdit: boolean,
    ) {
      setParameters((prev) => ({ ...prev, [paramKey]: value }));
      const numberValue = typeof value === "boolean" ? (value ? 1 : 0) : value;
      coreBridge.sendMessage({
        type: "performParameterEdit",
        paramKey,
        value: numberValue,
        isInstantEdit,
      });
    },
  };

  const actions = {
    beginEdit(paramKey: string) {
      coreBridge.sendMessage({ type: "beginParameterEdit", paramKey });
    },
    performEdit(paramKey: string, value: number | boolean) {
      actionsInternal.performEdit(paramKey, value, false);
    },
    endEdit(paramKey: string) {
      coreBridge.sendMessage({ type: "endParameterEdit", paramKey });
    },
    instantEdit(paramKey: string, value: number | boolean) {
      actionsInternal.performEdit(paramKey, value, true);
    },
    requestNoteOn(noteNumber: number) {
      coreBridge.sendMessage({ type: "noteOnRequest", noteNumber });
    },
    requestNoteOff(noteNumber: number) {
      coreBridge.sendMessage({ type: "noteOffRequest", noteNumber });
    },
  };

  useEffect(() => {
    coreBridge.sendMessage({ type: "uiLoaded" });
  }, []);

  return {
    parameters,
    hostNoteNumbers,
    ...actions,
  };
}
