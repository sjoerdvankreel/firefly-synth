#include <plugin_base.vst3/inf_editor.hpp>

using namespace Steinberg;

namespace plugin_base::vst3 {

inf_editor::
inf_editor(inf_controller* controller) :
EditorView(controller), _controller(controller),
_gui(std::make_unique<plugin_gui>(&controller->desc(), controller->ui_state())) {}

tresult PLUGIN_API 
inf_editor::removed()
{
  _gui->remove_ui_listener(this);
  _gui->setVisible(false);
  _gui->removeFromDesktop();
  return EditorView::removed();
}

tresult PLUGIN_API
inf_editor::attached(void* parent, FIDString type)
{
  _gui->addToDesktop(0, parent);
  _gui->setVisible(true);
  _gui->add_ui_listener(this);
  return EditorView::attached(parent, type);
}

tresult PLUGIN_API
inf_editor::getSize(ViewRect* new_size)
{
  new_size->top = 0;
  new_size->left = 0;
  new_size->right = _gui->getWidth();
  new_size->bottom = _gui->getHeight();
  return EditorView::getSize(new_size);
}

tresult PLUGIN_API 
inf_editor::onSize(ViewRect* new_size)
{
  _gui->setSize(new_size->getWidth(), new_size->getHeight());
  return EditorView::onSize(new_size);
}

tresult PLUGIN_API
inf_editor::checkSizeConstraint(ViewRect* new_rect)
{
  int new_height = new_rect->getWidth() / _controller->desc().plugin->gui_aspect_ratio;
  new_rect->bottom = new_rect->top + new_height;
  return kResultTrue;
}

void 
inf_editor::ui_changing(int index, plain_value plain)
{
  int id = _controller->desc().index_to_id[index];
  param_mapping const& mapping = _controller->desc().mappings[index];
  auto normalized = _controller->desc().plain_to_normalized_at(mapping, plain).value();
  _controller->performEdit(id, normalized);
  _controller->setParamNormalized(id, normalized);
}

}