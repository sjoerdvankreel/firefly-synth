#include <plugin_base.vst3/editor.hpp>

using namespace Steinberg;

namespace plugin_base::vst3 {

tresult PLUGIN_API 
editor::removed()
{
  _gui.remove_any_param_ui_listener(this);
  _gui.setVisible(false);
  _gui.removeFromDesktop();
  return EditorView::removed();
}

tresult PLUGIN_API
editor::attached(void* parent, FIDString type)
{
  _gui.addToDesktop(0, parent);
  _gui.setVisible(true);
  _gui.add_any_param_ui_listener(this);
  return EditorView::attached(parent, type);
}

tresult PLUGIN_API
editor::getSize(ViewRect* new_size)
{
  new_size->top = 0;
  new_size->left = 0;
  new_size->right = _gui.getWidth();
  new_size->bottom = _gui.getHeight();
  return EditorView::getSize(new_size);
}

tresult PLUGIN_API 
editor::onSize(ViewRect* new_size)
{
  _gui.setSize(new_size->getWidth(), new_size->getHeight());
  return EditorView::onSize(new_size);
}

tresult PLUGIN_API
editor::checkSizeConstraint(ViewRect* new_rect)
{
  int new_height = new_rect->getWidth() / _gui.desc().topo.gui_aspect_ratio;
  new_rect->bottom = new_rect->top + new_height;
  return kResultTrue;
}

void 
editor::ui_param_changing(int param_index, param_value value)
{
  int param_tag = _controller->desc().index_to_id[param_index];
  param_mapping mapping = _controller->desc().param_mappings[param_index];
  _controller->performEdit(param_tag, value.to_normalized(*_controller->desc().param_at(mapping).topo));
}

}