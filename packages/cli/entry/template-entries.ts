import type { TemplateEntry } from "@/common/worker-types";
import auv3SwiftXcode from "@/templates/auv3-swift-xcode/__worker";
import vstSimple from "@/templates/vst-simple/__worker";
import cppMultiTarget from "@/templates/cpp-multi-target/__worker";

export const templateEntries: TemplateEntry[] = [
  vstSimple,
  cppMultiTarget,
  auv3SwiftXcode,
];
