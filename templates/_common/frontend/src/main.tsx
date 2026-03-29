import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import "./index.css";
import { App } from "@/App.tsx";
import { logger } from "@/bridge/logger";
import { UiPresenterProvider } from "@/presenter/ui-presenter-context";

function start() {
  logger.info("frontend start");
  logger.log(`at ${location.href}`);
  createRoot(document.getElementById("root")!).render(
    <StrictMode>
      <UiPresenterProvider>
        <App />
      </UiPresenterProvider>
    </StrictMode>,
  );
}

start();
