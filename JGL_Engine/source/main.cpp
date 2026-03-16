#include "pch.h"
#include "engine/engine.h"

int main(void)
{
  nengine::Engine engine;
  if (!engine.init())
    return 1;

  engine.run();

  return 0;
}
