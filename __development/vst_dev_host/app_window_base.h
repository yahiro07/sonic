class AppWindowBase {
public:
  virtual ~AppWindowBase() = default;
  virtual void show() = 0;
  virtual void loop() = 0;
};