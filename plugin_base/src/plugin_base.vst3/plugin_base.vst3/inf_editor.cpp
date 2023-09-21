#include <plugin_base.vst3/inf_editor.hpp>

using namespace Steinberg;

namespace plugin_base::vst3 {

tresult PLUGIN_API
inf_editor::setContentScaleFactor(float factor)
{
  gui()->content_scale(factor);
  return kResultOk;
}

tresult PLUGIN_API
inf_editor::queryInterface(TUID const iid, void** obj)
{
  QUERY_INTERFACE(iid, obj, IPlugViewContentScaleSupport::iid, IPlugViewContentScaleSupport)
  return EditorView::queryInterface(iid, obj);
}

tresult PLUGIN_API
inf_editor::getSize(ViewRect* new_size)
{
  new_size->top = 0;
  new_size->left = 0;
  new_size->right = gui()->getWidth();
  new_size->bottom = gui()->getHeight();
  checkSizeConstraint(new_size);
  return EditorView::getSize(new_size);
}

tresult PLUGIN_API 
inf_editor::onSize(ViewRect* new_size)
{
  checkSizeConstraint(new_size);
  gui()->setSize(new_size->getWidth(), new_size->getHeight());
  return EditorView::onSize(new_size);
}

tresult PLUGIN_API
inf_editor::checkSizeConstraint(ViewRect* new_rect)
{
  auto const& topo = *_controller->desc().plugin;
  int new_width = std::max(new_rect->getWidth(), topo.gui_min_width);
  new_rect->top = 0;
  new_rect->left = 0;
  new_rect->right = new_width;
  new_rect->bottom = new_rect->getWidth() / topo.gui_aspect_ratio;
  rect.right = new_rect->right;
  rect.bottom = new_rect->bottom;
  return kResultTrue;
}

}