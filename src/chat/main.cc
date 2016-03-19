#include <iostream>

#include "uv.h"

#include "server.h"

int main() {
  bchat::chat::Server s{uv_default_loop()};
  s.Start();
  std::cout << uv_strerror(uv_loop_close(uv_default_loop())) << std::endl;
}
