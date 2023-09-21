#if JUCE_LINUX || JUCE_BSD
#include <plugin_base.vst3/inf_editor_linux.hpp>

#include <pluginterfaces/vst/ivstplugview.h>

using namespace Steinberg;

#include <juce_events/native/juce_EventLoopInternal_linux.h>
#include <juce_audio_plugin_client/detail/juce_LinuxMessageThread.h>
#include <plugin_base.vst3/inf_editor_linux_juce_vst3_common_rip.hpp>
#include <plugin_base.vst3/inf_editor_linux_juce_audio_plugin_client_vst3_rip.hpp>

using namespace juce;

namespace plugin_base::vst3 {

struct inf_editor_linux::impl
{
  std::unique_ptr<plugin_gui> gui; // TODO
  SharedResourcePointer<EventHandler> event_handler;
  SharedResourcePointer<detail::MessageThread> message_thread;
};

inf_editor_linux::
inf_editor_linux(inf_controller* controller):
inf_editor(controller), _impl(std::make_unique<impl>())
{ _impl->gui = std::make_unique<plugin_gui>(&controller->desc(), &controller->ui_state()); }

plugin_gui* 
inf_editor_linux::gui() const
{ return _impl->gui.get(); }

tresult PLUGIN_API 
inf_editor_linux::removed()
{
}

tresult PLUGIN_API 
inf_editor_linux::attached(void* parent, FIDString type)
{
}

}

#endif
