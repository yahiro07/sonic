import { logger } from "@/bridge/logger";

type MessageFromUi =
  | { type: "uiLoaded" }
  | { type: "beginEdit"; paramKey: string }
  | { type: "performEdit"; paramKey: string; value: number }
  | { type: "endEdit"; paramKey: string }
  | { type: "instantEdit"; paramKey: string; value: number }
  | { type: "noteOnRequest"; noteNumber: number }
  | { type: "noteOffRequest"; noteNumber: number };

type MessageFromApp =
  | { type: "setParameter"; paramKey: string; value: number }
  | { type: "bulkSendParameters"; parameters: Record<string, number> }
  | { type: "hostNoteOn"; noteNumber: number }
  | { type: "hostNoteOff"; noteNumber: number };

const windowTyped = window as unknown as {
  webkit?: {
    messageHandlers: {
      pluginEditor: {
        postMessage: (msg: string | object) => void;
      };
    };
  };
  pluginEditorCallback?: (msg: MessageFromApp) => void;
};

type EditorBridgeMessageLister = (msg: MessageFromApp) => void;
const listeners: Set<EditorBridgeMessageLister> = new Set();

function sendMessage(msg: MessageFromUi) {
  logger.log("⇠ui", msg);
  windowTyped.webkit?.messageHandlers.pluginEditor.postMessage(msg);
}

function subscribe(listener: EditorBridgeMessageLister): () => void {
  listeners.add(listener);
  return () => {
    listeners.delete(listener);
  };
}

windowTyped.pluginEditorCallback = (msg) => {
  logger.log("⇢ui", msg);
  listeners.forEach((l) => l(msg));
};

export const coreBridge = {
  sendMessage,
  subscribe,
};
