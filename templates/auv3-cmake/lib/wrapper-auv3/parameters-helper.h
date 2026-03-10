#pragma once

#import "../core/parameter-item.h"
#import <CoreAudioKit/CoreAudioKit.h>

namespace sonic {

static AUParameter *
createAUParameterFromParameterItem(const ParameterItem &entry) {
  AudioUnitParameterOptions paramOptions =
      kAudioUnitParameterFlag_IsWritable | kAudioUnitParameterFlag_IsReadable;
  if (entry.type == ParameterType::Enum) {
    paramOptions |= kAudioUnitParameterFlag_ValuesHaveStrings;
  }
  AUParameter *param = [AUParameterTree
      createParameterWithIdentifier:[NSString
                                        stringWithUTF8String:entry.paramKey
                                                                 .c_str()]
                               name:[NSString stringWithUTF8String:entry.label
                                                                       .c_str()]
                            address:entry.id
                                min:entry.minValue
                                max:entry.maxValue
                               unit:kAudioUnitParameterUnit_Generic
                           unitName:nil
                              flags:paramOptions
                       valueStrings:nil
                dependentParameters:nil];
  param.value = entry.defaultValue;
  return param;
}

static AUParameterTree *createAUParameterTreeFromParameterItems(
    const std::vector<ParameterItem> &items) {
  NSMutableArray *auParams = [NSMutableArray array];
  for (const auto &entry : items) {
    [auParams addObject:createAUParameterFromParameterItem(entry)];
  }
  return [AUParameterTree createTreeWithChildren:auParams];
}

static int getMaxIdFromParameterItems(const std::vector<ParameterItem> &items) {
  int maxId = 0;
  for (const auto &item : items) {
    if (item.id > maxId) {
      maxId = item.id;
    }
  }
  return maxId;
}
} // namespace sonic