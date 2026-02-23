
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