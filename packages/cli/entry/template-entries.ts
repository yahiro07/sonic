import type { TemplateEntry } from "@/common/worker-types";
import auv3SwiftXcode from "@/templates/auv3-swift-xcode/__worker";
import vstSimple from "@/templates/vst-simple/__worker";
import cppMultiTarget from "@/workers/cpp-multi-target";

export const templateEntries: TemplateEntry[] = [
  vstSimple,
  cppMultiTarget,
  auv3SwiftXcode,
];
