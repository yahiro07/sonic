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
	fs.mkdirSync(path.join(process.cwd(), projectName), { recursive: true });

	//copy other entities than __worker folder
	const templateEntities = fs.readdirSync(templateFolderPath);
	for (const entity of templateEntities) {
		if (entity !== "__worker") {
			const srcPath = path.join(templateFolderPath, entity);
			const destPath = path.join(process.cwd(), projectName, entity);
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

export function casingToSnake(str: string) {
	return str
		.replace(/([A-Z])/g, "_$1")
		.toLowerCase()
		.replace(/^_/, "");
}

export function casingToCapital(str: string) {
	return str.charAt(0).toUpperCase() + str.slice(1);
}

const lettersSource = {
	alpha: "abcdefghijklmnopqrstuvwxyz",
	alphaNumeric: "abcdefghijklmnopqrstuvwxyz0123456789",
	alphaNumericWithCapital:
		"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
};
export function generateRandomString(
	sourceKind: "alpha" | "alphaNumeric" | "alphaNumericWithCapital",
	len: number,
): string {
	const letters = lettersSource[sourceKind];
	return new Array(len)
		.fill(0)
		.map(() => letters[Math.floor(Math.random() * letters.length)])
		.join("");
}
