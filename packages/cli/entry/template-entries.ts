import type { TemplateEntry } from "@/common/worker-types";
import auv3SwiftXcode from "@/templates/auv3-swift-xcode/__worker";
import vstSimple from "@/templates/vst-simple/__worker";
import vstFlexible from "@/templates/vst-flexible/__worker";

export const templateEntries: TemplateEntry[] = [
  vstSimple,
  vstFlexible,
  auv3SwiftXcode,
];
