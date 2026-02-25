import path from "path";
import {
	casingToCapital,
	casingToSnake,
	workerHelper_copyProjectContent,
	workerHelper_replaceInFileSignaturesRecursive,
	workerHelper_updateFileNamesRecursive,
} from "../../../common/worker-helper";
import type { WorkerInterface } from "../../../common/worker-types";

export const template_vstSimple_worker: WorkerInterface = {
	async createProject(projectName, templateName) {
		workerHelper_copyProjectContent(projectName, templateName);

		const newFolderPath = path.join(process.cwd(), projectName);

		const projectNameInFilePath = casingToSnake(projectName);

		const projectNameInSignatures = casingToCapital(projectName);

		workerHelper_updateFileNamesRecursive(newFolderPath, {
			filesMatcher: "source/vst/**/*",
			replacements: [
				{ originalPrefix: "project1", newPrefix: projectNameInFilePath },
			],
		});

		workerHelper_replaceInFileSignaturesRecursive(newFolderPath, {
			filesMatcher: "source/vst/**/*",
			replacements: [
				{ originalPrefix: "project1", newPrefix: projectNameInFilePath },
			],
		});

		workerHelper_replaceInFileSignaturesRecursive(newFolderPath, {
			filesMatcher: "source/vst/**/*",
			replacements: [
				{ originalPrefix: "Project1", newPrefix: projectNameInSignatures },
			],
		});
		return true;
	},
};
