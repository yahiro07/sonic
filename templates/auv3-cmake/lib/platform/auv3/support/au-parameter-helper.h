#pragma once

#import "../../../core/parameter-spec-item.h"
#import <CoreAudioKit/CoreAudioKit.h>

namespace sonic {

static AUParameter *
createAUParameterFromParameterItem(const ParameterSpecItem &entry) {
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

static AUParameterTree *
createAUParameterTreeFromParameterItems(const ParameterSpecArray &items) {
  NSMutableArray *auParams = [NSMutableArray array];
  for (const auto &entry : items) {
    [auParams addObject:createAUParameterFromParameterItem(entry)];
  }
  return [AUParameterTree createTreeWithChildren:auParams];
}

} // namespace sonic