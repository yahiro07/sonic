#!/usr/bin/env node
import * as clackPrompts from "@clack/prompts";
import fs from "fs";
import path from "path";
import { casingToCapital, incrementSuffix } from "@/src/common";
import { templateEntries } from "./workers/template-entries";
import { appEnvs, appEnvs_getPackageRootFolderPath } from "@/src/base/app-envs";
import childProcess from "child_process";

type InputCommand =
  | { type: "create" }
  | { type: "logger" }
  | { type: "help" }
  | { type: "version" }
  | { type: "cancelled" };

async function chooseOperation(): Promise<InputCommand> {
  const operation = await clackPrompts.select({
    message: "Select an operation",
    options: [
      { value: "create", hint: "Create a new project" },
      { value: "logger", hint: "Start local log server" },
      { value: "help", hint: "Show help" },
      { value: "version", hint: "Show version" },
      { value: "cancelled", label: "cancel", hint: "Cancel operation" },
    ],
  });
  if (clackPrompts.isCancel(operation)) {
    return { type: "cancelled" };
  }
  return { type: operation };
}

async function parseArgs(args: string[]): Promise<InputCommand | undefined> {
  const len = args.length;
  const firstArg = args[0];
  if (len === 0) {
    return await chooseOperation();
  } else if (len === 1 && firstArg === "create") {
    return { type: "create" };
  } else if (len === 1 && firstArg === "logger") {
    return { type: "logger" };
  } else if (len == 1 && (firstArg === "version" || firstArg === "--version")) {
    return { type: "version" };
  } else if (len == 1 && (firstArg === "help" || firstArg === "--help")) {
    return { type: "help" };
  }
  return undefined;
}

function showCommandsAvailable() {
  let command = 'sonic';
  if (process.env._ && process.env._.includes('npx')) {
    command = 'npx sonic-shell';
  }
  console.log("supported commands:");
  console.log(command);
  console.log(`${command} create`);
  console.log(`${command} logger`);
  console.log(`${command} help`);
  console.log(`${command} version`);
}

async function handleInputCommand(inputCommand: InputCommand | undefined) {
  if (!inputCommand) {
    console.log("incompatible command.");
  } else if (inputCommand.type === "version") {
    try {
      const rootPath = appEnvs_getPackageRootFolderPath();
      const packageJsonPath = path.join(rootPath, "package.json");
      const pkgJson = JSON.parse(fs.readFileSync(packageJsonPath, "utf-8"));
      console.log(`sonic cli version ${pkgJson.version}`);
    } catch (e) {
      console.log("sonic cli version (failed to read package.json)");
    }
  } else if (inputCommand.type === "help") {
    showCommandsAvailable();
  } else if (inputCommand.type === "create") {
    await createProject();
  } else if (inputCommand.type === "cancelled") {
    console.log("operation cancelled.");
  } else if (inputCommand.type === "logger") {
    const rootPath = appEnvs_getPackageRootFolderPath();
    const logServerJsPath = path.join(rootPath, "log-server.js");
    if (!fs.existsSync(logServerJsPath)) {
      console.error("log-server.js not found.");
      return;
    }
    const logServerProcess = childProcess.spawn("node", [logServerJsPath], {
      stdio: "inherit",
    });
    logServerProcess.on("close", (code) => {
      console.log(`log server exited with code ${code}`);
    });
  }

}

type BaseOptions = {
  projectName: string;
  templateName: string;
};
async function readBaseOptions(): Promise<BaseOptions | "cancelled"> {
  const templateName = await clackPrompts.select({
    message: "Select a template",
    options: templateEntries.map((entry) => ({
      value: entry.name,
      hint: entry.description,
    })),
  });
  if (clackPrompts.isCancel(templateName)) return "cancelled";

  let defaultProjectName = `${casingToCapital(templateName)}1`;

  let retryCount = 0;
  while (fs.existsSync(path.join(process.cwd(), defaultProjectName))) {
    defaultProjectName = incrementSuffix(defaultProjectName);
    retryCount++;
    if (retryCount > 100) {
      throw new Error("too many default project name retries");
    }
  }

  let projectName = await clackPrompts.text({
    message: "Enter project name",
    placeholder: defaultProjectName,
  });
  if (clackPrompts.isCancel(projectName)) return "cancelled";

  if (projectName === "") {
    projectName = defaultProjectName;
  }
  const newProjectFolderPath = path.join(process.cwd(), projectName);
  if (fs.existsSync(newProjectFolderPath)) {
    const res = await clackPrompts.confirm({
      message: `Project ${projectName} already exists. Do you want to replace it?`,
      initialValue: false,
    });
    if (!res || clackPrompts.isCancel(res)) {
      return "cancelled";
    }
  }
  return { projectName, templateName };
}

async function readBaseOptionsForDebug(): Promise<BaseOptions | "cancelled"> {
  const templateName = await clackPrompts.select({
    message: "Select a template",
    options: templateEntries.map((entry) => ({
      value: entry.name,
      hint: entry.description,
    })),
  });
  if (clackPrompts.isCancel(templateName)) return "cancelled";
  //use the same default project name for debugging
  const projectNameDefault = `${casingToCapital(templateName)}1`;
  let projectName = await clackPrompts.text({
    message: "Enter project name",
    initialValue: projectNameDefault,
  });
  if (clackPrompts.isCancel(projectName)) return "cancelled";
  return { projectName, templateName };
}

async function createProject() {
  try {
    const baseOptions = appEnvs.isWorkerDev
      ? await readBaseOptionsForDebug()
      : await readBaseOptions();
    if (baseOptions === "cancelled") {
      console.log("operation cancelled.");
      return;
    }
    const { projectName, templateName } = baseOptions;

    const newProjectFolderPath = path.join(process.cwd(), projectName);
    if (fs.existsSync(newProjectFolderPath)) {
      fs.rmSync(newProjectFolderPath, { recursive: true });
    }

    const templateEntry = templateEntries.find(
      (entry) => entry.name === templateName,
    );
    if (!templateEntry) {
      throw new Error(`incompatible template name: ${templateName}`);
    }

    try {
      const result = await templateEntry.worker.createProject(
        projectName,
        templateName,
      );
      if (result === true) {
        console.log(`project ${projectName} created.`);
      }
      if (result === false) {
        console.log(`failed to create project ${projectName}`);
        return;
      } else if (result === "cancelled") {
        console.log("operation cancelled.");
        return;
      }
    } catch (error) {
      if (appEnvs.isWorkerDev) {
        //keep incomplete project folder for debugging
      } else {
        //cleanup incomplete project folder
        fs.rmSync(newProjectFolderPath, { recursive: true });
      }
      throw error;
    }
  } catch (error) {
    console.error(error);
  }
}

async function run() {
  console.log("Sonic Framework CLI");
  const args = process.argv.slice(2);
  const inputCommand = await parseArgs(args);
  await handleInputCommand(inputCommand);
}

run();
