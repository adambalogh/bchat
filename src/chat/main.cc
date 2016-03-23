#include <iostream>

#include "uv.h"

#include "server.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: <port>\n");
    exit(1);
  }
  int port = atoi(argv[1]);

  bchat::chat::Server s{uv_default_loop(), port};
  s.Start();
  std::cout << uv_strerror(uv_loop_close(uv_default_loop())) << std::endl;
}
