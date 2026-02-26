import type { WorkerInterface } from "../common/worker-types";
import auSimpleWorker from "../templates/au-simple/__worker";
import vstSimpleWorker from "../templates/vst-simple/__worker";

export const templateWorkers: Record<string, WorkerInterface> = {
	"au-simple": auSimpleWorker,
	"vst-simple": vstSimpleWorker,
};
