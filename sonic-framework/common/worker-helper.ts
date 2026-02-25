import fs from "fs";
import path, { dirname } from "path";
import { fileURLToPath } from "url";

export function workerHelper_copyProjectContent(
	projectName: string,
	templateName: string,
) {
	const binFolderPath = dirname(fileURLToPath(import.meta.url));
	const templateContentsFolderPath = path.join(
		binFolderPath,
		"../",
		"templates",
		templateName,
		"contents",
	);

	const newProjectFolderPath = path.join(process.cwd(), projectName);
	console.log("templateContentsFolderPath: ", templateContentsFolderPath);
	console.log("targetFolderPath: ", newProjectFolderPath);

	fs.cpSync(templateContentsFolderPath, newProjectFolderPath, {
		recursive: true,
	});
}

export function workerHelper_updateFileNamesRecursive(
	folderPath: string,
	spec: {
		// extensions?: string[];
		filesMatcher?: string;
		replacements: {
			// forwardChars?: string[];
			originalPrefix: string;
			newPrefix: string;
		}[];
	},
) {}

export function workerHelper_replaceInFileSignaturesRecursive(
	folderPath: string,
	spec: {
		// extensions?: string[];
		filesMatcher?: string;
		replacements: {
			// forwardChars?: string[];
			originalPrefix: string;
			newPrefix: string;
		}[];
	},
) {}

export function casingToSnake(str: string) {
	return str.replace(/([A-Z])/g, "_$1").toLowerCase();
}

export function casingToCapital(str: string) {
	return str.replace(/([a-z])/g, "_$1").toLowerCase();
}
