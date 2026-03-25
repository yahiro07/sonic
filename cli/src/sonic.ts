#!/usr/bin/env node
import * as clackPrompts from "@clack/prompts";
import fs from "fs";
import path from "path";
import { casingToCapital, incrementSuffix } from "@/src/common";
import { templateEntries } from "./workers/template-entries";
import { appEnvs } from "@/src/base/app-envs";

type InputCommand =
  | { type: "create" }
  | { type: "help" }
  | { type: "version" }
  | { type: "cancelled" };

async function chooseOperation(): Promise<InputCommand> {
  const operation = await clackPrompts.select({
    message: "Select an operation",
    options: [
      { value: "create", hint: "Create a new project" },
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
  } else if (len == 1 && (firstArg === "version" || firstArg === "--version")) {
    return { type: "version" };
  } else if (len == 1 && (firstArg === "help" || firstArg === "--help")) {
    return { type: "help" };
  }
  return undefined;
}

async function handleInputCommand(inputCommand: InputCommand | undefined) {
  if (!inputCommand) {
    console.log("incompatible command.");
    console.log("invocation should be one of:");
    console.log("sonic");
    console.log("sonic create");
    console.log("sonic --version");
    console.log("sonic --help");
  } else if (inputCommand.type === "version") {
    console.log("sonic cli version 0.0.0-in-development");
  } else if (inputCommand.type === "help") {
    console.log("supported commands:");
    console.log("sonic");
    console.log("sonic create");
    console.log("sonic --version");
    console.log("sonic --help");
  } else if (inputCommand.type === "create") {
    await createProject();
  } else if (inputCommand.type === "cancelled") {
    console.log("operation cancelled.");
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
    console.log({ newProjectFolderPath });
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
