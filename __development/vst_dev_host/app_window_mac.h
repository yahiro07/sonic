#pragma once

#include "app_window_base.h"

class AppWindowMac : public AppWindowBase {
public:
  AppWindowMac();
  ~AppWindowMac();

  void show() override;
  void loop() override;

private:
  void *window_{nullptr};
  void *delegate_{nullptr};
};