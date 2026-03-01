#pragma once

#include "app_window_base.h"
#include <memory>

class AppWindowMac : public AppWindowBase {
public:
  AppWindowMac();
  ~AppWindowMac();

  void show() override;
  void loop() override;

private:
  struct InternalStates;
  std::unique_ptr<InternalStates> states;
};