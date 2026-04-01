const isDebug = location.search.includes("debug=1");
const isDirectLoggingEnabled = location.search.includes("dlog=1");

const loggerOptions = {
  outputToConsole: false,
  sendToWebViewOwner: false,
  sendToLocalHttpLogServer: false,
};
if (isDebug) {
  Object.assign(loggerOptions, {
    outputToConsole: true,
    sendToWebViewOwner: true,
    sendToLocalHttpLogServer: isDirectLoggingEnabled,
  });
}

let postFailed = false;

type LogKind = "log" | "mark" | "warn" | "error" | "mute" | "unmute";

type LogItem = {
  timestamp: number;
  subsystem: string;
  logKind: LogKind;
  message: string;
};

function createLogFormatter() {
  // const subsystemIcons: Record<string, string> = {
  //   host: "🟣",
  //   ext: "🔸",
  //   ui: "🔹",
  //   dsp: "🔺",
  // };

  const logKindIcons: Record<string, string> = {
    log: "",
    mark: "🔽",
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
    // const ssIcon = subsystemIcons[logItem.subsystem ?? ""] ?? "";
    const kindIcon = logKindIcons[logItem.logKind ?? ""] ?? "";
    const ts = formatTimestamp(logItem.timestamp);
    return `${ts} ${kindIcon} ${logItem.message}`;
  }
  return { formatLogLine };
}
const logFormatter = createLogFormatter();

async function loggingViaLocalHttp(logItem: LogItem) {
  if (!postFailed) {
    try {
      await fetch("http://localhost:9002", {
        method: "POST",
        body: JSON.stringify(logItem),
      });
    } catch {
      //skip sending logs after first failure
      console.log(`failed to post local http log`);
      postFailed = true;
    }
  }
}

function sendLogItemToWebViewOwner(logItem: LogItem) {
  const globalThisTyped = globalThis as unknown as {
    webkit?: {
      messageHandlers: {
        pluginEditor?: {
          postMessage: (msg: {
            type: "log";
            timestamp: number;
            logKind: string;
            message: string;
          }) => void;
        };
      };
    };
  };
  globalThisTyped.webkit?.messageHandlers.pluginEditor?.postMessage({
    type: "log",
    timestamp: logItem.timestamp,
    logKind: logItem.logKind,
    message: logItem.message,
  });
}

function writeLogItemToConsole(logItem: LogItem) {
  const logLine = logFormatter.formatLogLine(logItem);
  console.log(logLine);
}

type LogArguments = (
  | string
  | number
  | boolean
  | object
  | Array<string | number | boolean | object>
  | unknown
)[];

function mapLogArgumentsToString(args: LogArguments) {
  return args
    .map((arg) => {
      if (typeof arg === "object") {
        return JSON.stringify(arg);
      }
      return String(arg);
    })
    .join(" ");
}

function createLoggerEntry() {
  function pushLog(kind: LogKind, args: LogArguments) {
    if (
      !loggerOptions.outputToConsole &&
      !loggerOptions.sendToLocalHttpLogServer &&
      !loggerOptions.sendToWebViewOwner
    ) {
      return;
    }
    const msg = mapLogArgumentsToString(args);

    const logItem: LogItem = {
      timestamp: Date.now(),
      subsystem: "ui",
      logKind: kind,
      message: msg,
    };

    if (loggerOptions.outputToConsole) {
      writeLogItemToConsole(logItem);
    }

    if (loggerOptions.sendToLocalHttpLogServer) {
      void loggingViaLocalHttp(logItem);
    }

    if (loggerOptions.sendToWebViewOwner) {
      sendLogItemToWebViewOwner(logItem);
    }
  }

  return {
    log(...args: LogArguments) {
      pushLog("log", args);
    },
    mark(...args: LogArguments) {
      pushLog("mark", args);
    },
    warn(...args: LogArguments) {
      pushLog("warn", args);
    },
    error(...args: LogArguments) {
      pushLog("error", args);
    },
  };
}

export const logger = createLoggerEntry();
