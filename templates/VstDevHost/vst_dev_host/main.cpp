extern void app0Entry(int argc, char **argv);
extern void app1Entry();

int main(int argc, char **argv) {
  app0Entry(argc, argv);
  // app1Entry(argc, argv);
  return 0;
}