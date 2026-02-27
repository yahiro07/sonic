import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import "./index.css";
import { App } from "@/App.tsx";
import { logger } from "@/bridge/logger";
import { coreBridge } from "@/bridge/core-bridge";

function start() {
  logger.mark("frontend start");
  logger.log(`at ${location.href}`);
  createRoot(document.getElementById("root")!).render(
    <StrictMode>
      <App />
    </StrictMode>,
  );
  coreBridge.sendMessage({ type: "uiLoaded" });
}

start();
