#include <plugin_base.vst3/editor.hpp>

using namespace Steinberg;

namespace plugin_base::vst3 {

editor::
editor(plugin_base::vst3::controller* controller, plugin_topo_factory factory) : 
EditorView(controller), _controller(controller)
{
  // pull the controller values to the ui (both live on the same thread)
  // alternatively we could keep yet another copy of ui-thread values
  // in group-module-param format but it's complicated enough already
  // to deal with processor-controller-ui_controls as is
  int plugin_param_index = 0;
  plugin_dims dims(controller->desc().topo);
  jarray3d<param_value> ui_initial_values;
  ui_initial_values.init(dims.module_param_counts);
  for (int g = 0; g < controller->desc().topo.module_groups.size(); g++)
  {
    auto const& group = controller->desc().topo.module_groups[g];
    for (int m = 0; m < group.module_count; m++)
      for(int p = 0; p < group.params.size(); p++)
      {
        auto const& param = group.params[p];
        int param_tag = _controller->desc().index_to_id[plugin_param_index++];
        double normalized = _controller->getParamNormalized(param_tag);
        ui_initial_values[g][m][p] = param_value::from_normalized(param, normalized);
      }
  }
  _gui = std::make_unique<plugin_gui>(factory, ui_initial_values);
}

tresult PLUGIN_API 
editor::removed()
{
  _gui->remove_any_param_ui_listener(this);
  _gui->setVisible(false);
  _gui->removeFromDesktop();
  return EditorView::removed();
}

tresult PLUGIN_API
editor::attached(void* parent, FIDString type)
{
  _gui->addToDesktop(0, parent);
  _gui->setVisible(true);
  _gui->add_any_param_ui_listener(this);
  return EditorView::attached(parent, type);
}

tresult PLUGIN_API
editor::getSize(ViewRect* new_size)
{
  new_size->top = 0;
  new_size->left = 0;
  new_size->right = _gui->getWidth();
  new_size->bottom = _gui->getHeight();
  return EditorView::getSize(new_size);
}

tresult PLUGIN_API 
editor::onSize(ViewRect* new_size)
{
  _gui->setSize(new_size->getWidth(), new_size->getHeight());
  return EditorView::onSize(new_size);
}

tresult PLUGIN_API
editor::checkSizeConstraint(ViewRect* new_rect)
{
  int new_height = new_rect->getWidth() / _gui->desc().topo.gui_aspect_ratio;
  new_rect->bottom = new_rect->top + new_height;
  return kResultTrue;
}

void 
editor::ui_param_changing(int param_index, param_value value)
{
  int param_tag = _controller->desc().index_to_id[param_index];
  param_mapping mapping = _controller->desc().param_mappings[param_index];
  auto normalized = value.to_normalized(*_controller->desc().param_at(mapping).topo);
  _controller->performEdit(param_tag, normalized);
  _controller->setParamNormalized(param_tag, normalized);
}

}