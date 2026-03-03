export type TemplateWorker = {
  createProject(
    projectName: string,
    templateName: string,
  ): Promise<boolean | "cancelled">;
};

export type TemplateEntry = {
  name: string;
  description: string;
  worker: TemplateWorker;
};
