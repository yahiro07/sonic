// type MessageFromUi =
//   | { type: "uiLoaded" }
//   | { type: "beginEdit"; paramKey: string }
//   | { type: "performEdit"; paramKey: string; value: number }
//   | { type: "endEdit"; paramKey: string }
//   | { type: "instantEdit"; paramKey: string; value: number }
//   | { type: "noteOnRequest"; noteNumber: number }
//   | { type: "noteOffRequest"; noteNumber: number };

// type MessageFromApp =
//   | { type: "setParameter"; paramKey: string; value: number }
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
    logArea.style.height = "100px";
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
pushLogLine("href: " + location.href);

function addSlider(
  name,
  paramKey,
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
  slider.id = paramKey;
  slider.onpointerdown = () => {
    //pointer capture would needed for more robust handling
    sendMessage({ type: "beginEdit", paramKey });
  };
  slider.oninput = () => {
    sendMessage({
      type: "performEdit",
      paramKey,
      value: parseFloat(slider.value),
    });
  };
  slider.onpointerup = () => {
    sendMessage({ type: "endEdit", paramKey });
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

function addIndicator() {
  const div = document.createElement("div");
  div.id = "indicator";
  document.body.appendChild(div);
}

addSlider("Enabled", "oscEnabled", 1, 0, 1, 1);
addSlider("Wave", "oscWave", 0, 0, 3, 1);
addSlider("Pitch", "oscPitch", 0.5);
addSlider("Volume", "oscVolume", 0.5);
addNoteButton("Note(60)", 60);
addIndicator();

sendMessage({ type: "uiLoaded" });

// sendMessage({
//   type: "setupPollingTelemetries",
//   targetBitFlags: 1,
//   timerIntervalMs: 1000,
// });
// sendMessage({ type: "stopPollingTelemetries" });

window.pluginEditorCallback = (msg) => {
  pushLogLine("⇢ui", msg);
  if (msg.type === "setParameter") {
    const slider = document.getElementById(msg.paramKey);
    if (slider) {
      slider.value = msg.value;
    }
  } else if (msg.type === "bulkSendParameters") {
    for (const [paramKey, value] of Object.entries(msg.parameters)) {
      const slider = document.getElementById(paramKey);
      if (slider) {
        slider.value = value;
      }
    }
  } else if (msg.type === "hostNoteOn") {
    pushLogLine(`host note on: ${msg.noteNumber}, ${msg.velocity}`);
    const indicator = document.getElementById("indicator");
    indicator.innerText = "⭐️";
  } else if (msg.type === "hostNoteOff") {
    pushLogLine(`host note off: ${msg.noteNumber}`);
    const indicator = document.getElementById("indicator");
    indicator.innerText = "";
  }
};
