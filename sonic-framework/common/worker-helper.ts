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
