import * as dgram from "node:dgram";
import * as http from "node:http";
import { TextDecoder } from "node:util";

function readPort(envValue: string | undefined, fallback: number): number {
  const parsed = Number.parseInt(envValue ?? "", 10);
  return Number.isFinite(parsed) ? parsed : fallback;
}

const configs = {
  udpPortForApp: readPort(process.env.UDP_PORT_FOR_APP, 9001),
  httpPortForUi: readPort(process.env.HTTP_PORT_FOR_UI, 9002),
};

type LogKind = "log" | "mark" | "warn" | "error" | "mute" | "unmute";

type LogItem = {
  timestamp: number; //ms from epoch
  subsystem: string;
  logKind: LogKind;
  message: string;
  subOrdering: number;
};

type LogItemEx = LogItem & {
  subOrdering: number;
};

function createLogFormatter() {
  const subsystemIcons: Record<string, string> = {
    host: "🟣",
    ext: "🔸",
    ui: "🔹",
    dsp: "🔺",
  };

  const logKindIcons: Record<string, string> = {
    trace: "🔽",
    info: "◻️",
    log: "▫️",
    warn: "⚠️",
    error: "📛",
  };

  function formatTimestamp(timestamp: number) {
    const date = new Date(timestamp);
    const hours = String(date.getHours()).padStart(2, "0");
    const minutes = String(date.getMinutes()).padStart(2, "0");
    const seconds = String(date.getSeconds()).padStart(2, "0");
    const milliseconds = String(date.getMilliseconds()).padStart(3, "0");
    //00:00:00.000
    return `${hours}:${minutes}:${seconds}.${milliseconds}`;
  }

  function formatLogLine(logItem: LogItem): string {
    const ssIcon = subsystemIcons[logItem.subsystem ?? ""] ?? "";
    const kindIcon = logKindIcons[logItem.logKind ?? ""] ?? "";
    const ts = formatTimestamp(logItem.timestamp);
    return `${ts} [${ssIcon}${logItem.subsystem}] ${kindIcon} ${logItem.message}`;
  }
  return { formatLogLine };
}
const logFormatter = createLogFormatter();

//messages are shown with a little delay and sorted by timestamp
//since http messages from UI are received asynchronously, their order might not be strictly chronological

function createLoggerCore() {
  const logItems: LogItemEx[] = [];
  let timerId: NodeJS.Timeout | undefined;
  let isMuted = false;
  let subOrderingCounter = 0;

  function consumeLogItem(logItem: LogItem) {
    if (logItem.logKind === "mute") {
      isMuted = true;
    } else if (logItem.logKind === "unmute") {
      isMuted = false;
    } else if (logItem.message) {
      if (!isMuted) {
        const logLine = logFormatter.formatLogLine(logItem);
        console.log(logLine);
      }
    }
  }

  function consumeLogItems() {
    logItems.sort(
      (a, b) => a.timestamp - b.timestamp || a.subOrdering - b.subOrdering,
    );
    const curr = Date.now();
    while (logItems.length > 0) {
      const item = logItems[0];
      if (item.timestamp <= curr) {
        logItems.shift();
        consumeLogItem(item);
      } else {
        break;
      }
    }
  }

  function pushLogItem(logItem: LogItem) {
    logItems.push(logItem);
    if (timerId) {
      clearTimeout(timerId);
    }
    timerId = setTimeout(consumeLogItems, 10);
  }

  return {
    log(jsonString: string) {
      try {
        const logItem = JSON.parse(jsonString) as LogItem;
        // console.log({ logItem });
        pushLogItem({
          ...logItem,
          subOrdering: subOrderingCounter++,
        });
      } catch {
        console.warn("Failed to parse log item:", jsonString);
      }
    },
  };
}
const loggerCore = createLoggerCore();

async function setupUdpServerForApp() {
  const socket = dgram.createSocket("udp4");
  const decoder = new TextDecoder();

  socket.on("error", (err) => {
    console.error("UDP(app) socket error:", err);
  });

  socket.on("message", (data) => {
    const msg = decoder.decode(data);
    loggerCore.log(msg);
  });

  await new Promise<void>((resolve, reject) => {
    socket.once("error", reject);
    socket.bind(configs.udpPortForApp, () => resolve());
  });

  return socket;
}

async function setupHttpServerForUi() {
  const headers = {
    "Access-Control-Allow-Origin": "*",
    "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
    "Access-Control-Allow-Headers": "Content-Type",
  };

  const server = http.createServer((req, res) => {
    for (const [k, v] of Object.entries(headers)) {
      res.setHeader(k, v);
    }

    if (req.method === "OPTIONS") {
      res.statusCode = 204;
      res.end();
      return;
    }

    if (req.method === "POST") {
      const chunks: Buffer[] = [];
      req.on("data", (chunk) =>
        chunks.push(Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk)),
      );
      req.on("end", () => {
        const msg = Buffer.concat(chunks).toString("utf8");
        loggerCore.log(msg);
        res.statusCode = 200;
        res.end("ok");
      });
      return;
    }

    res.statusCode = 200;
    res.end("log server");
  });

  server.on("error", (err) => {
    console.error("HTTP server error:", err);
  });

  await new Promise<void>((resolve, reject) => {
    server.once("error", reject);
    server.listen(configs.httpPortForUi, () => resolve());
  });
  return server;
}

async function start() {
  console.log("log server");

  try {
    const udpApp = await setupUdpServerForApp();
    const httpServer = await setupHttpServerForUi();

    const shutdown = () => {
      udpApp.close();
      httpServer.close();
    };

    process.once("SIGINT", shutdown);
    process.once("SIGTERM", shutdown);

    console.log(
      `Log server running:
    UDP_PORT_FOR_APP : ${configs.udpPortForApp}
    HTTP_PORT_FOR_UI : ${configs.httpPortForUi}
  `,
    );
  } catch (err) {
    console.error("Failed to start log server:", err);
    process.exitCode = 1;
  }
}

void start();
