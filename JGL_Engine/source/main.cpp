#include "pch.h"
#include "editor/editor_overlay.h"
#include "engine/engine.h"

int main(void)
{
  nengine::Engine engine;
  engine.set_window_overlay(std::make_shared<neditor::EditorOverlay>());
  if (!engine.init())
    return 1;

  engine.run();

  return 0;
}
