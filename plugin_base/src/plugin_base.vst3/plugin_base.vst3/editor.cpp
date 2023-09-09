#include <plugin_base.vst3/editor.hpp>

using namespace Steinberg;

namespace plugin_base::vst3 {

tresult PLUGIN_API 
editor::removed()
{
  _gui.setVisible(false);
  _gui.removeFromDesktop();
  return EditorView::removed();
}

tresult PLUGIN_API
editor::attached(void* parent, FIDString type)
{
  _gui.addToDesktop(0, parent);
  _gui.setVisible(true);
  return EditorView::attached(parent, type);
}

tresult PLUGIN_API 
editor::onSize(ViewRect* new_size)
{
  _gui.setSize(new_size->getWidth(), new_size->getHeight());
  return EditorView::onSize(new_size);
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

}