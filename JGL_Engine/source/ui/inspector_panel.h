#pragma once

#include "ui/editor_panel_common.h"

namespace nui
{
  class InspectorPanel
  {
  public:
    void render(const EditorPanelContext& context, EditorPanelState& state);
  };
}
