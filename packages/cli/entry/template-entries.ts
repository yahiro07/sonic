import type { TemplateEntry } from "@/common/worker-types";
import auv3SwiftXcode from "@/workers/auv3-swift-xcode";
import vstSimple from "@/workers/vst-simple";
import cppMultiTarget from "@/workers/cpp-multi-target";

export const templateEntries: TemplateEntry[] = [
  vstSimple,
  cppMultiTarget,
  auv3SwiftXcode,
];
