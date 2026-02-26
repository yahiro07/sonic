#!/usr/bin/env node
import { templateWorkers } from "./template-workers";

type InputCommand =
	| { type: "create"; projectName: string; templateName: string }
	| { type: "createIncompatibleArgs" }
	| { type: "help" }
	| { type: "version" };

function parseArgs(args: string[]): InputCommand | undefined {
	if (args[0] === "create") {
		const projectName = args[1];
		if (args[2] === "-t" || args[2] === "--template") {
			const templateName = args[3];
			if (templateName) {
				return {
					type: "create",
					projectName,
					templateName,
				};
			}
		}
		return { type: "createIncompatibleArgs" };
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
		console.log("sonic create <project-name> -t <template-name>");
		console.log("sonic --version");
		console.log("sonic --help");
	} else if (inputCommand.type === "version") {
		console.log("sonic cli version 0.0.0-in-development");
	} else if (inputCommand.type === "help") {
		console.log("supported commands:");
		console.log("sonic create <project-name> -t <template-name>");
		console.log("sonic --version");
		console.log("sonic --help");
	} else if (inputCommand.type === "createIncompatibleArgs") {
		console.log("incompatible arguments");
		console.log("please use: sonic create <project-name> -t <template-name>");
	} else if (inputCommand.type === "create") {
		createProject(inputCommand.projectName, inputCommand.templateName);
	}
}

async function createProject(projectName: string, templateName: string) {
	console.log(`creating project ${projectName} with template ${templateName}`);
	const worker = templateWorkers[templateName];
	if (!worker) {
		console.log(`incompatible template name: ${templateName}`);
		console.log("supported template names:", Object.keys(templateWorkers));
		return;
	}
	const ok = await worker.createProject(projectName, templateName);
	if (!ok) {
		console.log(`failed to create project ${projectName}`);
		return;
	}
	console.log(`project ${projectName} created`);
}

function run() {
	console.log("Sonic Framework CLI");
	const args = process.argv.slice(2);
	// console.log("Running with arguments:", args);
	const inputCommand = parseArgs(args);
	handleInputCommand(inputCommand);
}

run();
