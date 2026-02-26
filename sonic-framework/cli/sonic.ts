#!/usr/bin/env node
import * as clackPrompts from "@clack/prompts";
import fs from "fs";
import path from "path";
import { casingToCapital, incrementSuffix } from "../common";
import { templateEntries } from "./template-entries";

type InputCommand =
	| { type: "create" }
	| { type: "createIncompatibleArgs" }
	| { type: "help" }
	| { type: "version" };

function parseArgs(args: string[]): InputCommand | undefined {
	if (args[0] === "create") {
		return { type: "create" };
	} else if (args[0] === "--version") {
		return { type: "version" };
	} else if (args[0] === "--help") {
		return { type: "help" };
	}
	return undefined;
}

function handleInputCommand(inputCommand: InputCommand | undefined) {
	if (!inputCommand) {
		console.log("incompatible command.");
		console.log("command should be one of:");
		console.log("sonic create");
		console.log("sonic --version");
		console.log("sonic --help");
	} else if (inputCommand.type === "version") {
		console.log("sonic cli version 0.0.0-in-development");
	} else if (inputCommand.type === "help") {
		console.log("supported commands:");
		console.log("sonic create");
		console.log("sonic --version");
		console.log("sonic --help");
	} else if (inputCommand.type === "createIncompatibleArgs") {
		console.log("incompatible arguments");
		console.log("please use: sonic create");
	} else if (inputCommand.type === "create") {
		createProject();
	}
}

async function createProject() {
	try {
		const templateName = await clackPrompts.select({
			message: "Select a template",
			options: templateEntries.map((entry) => ({
				value: entry.name,
				hint: entry.description,
			})),
		});
		if (clackPrompts.isCancel(templateName)) return;

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
		if (clackPrompts.isCancel(projectName)) return;

		if (projectName === "") {
			projectName = defaultProjectName;
		}

		const newProjectFolderPath = path.join(process.cwd(), projectName);
		if (fs.existsSync(newProjectFolderPath)) {
			const ok = await clackPrompts.confirm({
				message: `Project ${projectName} already exists. Do you want to overwrite it?`,
				initialValue: false,
			});
			if (!ok) {
				console.log("operation cancelled.");
				return;
			}
			fs.rmSync(newProjectFolderPath, { recursive: true });
		}

		const templateEntry = templateEntries.find(
			(entry) => entry.name === templateName,
		);
		if (!templateEntry) {
			throw new Error(`incompatible template name: ${templateName}`);
		}
		const ok = await templateEntry.worker.createProject(
			projectName,
			templateName,
		);
		if (!ok) {
			console.log(`failed to create project ${projectName}`);
			return;
		}
		console.log(`project ${projectName} created.`);
	} catch (error) {
		console.error(error);
	}
}

function run() {
	console.log("Sonic Framework CLI");
	const args = process.argv.slice(2);
	const inputCommand = parseArgs(args);
	handleInputCommand(inputCommand);
}

run();
