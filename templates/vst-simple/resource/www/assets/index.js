function sendMessage(msg) {
  window.webkit.messageHandlers.pluginEditor.postMessage(
    JSON.stringify(msg)
  );
}

function sendParameter(identifier, value) {
  sendMessage({
    type: "setParameter",
    identifier,
    value
  })
}


function pushLine(line) {
  const div = document.createElement("div");
  div.innerText = line;
  document.body.appendChild(div);
};

pushLine("hello from js 1235");

pushLine("href:" + location.href)



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



function addSlider(name, identifier, defaultValue, min=0, max=1, step=0.01) {
  const slider = document.createElement("input");
  slider.type = "range";
  slider.min = min;
  slider.max = max;
  slider.step = step;
  slider.value = defaultValue;
  slider.id = identifier
  slider.oninput = () => {
    sendParameter(identifier, parseFloat(slider.value));
  };
  // document.body.appendChild(slider);

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
      velocity: 1.0
    });
  };
  button.onpointerup = () => {
    sendMessage({
      type: "noteOffRequest",
      noteNumber
    });
  };
  document.body.appendChild(button);
}


addSlider("Gain", "gain", 0.5);
addSlider("Wave", "waveType", 0, 0, 3, 1);
addSlider("Pitch", "oscPitch", 0.5);
addSlider("Volume", "oscVolume", 0.5);

addNoteButton("Note(60)", 60);


function handleMessage(msg){
  if (msg.type === "setParameter") {
    const slider = document.getElementById(msg.identifier);
    if (slider) {
      slider.value = msg.value;
    }
  }else if (msg.type === "bulkSendParameters") {
    for (const [identifier, value] of Object.entries(msg.parameters)) {
      const slider = document.getElementById(identifier);
      if (slider) {
        slider.value = value;
      }
    }
  }
}

// window.addEventListener("native-message", (event) => {
//   pushLine("rcv");
//   console.log("native-message_received_in_js", event.detail);
//   pushLine(JSON.stringify(event));
//   pushLine(JSON.stringify(event.detail));
//   handleMessage(event.detail);
// });

window.pluginEditorCallback = (msg) => {
  // window.dispatchEvent(new CustomEvent("plugin-editor-message", { detail: msg }));
  pushLine(JSON.stringify(msg));
  handleMessage(msg);
}

// window.addEventListener("plugin-editor-message", (event) => {
//   pushLine("plugin-editor-message");
// });
