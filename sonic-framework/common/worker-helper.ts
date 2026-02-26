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

export function workerHelper_copyProjectContent_excludingWorkerFolder(
	projectName: string,
	templateName: string,
) {
	const binFolderPath = dirname(fileURLToPath(import.meta.url));
	const templateFolderPath = path.join(
		binFolderPath,
		"../",
		"templates",
		templateName,
	);
	const newProjectFolderPath = path.join(process.cwd(), projectName);
	fs.mkdirSync(newProjectFolderPath, { recursive: true });

	//copy other entities than __worker folder
	const templateEntities = fs.readdirSync(templateFolderPath);
	for (const entity of templateEntities) {
		if (entity !== "__worker") {
			const srcPath = path.join(templateFolderPath, entity);
			const destPath = path.join(newProjectFolderPath, entity);
			if (fs.statSync(srcPath).isDirectory()) {
				fs.cpSync(srcPath, destPath, {
					recursive: true,
				});
			} else {
				fs.copyFileSync(srcPath, destPath);
			}
		}
	}
}

export function workerHelper_updateFileNamesWithPrefix(
	folderPath: string,
	spec: {
		filePaths: string[];
		originalPrefix: string;
		newPrefix: string;
	},
) {
	const fullPaths = spec.filePaths.map((filePath) =>
		path.join(folderPath, filePath),
	);
	for (const filePath of fullPaths) {
		const fileName = path.basename(filePath);
		if (!fileName.startsWith(spec.originalPrefix)) continue;
		if (!fs.existsSync(filePath)) continue;
		const newFileName = fileName.replace(
			new RegExp(spec.originalPrefix, "g"),
			spec.newPrefix,
		);
		const newFilePath = path.join(path.dirname(filePath), newFileName);
		fs.renameSync(filePath, newFilePath);
	}
}

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
) {
	const fullPaths = spec.filePaths.map((filePath) =>
		path.join(folderPath, filePath),
	);
	for (const filePath of fullPaths) {
		if (!fs.existsSync(filePath)) continue;
		let fileContent = fs.readFileSync(filePath, "utf-8");
		let replaced = false;
		for (const replacement of spec.replacements) {
			fileContent = fileContent.replace(
				new RegExp(replacement.from, "g"),
				replacement.to,
			);
			replaced = true;
		}
		if (replaced) {
			fs.writeFileSync(filePath, fileContent);
		}
	}
}
