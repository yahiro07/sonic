// type MessageFromUi =
//   | { type: "uiLoaded" }
//   | { type: "beginEdit"; identifier: string }
//   | { type: "applyEdit"; identifier: string; value: number }
//   | { type: "endEdit"; identifier: string }
//   | { type: "applyInstantEdit"; identifier: string; value: number }
//   | { type: "noteOnRequest"; noteNumber: number }
//   | { type: "noteOffRequest"; noteNumber: number };

// type MessageFromApp =
//   | { type: "setParameter"; identifier: string; value: number }
//   | { type: "bulkSendParameters"; parameters: Record<string, number> }
//   | { type: "hostNoteOn"; noteNumber: number }
//   | { type: "hostNoteOff"; noteNumber: number };

function pushLine(...args) {
  const div = document.createElement("div");
  div.innerText = args
    .map((it) => (typeof it === "object" ? JSON.stringify(it) : it))
    .join(" ");
  document.body.appendChild(div);
}

function createCoreBridge() {
  const listeners = new Set();

  function sendMessage(msg) {
    pushLine("⇠ui", msg);
    window.webkit.messageHandlers.pluginEditor.postMessage(msg);
  }

  function subscribe(listener) {
    listeners.add(listener);
    return () => {
      listeners.delete(listener);
    };
  }

  window.pluginEditorCallback = (msg) => {
    pushLine("⇢ui", msg);
    listeners.forEach((l) => l(msg));
  };

  return {
    sendMessage,
    subscribe,
  };
}
const { sendMessage, subscribe } = createCoreBridge();

function sendParameter(identifier, value) {
  sendMessage({
    type: "applyEdit",
    identifier,
    value,
  });
}

pushLine("hello from js 1235");

pushLine("href:" + location.href);

sendMessage({ type: "uiLoaded" });

// window.webkit.messageHandlers.native.postMessage(
//   JSON.stringify({ data: "A" })
// );

// setTimeout(() => {
//   window.webkit.messageHandlers.native.postMessage(
//     JSON.stringify({ data: "B" })
//   );
//   pushLine("B")
// }, 2000)

// setTimeout(() => {
//   window.webkit.messageHandlers.native.postMessage(
//     "ON"
//   );
//   pushLine("C ON")
// }, 3000)

function addSlider(
  name,
  identifier,
  defaultValue,
  min = 0,
  max = 1,
  step = 0.01,
) {
  const slider = document.createElement("input");
  slider.type = "range";
  slider.min = min;
  slider.max = max;
  slider.step = step;
  slider.value = defaultValue;
  slider.id = identifier;
  slider.oninput = () => {
    sendParameter(identifier, parseFloat(slider.value));
  };

  const label = document.createElement("label");
  label.innerText = name;

  const div = document.createElement("div");
  div.style.display = "flex";
  div.appendChild(label);
  div.appendChild(slider);
  document.body.appendChild(div);
}

function addNoteButton(label, noteNumber) {
  const button = document.createElement("button");
  button.innerText = label;
  button.onpointerdown = () => {
    sendMessage({
      type: "noteOnRequest",
      noteNumber,
      velocity: 1.0,
    });
  };
  button.onpointerup = () => {
    sendMessage({
      type: "noteOffRequest",
      noteNumber,
    });
  };
  document.body.appendChild(button);
}

addSlider("Gain", "gain", 0.5);
addSlider("Wave", "waveType", 0, 0, 3, 1);
addSlider("Pitch", "oscPitch", 0.5);
addSlider("Volume", "oscVolume", 0.5);
addNoteButton("Note(60)", 60);

subscribe((msg) => {
  if (msg.type === "setParameter") {
    const slider = document.getElementById(msg.identifier);
    if (slider) {
      slider.value = msg.value;
    }
  } else if (msg.type === "bulkSendParameters") {
    for (const [identifier, value] of Object.entries(msg.parameters)) {
      const slider = document.getElementById(identifier);
      if (slider) {
        slider.value = value;
      }
    }
  } else if (msg.type === "hostNoteOn") {
    pushLine(`host note on: ${msg.noteNumber}, ${msg.velocity}`);
  } else if (msg.type === "hostNoteOff") {
    pushLine(`host note off: ${msg.noteNumber}`);
  }
});
