import type { TemplateEntry } from "../common/worker-types";
import auSimple from "../templates/au-simple/__worker";
import vstSimple from "../templates/vst-simple/__worker";

export const templateEntries: TemplateEntry[] = [vstSimple, auSimple];
