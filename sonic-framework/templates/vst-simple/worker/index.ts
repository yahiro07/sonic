import { workerHelper_copyProjectContent } from "../../../common/worker-helper";
import type { WorkerInterface } from "../../../common/worker-types";

export const template_vstSimple_worker: WorkerInterface = {
	async createProject(projectName, templateName) {
		workerHelper_copyProjectContent(projectName, templateName);
		return true;
	},
};
