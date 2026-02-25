export type WorkerInterface = {
	createProject(projectName: string, templateName: string): Promise<boolean>;
};
