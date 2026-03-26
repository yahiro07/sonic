import type { TemplateEntry } from "@/src/common/worker-types";
import auv3SwiftXcode from "@/src/workers/auv3-swift-xcode";
import vstSimple from "@/src/workers/vst-simple";
import cppMultiTarget from "@/src/workers/cpp-multi-target";

export const templateEntries: TemplateEntry[] = [
  vstSimple,
  cppMultiTarget,
  auv3SwiftXcode,
];
