#include <plugin_base.vst3/inf_editor_generic.hpp>

using namespace Steinberg;

namespace plugin_base::vst3 {

inf_editor_generic::
inf_editor_generic(inf_controller* controller) :
inf_editor(controller),
_gui(std::make_unique<plugin_gui>(&controller->desc(), &controller->ui_state())) {}

tresult PLUGIN_API
inf_editor_generic::removed()
{
  _gui->remove_ui_listener(_controller);
  _gui->setVisible(false);
  _gui->removeFromDesktop();
  return EditorView::removed();
}

tresult PLUGIN_API
inf_editor_generic::attached(void* parent, FIDString type)
{
  _gui->addToDesktop(0, parent);
  _gui->setVisible(true);
  _gui->add_ui_listener(_controller);
  return EditorView::attached(parent, type);
}

}