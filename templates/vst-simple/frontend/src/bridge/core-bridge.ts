type MessageFromUi =
  | { type: "uiLoaded" }
  | { type: "beginParameterEdit"; paramKey: string }
  | { type: "performParameterEdit"; paramKey: string; value: number }
  | { type: "endParameterEdit"; paramKey: string }
  | { type: "applyInstantEdit"; paramKey: string; value: number }
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
        postMessage: (body: string) => void;
      };
    };
  };
  pluginEditorCallback?: (msg: MessageFromApp) => void;
};

type EditorBridgeMessageLister = (msg: MessageFromApp) => void;
const listeners: Set<EditorBridgeMessageLister> = new Set();

function sendMessage(msg: MessageFromUi) {
  windowTyped.webkit?.messageHandlers.pluginEditor.postMessage(
    JSON.stringify(msg),
  );
}

function subscribe(listener: EditorBridgeMessageLister): () => void {
  listeners.add(listener);
  return () => {
    listeners.delete(listener);
  };
}

windowTyped.pluginEditorCallback = (msg) => {
  listeners.forEach((l) => l(msg));
};

export const coreBridge = {
  sendMessage,
  subscribe,
};
