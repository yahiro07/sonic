# Webview Messaging

In this document, we describe the messaging interfaces between C++ and WebView.

## Data Format

```ts
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
```

Communication between C++ and WebView is handled using JSON objects.
They have a common `type` field to identify the message type and additional fields for the message content.

Parameters are identified by string `paramKey` which is defined when setting up parameters in the C++ side.

## Communication Interfaces

```ts
function sendMessage(msg: MessageFromUi) {
  //send message to C++ side
  window.webkit.messageHandlers.pluginEditor.postMessage(msg);
}

window.pluginEditorCallback = (msg: MessageFromApp) => {
  //receiver callback for messages sent from C++ side
  switch (msg.type) {
    case "setParameter":
      break;
    case "bulkSendParameters":
      break;
    case "hostNoteOn":
      break;
    case "hostNoteOff":
      break;
  }
};
```

From WebView side, we can sends messages with `window.webkit.messageHandlers.pluginEditor.postMessage()` and can receives messages by setting callback function to `window.pluginEditorCallback`.

## Loading Initial Parameters

After the page load, the UI sends `{ type: “uiLoaded” }`. In response, the C++ side sends `{ type: “bulkSendParameters”, parameters: { ... } }`. This populates the UI with the initial parameter values.

## Editing Parameters

Changes to the parameters are sent to the C++ side via messages such as `beginEdit`, `performEdit`, and `endEdit` in response to UI operations.

```ts
  | { type: "beginEdit"; paramKey: string }
  | { type: "performEdit"; paramKey: string; value: number }
  | { type: "endEdit"; paramKey: string }
  | { type: "instantEdit"; paramKey: string; value: number }
```

For the knob or slider like UI, `beginEdit` is sent when the user starts touching the knob or slider, `performEdit` is sent when the user moves the knob or slider, and `endEdit` is sent when the user releases the knob or slider.

For the button or toggle switch selector like UI, just `instantEdit` is sent when the value is changed. This wraps `beginEdit`, `performEdit`, and `endEdit` in one message.

## Parameter Changes from the C++ Side

When parameters are changed via the host’s parameter UI or automation, a `setParameter` message is sent from the C++ side. The UI receives this message and updates the parameter display.

When multiple parameters are changed—such as when loading a preset or restoring the plugin’s state a `bulkSendParameters` message is sent including all the parameter values.

## Note Requests from the UI

From the UI side, you can request note-on or note-off events for DSP processing by sending `noteOnRequest` or `noteOffRequest`. These are internal plugin messages and are not sent to the host.

## Receiving Notes Sent from the Host

The UI can also receive note information sent from the host to the plugin. When `hostNoteOn` or `hostNoteOff` messages are received, the UI can use them to update the keyboard display and other elements.

Similarly, when the UI sends `noteOnRequest` or `noteOffRequest`, the DSP processing side also returns a `hostNoteOn` or `hostNoteOff` message as a response.
