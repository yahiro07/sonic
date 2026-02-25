#pragma once

#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/vst/vsttypes.h>
#include <string>

static bool isHexChar(char c) {
  const auto uc = static_cast<unsigned char>(c);
  return std::isxdigit(uc) != 0;
}

static std::string trimAscii(std::string s) {
  auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&](char ch) {
            return !isSpace(static_cast<unsigned char>(ch));
          }));
  s.erase(std::find_if(
              s.rbegin(), s.rend(),
              [&](char ch) { return !isSpace(static_cast<unsigned char>(ch)); })
              .base(),
          s.end());
  return s;
}

static void loadTUIDFromGUIDString(Steinberg::TUID tuid,
                                   const std::string &guidString) {
  // TUID is a plain 16-byte array (typedef char TUID[16])
  std::memset(tuid, 0, 16);

  auto s = trimAscii(guidString);
  if (s.empty())
    return;

  // Accept optional braces.
  if (!s.empty() && s.front() == '{')
    s.erase(s.begin());
  if (!s.empty() && s.back() == '}')
    s.pop_back();

  Steinberg::FUID uid;

  const auto hasHyphen = (s.find('-') != std::string::npos);
  if (hasHyphen) {
    // Expect 8-4-4-4-12 = 36 chars with hyphens.
    if (s.size() != 36 || s[8] != '-' || s[13] != '-' || s[18] != '-' ||
        s[23] != '-')
      return;
    for (size_t i = 0; i < s.size(); ++i) {
      if (i == 8 || i == 13 || i == 18 || i == 23)
        continue;
      if (!isHexChar(s[i]))
        return;
    }

    const std::string registry = "{" + s + "}";
    if (!uid.fromRegistryString(registry.c_str()))
      return;
  } else {
    // Expect 32 hex characters (no separators)
    if (s.size() != 32)
      return;
    for (const auto ch : s) {
      if (!isHexChar(ch))
        return;
    }
    if (!uid.fromString(s.c_str()))
      return;
  }

  uid.toTUID(tuid);
}