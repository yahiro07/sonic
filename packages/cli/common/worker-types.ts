export type TemplateWorker = {
	createProject(projectName: string, templateName: string): Promise<boolean>;
};

export type TemplateEntry = {
	name: string;
	description: string;
	worker: TemplateWorker;
};
