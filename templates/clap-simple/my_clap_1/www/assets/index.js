// type MessageFromUi =
//   | { type: "uiLoaded" }
//   | { type: "beginEdit"; identifier: string }
//   | { type: "performEdit"; identifier: string; value: number }
//   | { type: "endEdit"; identifier: string }
//   | { type: "instantEdit"; identifier: string; value: number }
//   | { type: "noteOnRequest"; noteNumber: number }
//   | { type: "noteOffRequest"; noteNumber: number };

// type MessageFromApp =
//   | { type: "setParameter"; identifier: string; value: number }
//   | { type: "bulkSendParameters"; parameters: Record<string, number> }
//   | { type: "hostNoteOn"; noteNumber: number }
//   | { type: "hostNoteOff"; noteNumber: number };

function pushLogLine(...args) {
  let logArea = document.getElementById("logArea");
  if (!logArea) {
    logArea = document.createElement("div");
    logArea.id = "logArea";
    logArea.style.backgroundColor = "#0004";
    logArea.style.padding = "10px";
    logArea.style.maxHeight = "200px";
    logArea.style.overflowY = "scroll";
    document.body.appendChild(logArea);
  }

  const lineDiv = document.createElement("div");
  lineDiv.innerText = args
    .map((it) => (typeof it === "object" ? JSON.stringify(it) : it))
    .join(" ");
  logArea.appendChild(lineDiv);
  logArea.scrollTop = logArea.scrollHeight;
}

if (!window.webkit) {
  pushLogLine("incompatible environment, window.webkit is not available");
}

function sendMessage(msg) {
  if (window.webkit) {
    pushLogLine("⇠ui", msg);
    window.webkit.messageHandlers.pluginEditor.postMessage(msg);
  }
}

pushLogLine("hello from js");
pushLogLine("href:" + location.href);

sendMessage({ type: "uiLoaded" });

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
  slider.onpointerdown = () => {
    //pointer capture would needed for more robust handling
    sendMessage({ type: "beginEdit", identifier });
  };
  slider.oninput = () => {
    sendMessage({
      type: "performEdit",
      identifier,
      value: parseFloat(slider.value),
    });
  };
  slider.onpointerup = () => {
    sendMessage({ type: "endEdit", identifier });
  };

  const label = document.createElement("label");
  label.innerText = name;

  const div = document.createElement("div");
  div.style.display = "flex";
  div.appendChild(label);
  div.appendChild(slider);
  document.body.insertBefore(div, document.body.firstChild);
}

function addNoteButton(label, noteNumber) {
  const button = document.createElement("button");
  button.innerText = label;
  button.onpointerdown = () => {
    sendMessage({
      type: "noteOnRequest",
      noteNumber,
    });
  };
  button.onpointerup = () => {
    sendMessage({
      type: "noteOffRequest",
      noteNumber,
    });
  };
  document.body.insertBefore(button, document.body.firstChild);
}

addNoteButton("Note(60)", 60);
addSlider("Gain", "gain", 0.5);
addSlider("Wave", "waveType", 0, 0, 3, 1);
addSlider("Pitch", "oscPitch", 0.5);
addSlider("Volume", "oscVolume", 0.5);

window.pluginEditorCallback = (msg) => {
  pushLogLine("⇢ui", msg);
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
};
