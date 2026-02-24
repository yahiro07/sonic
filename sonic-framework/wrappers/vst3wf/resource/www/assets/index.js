
const pushLine = (line) => {
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


window.webkit.messageHandlers.native.postMessage(
  JSON.stringify({ data: "A" })
);


setTimeout(() => {
  window.webkit.messageHandlers.native.postMessage(
    JSON.stringify({ data: "B" })
  );
  pushLine("B")
}, 2000)

setTimeout(() => {
  window.webkit.messageHandlers.native.postMessage(
    "ON"
  );
  pushLine("C ON")
}, 3000)

function sendMessage(msg){
  window.webkit.messageHandlers.native.postMessage(
    JSON.stringify({ detail: msg })
  );
}

function sendParameter(id, value){
  sendMessage({
    type: "setParameter",
    id: id,
    value: value
  }) 
}


function addSlider(name, id, defaultValue){
  const slider = document.createElement("input");
  slider.type = "range";
  slider.min = 0;
  slider.max = 1;
  slider.value = defaultValue;
  slider.id = id;
  slider.oninput = () => {
    sendParameter(id, slider.value);
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

addSlider("Gain", 0, 0.5);
addSlider("Pitch", 1, 0.5);
addSlider("Volume", 2, 0.5);

