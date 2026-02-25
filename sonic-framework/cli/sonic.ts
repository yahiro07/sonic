#!/usr/bin/env node
import fs from "fs";
import path, { dirname } from "path";
import { fileURLToPath } from "url";

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

const supportedTemplateNames = ["vst-simple"];

function createProject(projectName: string, templateName: string) {
	console.log(`creating project ${projectName} with template ${templateName}`);
	if (!supportedTemplateNames.includes(templateName)) {
		console.log(`incompatible template name: ${templateName}`);
		console.log("supported template names:", supportedTemplateNames);
		return;
	}
	const __filename = fileURLToPath(import.meta.url);
	const __dirname = dirname(__filename);
	// console.log("__dirname: ", __dirname);
	// console.log("cwd: ", process.cwd());

	const templateFolderPath = path.join(__dirname, "../templates", templateName);
	// console.log("templateFolderPath: ", templateFolderPath);

	const templateContentsFolderPath = path.join(templateFolderPath, "contents");

	const newProjectFolderPath = path.join(process.cwd(), projectName);
	// console.log("templateContentsFolderPath: ", templateContentsFolderPath);
	// console.log("targetFolderPath: ", newProjectFolderPath);

	fs.cpSync(templateContentsFolderPath, newProjectFolderPath, {
		recursive: true,
	});
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
