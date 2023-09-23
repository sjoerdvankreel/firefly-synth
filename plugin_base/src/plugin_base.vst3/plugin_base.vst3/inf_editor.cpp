#include <plugin_base.vst3/inf_editor.hpp>
#if __linux__
#include <juce_events/native/juce_EventLoopInternal_linux.h>
#endif

#include <iostream>

using namespace juce;
using namespace Steinberg;

namespace plugin_base::vst3 {

inf_editor::
inf_editor(inf_controller* controller) :
EditorView(controller), _controller(controller),
_gui(std::make_unique<plugin_gui>(&controller->desc(), &controller->ui_state())) {}

tresult PLUGIN_API
inf_editor::setContentScaleFactor(float factor)
{
  _gui->content_scale(factor);
  return kResultOk;
}

tresult PLUGIN_API
inf_editor::getSize(ViewRect* new_size)
{
  new_size->top = 0;
  new_size->left = 0;
  new_size->right = _gui->getWidth();
  new_size->bottom = _gui->getHeight();
  checkSizeConstraint(new_size);
  return EditorView::getSize(new_size);
}

tresult PLUGIN_API
inf_editor::onSize(ViewRect* new_size)
{
  checkSizeConstraint(new_size);
  _gui->setSize(new_size->getWidth(), new_size->getHeight());
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

#ifdef __linux__
void PLUGIN_API
inf_editor::onFDIsSet(Steinberg::Linux::FileDescriptor fd)
{
  std::cout << "POSIX ME HARDER! " << fd << "\n";
  /*
  if (!MessageManager::getInstance()->isThisTheMessageThread())
  {
    if (messageThread->isRunning())
      messageThread->stop();
    MessageManager::getInstance()->setCurrentThreadAsMessageThread();
  }
  */
  LinuxEventLoopInternal::invokeEventLoopCallbackForFd(fd);
}
#endif

tresult PLUGIN_API
inf_editor::removed()
{
  _gui->remove_ui_listener(_controller);
  _gui->setVisible(false);
  _gui->removeFromDesktop();
#ifdef __linux__
  Steinberg::Linux::IRunLoop* loop = {};
  INF_ASSERT_EXEC(!plugFrame->queryInterface(Steinberg::Linux::IRunLoop::iid, (void**)&loop));
  std::cout << "Unregistering\n";
  loop->unregisterEventHandler(this);
#endif
  return EditorView::removed();
}

tresult PLUGIN_API
inf_editor::attached(void* parent, FIDString type)
{
#ifdef __linux__
  Steinberg::Linux::IRunLoop* loop = {};
  INF_ASSERT_EXEC(!plugFrame->queryInterface(Steinberg::Linux::IRunLoop::iid, (void**)&loop));
  for (int fd: LinuxEventLoopInternal::getRegisteredFds())
  {
    std::cout << "Registering fd " << fd << "\n";
    loop->registerEventHandler(this, fd);
  }
#endif
  _gui->addToDesktop(0, parent);
  _gui->setVisible(true);
  _gui->add_ui_listener(_controller);
  return EditorView::attached(parent, type);
}

tresult PLUGIN_API
inf_editor::queryInterface(TUID const iid, void** obj)
{
  QUERY_INTERFACE(iid, obj, IPlugViewContentScaleSupport::iid, IPlugViewContentScaleSupport)
#ifdef __linux__
  QUERY_INTERFACE(iid, obj, Steinberg::Linux::IEventHandler::iid, Steinberg::Linux::IEventHandler)
#endif
    return EditorView::queryInterface(iid, obj);
}

}