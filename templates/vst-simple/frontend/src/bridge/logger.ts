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

async function loggingViaLocalHttp(msg: string) {
  if (!postFailed) {
    try {
      await fetch("http://localhost:9003", {
        method: "POST",
        body: msg,
      });
    } catch {
      //skip sending logs after first failure
      console.log(`failed to post local http log`);
      postFailed = true;
    }
  }
}

type LogItem = {
  timeStamp: number;
  kind: string;
  message: string;
};

function sendLogItemToWebViewOwner(logItem: LogItem) {
  const globalThisTyped = globalThis as unknown as {
    webkit?: {
      messageHandlers: {
        pluginEditor: {
          postMessage: (msg: {
            type: "log";
            timeStamp: number;
            kind: string;
            message: string;
          }) => void;
        };
      };
    };
  };
  globalThisTyped.webkit?.messageHandlers.pluginEditor.postMessage({
    type: "log",
    timeStamp: logItem.timeStamp,
    kind: logItem.kind,
    message: logItem.message,
  });
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
  function pushLog(kind: string, args: LogArguments) {
    if (
      !loggerOptions.outputToConsole &&
      !loggerOptions.sendToLocalHttpLogServer &&
      !loggerOptions.sendToWebViewOwner
    ) {
      return;
    }
    const msg = mapLogArgumentsToString(args);

    if (loggerOptions.outputToConsole) {
      console.log(msg);
    }

    if (loggerOptions.sendToLocalHttpLogServer) {
      void loggingViaLocalHttp(`(t:${Date.now()}, s:ui, k:${kind}) ${msg}`);
    }

    if (loggerOptions.sendToWebViewOwner) {
      sendLogItemToWebViewOwner({
        timeStamp: Date.now(),
        kind: kind,
        message: msg,
      });
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
