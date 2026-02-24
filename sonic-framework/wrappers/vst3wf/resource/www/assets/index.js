function sendMessage(msg) {
  window.webkit.messageHandlers.native.postMessage(
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

window.addEventListener("native-message", (event) => {
  pushLine("rcv");
  console.log("native-message_received_in_js", event.detail);
  pushLine(JSON.stringify(event.detail));
});

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



function addSlider(name, identifier, defaultValue) {
  const slider = document.createElement("input");
  slider.type = "range";
  slider.min = 0;
  slider.max = 1;
  slider.step = 0.01;
  slider.value = defaultValue;
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

addSlider("Gain", "gain", 0.5);
addSlider("Pitch", "oscPitch", 0.5);
addSlider("Volume", "oscVolume", 0.5);

