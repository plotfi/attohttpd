#include "atto.h"

int main(int argc, char **argv) {
  std::string LOAD_DIR = ".";

  printf("* Beginning Server Initialization *\n\n");
  printf("** Server Root Directory: %s\n", LOAD_DIR.c_str());
  if (chdir(LOAD_DIR.c_str()) < 0) {
    printf("Error setting home directory.\n");
    exit(1);
  }

  InitThreads(256 /* THREADS */, 1337 /* PORT */);
  printf("* Server Started *\n\n");

  for (int c = '\0'; c != 'q';)
    c = getchar();

  return EXIT_SUCCESS;
}
