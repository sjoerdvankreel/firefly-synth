#include <plugin_base/gui.hpp>
#include <plugin_base/topo.hpp>
#include <vector>

using namespace juce;

namespace plugin_base {

class param_base:
public single_param_plugin_listener
{
protected:
  plugin_gui* const _gui;
  param_desc const* const _desc;
  param_base(plugin_gui* gui, param_desc const* desc);
public:
  virtual ~param_base();
};

class param_name_label:
public Label
{
public:
  param_name_label(param_topo const* topo);
};

class param_value_label:
public param_base,
public Label
{
public:
  param_value_label(plugin_gui* gui, param_desc const* desc);
  void plugin_value_changed(param_value value) override final;
};

class param_slider:
public param_base, 
public Slider
{
public: 
  param_slider(plugin_gui* gui, param_desc const* desc);
  void valueChanged() override;
  void plugin_value_changed(param_value value) override final;
};

class param_combobox :
public param_base,
public ComboBox,
public ComboBox::Listener
{
public:
  ~param_combobox();
  param_combobox(plugin_gui* gui, param_desc const* desc);
  void comboBoxChanged(ComboBox*) override;
  void plugin_value_changed(param_value value) override final;
};

class param_toggle_button :
public param_base,
public ToggleButton,
public Button::Listener
{
public:
  ~param_toggle_button();
  param_toggle_button(plugin_gui* gui, param_desc const* desc);
  void buttonClicked(Button*) override {}
  void buttonStateChanged(Button*) override;
  void plugin_value_changed(param_value value) override final;
};

param_base::
param_base(plugin_gui* gui, param_desc const* desc) : 
_gui(gui), _desc(desc)
{ _gui->add_single_param_plugin_listener(_desc->index_in_plugin, this); }
param_base::
~param_base()
{ _gui->remove_single_param_plugin_listener(_desc->index_in_plugin, this); }

void
param_combobox::plugin_value_changed(param_value value)
{ setSelectedItemIndex(value.step - _desc->topo->min); }
void
param_toggle_button::plugin_value_changed(param_value value)
{ setToggleState(value.step != 0, dontSendNotification); }
void 
param_value_label::plugin_value_changed(param_value value)
{ setText(value.to_text(*_desc->topo), dontSendNotification); }
void
param_slider::plugin_value_changed(param_value value)
{ setValue(value.to_plain(*_desc->topo), dontSendNotification); }

void 
param_slider::valueChanged()
{ _gui->ui_param_changed(_desc->index_in_plugin, param_value::from_real(getValue())); }
void 
param_toggle_button::buttonStateChanged(Button*)
{ _gui->ui_param_changed(_desc->index_in_plugin, param_value::from_step(getToggleState() ? 1 : 0)); }
void 
param_combobox::comboBoxChanged(ComboBox*) 
{ _gui->ui_param_changed(_desc->index_in_plugin, param_value::from_step(getSelectedItemIndex() + _desc->topo->min)); }

param_name_label::
param_name_label(param_topo const* topo)
{ setText(topo->name, dontSendNotification); }

param_value_label::
param_value_label(plugin_gui* gui, param_desc const* desc):
param_base(gui, desc), Label()
{ plugin_value_changed(param_value::default_value(*desc->topo)); }

param_toggle_button::
~param_toggle_button()
{ removeListener(this); }
param_toggle_button::
param_toggle_button(plugin_gui* gui, param_desc const* desc):
param_base(gui, desc), ToggleButton()
{ 
  addListener(this);
  plugin_value_changed(param_value::default_value(*desc->topo)); 
}

param_combobox::
~param_combobox()
{ removeListener(this); }
param_combobox::
param_combobox(plugin_gui* gui, param_desc const* desc) :
param_base(gui, desc), ComboBox()
{
  switch (desc->topo->type)
  {
  case param_type::name:
    for (int i = 0; i < desc->topo->names.size(); i++)
      addItem(desc->topo->names[i], i + 1);
    break;
  case param_type::item:
    for (int i = 0; i < desc->topo->items.size(); i++)
      addItem(desc->topo->items[i].name, i + 1);
    break;
  case param_type::step:
    for (int i = desc->topo->min; i <= desc->topo->max; i++)
      addItem(std::to_string(i), desc->topo->min + i + 1);
    break;
  default:
    assert(false);
    break;
  }
  addListener(this);
  setEditableText(false);
  plugin_value_changed(param_value::default_value(*desc->topo));
}

param_slider::
param_slider(plugin_gui* gui, param_desc const* desc) :
param_base(gui, desc), Slider()
{
  switch (desc->topo->display)
  {
  case param_display::vslider:
    setSliderStyle(Slider::LinearVertical);
    break;
  case param_display::hslider:
    setSliderStyle(Slider::LinearHorizontal);
    break;
  case param_display::knob:
    setSliderStyle(Slider::RotaryVerticalDrag);
    break;
  default:
    assert(false);
    break;
  }
  setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
  if (!desc->topo->is_real()) setRange(desc->topo->min, desc->topo->max, 1);
  else setNormalisableRange(
    NormalisableRange<double>(desc->topo->min, desc->topo->max,
    [this](double s, double e, double v) { return param_value::from_normalized(*_desc->topo, v).to_plain(*_desc->topo); },
    [this](double s, double e, double v) { return param_value::from_plain(*_desc->topo, v).to_normalized(*_desc->topo); }));
  plugin_value_changed(param_value::default_value(*_desc->topo));
}

void 
plugin_gui::add_any_param_ui_listener(any_param_ui_listener* listener)
{ _any_param_ui_listeners.push_back(listener); }

void 
plugin_gui::add_single_param_plugin_listener(int param_index, single_param_plugin_listener* listener)
{ _single_param_plugin_listeners[param_index].push_back(listener); }

void 
plugin_gui::ui_param_changed(int param_index, param_value value)
{
  auto& listeners = _any_param_ui_listeners;
  for(int i = 0; i < listeners.size(); i++)
    listeners[i]->ui_value_changed(param_index, value);
}

void 
plugin_gui::plugin_param_changed(int param_index, param_value value)
{
  auto& listeners = _single_param_plugin_listeners[param_index];
  for(int i = 0; i < listeners.size(); i++)
    listeners[i]->plugin_value_changed(value);
}

void 
plugin_gui::remove_any_param_ui_listener(any_param_ui_listener* listener)
{
  auto& listeners = _any_param_ui_listeners;
  auto iter = std::find(listeners.begin(), listeners.end(), listener);
  if(iter != listeners.end()) listeners.erase(iter);
}

void 
plugin_gui::remove_single_param_plugin_listener(int param_index, single_param_plugin_listener* listener)
{
  auto& listeners = _single_param_plugin_listeners[param_index];
  auto iter = std::find(listeners.begin(), listeners.end(), listener);
  if (iter != listeners.end()) listeners.erase(iter);
}

plugin_gui::
plugin_gui(plugin_topo_factory factory) :
_desc(factory), 
_single_param_plugin_listeners(_desc.param_mappings.size())
{
  setOpaque(true);
  setSize(_desc.topo.gui_default_width, _desc.topo.gui_default_width / _desc.topo.gui_aspect_ratio);
  for (int m = 0; m < _desc.modules.size(); m++) // todo dont leak
  {
    auto const& module = _desc.modules[m];
    for (int p = 0; p < module.params.size(); p++)
    {
      if(module.params[p].topo->display == param_display::toggle)
        addAndMakeVisible(new param_toggle_button(this, &_desc.modules[m].params[p]));
      else if (module.params[p].topo->display == param_display::list)
        addAndMakeVisible(new param_combobox(this, &_desc.modules[m].params[p]));
      else
        addAndMakeVisible(new param_slider(this, &_desc.modules[m].params[p]));
      if(module.params[p].topo->text == param_text::none)
        addAndMakeVisible(new Label());
      else if (module.params[p].topo->text == param_text::name)
        addAndMakeVisible(new param_name_label(module.params[p].topo));
      else
        addAndMakeVisible(new param_value_label(this, &_desc.modules[m].params[p]));
    }
  }
  resized();
}

void 
plugin_gui::resized()
{
  Grid grid;
  int c = 0;
  grid.templateRows.add(Grid::TrackInfo(Grid::Fr(1)));
  grid.templateRows.add(Grid::TrackInfo(Grid::Fr(1)));
  for (int m = 0; m < _desc.modules.size(); m++)
  {
    auto const& module = _desc.modules[m];
    for (int p = 0; p < module.params.size(); p++)
    {
      GridItem edit_item(getChildComponent(c * 2));
      edit_item.row.end = 2;
      edit_item.row.start = 1;
      edit_item.column.start = c + 1;
      edit_item.column.end = c + 2;
      grid.items.add(edit_item);
      GridItem label_item(getChildComponent(c * 2 + 1));
      label_item.row.end = 3;
      label_item.row.start = 2;
      label_item.column.start = c + 1;
      label_item.column.end = c + 2;
      grid.items.add(label_item);
      c++;
      grid.templateColumns.add(Grid::TrackInfo(Grid::Fr(1)));
    }
  } 
  grid.performLayout(getLocalBounds());
}

}