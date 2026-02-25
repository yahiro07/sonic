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

export function workerHelper_updateFileNamesWithPrefix(
	folderPath: string,
	spec: {
		filePaths: string[];
		originalPrefix: string;
		newPrefix: string;
	},
) {}

// export function workerHelper_replaceInFileSignaturesWithPrefix(
// 	folderPath: string,
// 	spec: {
// 		filePaths: string[];
// 		replacements: {
// 			originalPrefix: string;
// 			newPrefix: string;
// 		}[];
// 	},
// ) {}

export function workerHelper_replaceStrings(
	folderPath: string,
	spec: {
		filePaths: string[];
		replacements: { from: string; to: string }[];
	},
) {}

export function casingToSnake(str: string) {
	return str.replace(/([A-Z])/g, "_$1").toLowerCase();
}

export function casingToCapital(str: string) {
	return str.charAt(0).toUpperCase() + str.slice(1);
}
