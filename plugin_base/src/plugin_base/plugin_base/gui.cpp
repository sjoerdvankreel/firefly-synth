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
  param_value_label(plugin_gui* gui, param_desc const* desc, plain_value initial);
  void plugin_value_changed(plain_value plain) override final;
};

class param_slider:
public param_base, 
public Slider
{
public: 
  param_slider(plugin_gui* gui, param_desc const* desc, plain_value initial);
  void valueChanged() override;
  void stoppedDragging() override;
  void startedDragging() override;
  void plugin_value_changed(plain_value plain) override final;
};

class param_combobox :
public param_base,
public ComboBox,
public ComboBox::Listener
{
public:
  ~param_combobox();
  param_combobox(plugin_gui* gui, param_desc const* desc, plain_value initial);
  void comboBoxChanged(ComboBox*) override;
  void plugin_value_changed(plain_value plain) override final;
};

class param_toggle_button :
public param_base,
public ToggleButton,
public Button::Listener
{
  bool _checked = false;
public:
  ~param_toggle_button();
  param_toggle_button(plugin_gui* gui, param_desc const* desc, plain_value initial);
  void buttonClicked(Button*) override {}
  void buttonStateChanged(Button*) override;
  void plugin_value_changed(plain_value plain) override final;
};

class param_textbox :
public param_base,
public TextEditor,
public TextEditor::Listener
{
public:
  ~param_textbox();
  param_textbox(plugin_gui* gui, param_desc const* desc, plain_value initial);
  void plugin_value_changed(plain_value plain) override final;
  void textEditorFocusLost(TextEditor&) override;
  void textEditorTextChanged(TextEditor&) override;
  void textEditorReturnKeyPressed(TextEditor&) override;
  void textEditorEscapeKeyPressed(TextEditor&) override;
};

param_base::
param_base(plugin_gui* gui, param_desc const* desc) : 
_gui(gui), _desc(desc)
{ _gui->add_single_param_plugin_listener(_desc->index_in_plugin, this); }
param_base::
~param_base()
{ _gui->remove_single_param_plugin_listener(_desc->index_in_plugin, this); }

void
param_textbox::plugin_value_changed(plain_value plain)
{ setText(_desc->topo->plain_to_text(plain), false); }
void
param_combobox::plugin_value_changed(plain_value plain)
{ setSelectedItemIndex(plain.step() - _desc->topo->min); }
void
param_toggle_button::plugin_value_changed(plain_value plain)
{ setToggleState(plain.step() != 0, dontSendNotification); }
void
param_slider::plugin_value_changed(plain_value plain)
{ setValue(_desc->topo->plain_to_raw(plain), dontSendNotification); }

void 
param_value_label::plugin_value_changed(plain_value plain)
{ 
  std::string text = _desc->topo->plain_to_text(plain);
  if(_desc->topo->label == param_label::both)
    text = _desc->topo->name + " " + text;
  setText(text, dontSendNotification); 
}

void 
param_slider::stoppedDragging()
{ _gui->ui_param_end_changes(_desc->index_in_plugin); }
void 
param_slider::startedDragging()
{ _gui->ui_param_begin_changes(_desc->index_in_plugin); }

void 
param_slider::valueChanged()
{ 
  auto value = _desc->topo->raw_to_plain(getValue());
  _gui->ui_param_changing(_desc->index_in_plugin, value); 
}

void 
param_toggle_button::buttonStateChanged(Button*)
{ 
  if(_checked == getToggleState()) return;
  auto value = _desc->topo->raw_to_plain(getToggleState() ? 1 : 0);
  _checked = getToggleState();
  _gui->ui_param_begin_changes(_desc->index_in_plugin);
  _gui->ui_param_changing(_desc->index_in_plugin, value); 
  _gui->ui_param_end_changes(_desc->index_in_plugin);
}

void 
param_combobox::comboBoxChanged(ComboBox*) 
{ 
  auto value = _desc->topo->raw_to_plain(getSelectedItemIndex() + _desc->topo->min);
  _gui->ui_param_begin_changes(_desc->index_in_plugin);
  _gui->ui_param_changing(_desc->index_in_plugin, value);
  _gui->ui_param_end_changes(_desc->index_in_plugin);
}

void 
param_textbox::textEditorFocusLost(TextEditor&)
{
}

void 
param_textbox::textEditorTextChanged(TextEditor&)
{
}

void 
param_textbox::textEditorReturnKeyPressed(TextEditor&)
{
}

void 
param_textbox::textEditorEscapeKeyPressed(TextEditor&)
{
}

param_name_label::
param_name_label(param_topo const* topo)
{ setText(topo->name, dontSendNotification); }

param_value_label::
param_value_label(plugin_gui* gui, param_desc const* desc, plain_value initial):
param_base(gui, desc), Label()
{ plugin_value_changed(initial); }

param_textbox::
~param_textbox()
{ removeListener(this); }
param_textbox::
param_textbox(plugin_gui* gui, param_desc const* desc, plain_value initial) :
param_base(gui, desc), TextEditor()
{
  addListener(this);
  plugin_value_changed(initial);
}

param_toggle_button::
~param_toggle_button()
{ removeListener(this); }
param_toggle_button::
param_toggle_button(plugin_gui* gui, param_desc const* desc, plain_value initial):
param_base(gui, desc), ToggleButton()
{ 
  auto value = desc->topo->default_plain();
  _checked = value.step() != 0;
  addListener(this);
  plugin_value_changed(initial);
}

param_combobox::
~param_combobox()
{ removeListener(this); }
param_combobox::
param_combobox(plugin_gui* gui, param_desc const* desc, plain_value initial) :
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
  plugin_value_changed(initial);
}

param_slider::
param_slider(plugin_gui* gui, param_desc const* desc, plain_value initial) :
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
    [this](double s, double e, double v) { return _desc->topo->normalized_to_raw(normalized_value(v)); },
    [this](double s, double e, double v) { return _desc->topo->raw_to_normalized(v).value(); }));
  setDoubleClickReturnValue(true, _desc->topo->default_raw(), ModifierKeys::noModifiers);
  plugin_value_changed(initial);
}

void 
plugin_gui::add_any_param_ui_listener(any_param_ui_listener* listener)
{ _any_param_ui_listeners.push_back(listener); }

void 
plugin_gui::add_single_param_plugin_listener(int param_index, single_param_plugin_listener* listener)
{ _single_param_plugin_listeners[param_index].push_back(listener); }

void 
plugin_gui::ui_param_begin_changes(int param_index)
{
  auto& listeners = _any_param_ui_listeners;
  for(int i = 0; i < listeners.size(); i++)
    listeners[i]->ui_param_begin_changes(param_index);
}

void
plugin_gui::ui_param_end_changes(int param_index)
{
  auto& listeners = _any_param_ui_listeners;
  for (int i = 0; i < listeners.size(); i++)
    listeners[i]->ui_param_end_changes(param_index);
}

void
plugin_gui::ui_param_changing(int param_index, plain_value plain)
{
  auto& listeners = _any_param_ui_listeners;
  for (int i = 0; i < listeners.size(); i++)
    listeners[i]->ui_param_changing(param_index, plain);
}

void
plugin_gui::plugin_param_changed(int param_index, plain_value plain)
{
  auto& listeners = _single_param_plugin_listeners[param_index];
  for(int i = 0; i < listeners.size(); i++)
    listeners[i]->plugin_value_changed(plain);
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
plugin_gui(plugin_topo_factory factory, jarray3d<plain_value> const& initial) :
_desc(factory), 
_single_param_plugin_listeners(_desc.param_mappings.size())
{
  setOpaque(true);
  setSize(_desc.topo.gui_default_width, _desc.topo.gui_default_width / _desc.topo.gui_aspect_ratio);
  for (int m = 0; m < _desc.modules.size(); m++)
  {
    auto const& module = _desc.modules[m];
    for (int p = 0; p < module.params.size(); p++)
    {
      if(module.params[p].topo->display == param_display::toggle)
      {
        _children.emplace_back(std::make_unique<param_toggle_button>(this, &_desc.modules[m].params[p], initial[module.group_in_plugin][module.module_in_group][p]));
        addAndMakeVisible(_children[_children.size() - 1].get());
      }
      else if (module.params[p].topo->display == param_display::list)
      {
        _children.emplace_back(std::make_unique<param_combobox>(this, &_desc.modules[m].params[p], initial[module.group_in_plugin][module.module_in_group][p]));
        addAndMakeVisible(_children[_children.size() - 1].get());
      }
      else if (module.params[p].topo->display == param_display::text)
      {
        _children.emplace_back(std::make_unique<param_textbox>(this, &_desc.modules[m].params[p], initial[module.group_in_plugin][module.module_in_group][p]));
        addAndMakeVisible(_children[_children.size() - 1].get());
      }
      else
      {
        _children.emplace_back(std::make_unique<param_slider>(this, &_desc.modules[m].params[p], initial[module.group_in_plugin][module.module_in_group][p]));
        addAndMakeVisible(_children[_children.size() - 1].get());
      }
      if(module.params[p].topo->label == param_label::none)
      {
        _children.emplace_back(std::make_unique<Label>());
        addAndMakeVisible(_children[_children.size() - 1].get());
      }
      else if (module.params[p].topo->label == param_label::name)
      {
        _children.emplace_back(std::make_unique<param_name_label>(module.params[p].topo));
        addAndMakeVisible(_children[_children.size() - 1].get());
      }
      else
      {
        _children.emplace_back(std::make_unique<param_value_label>(this, &_desc.modules[m].params[p], initial[module.group_in_plugin][module.module_in_group][p]));
        addAndMakeVisible(_children[_children.size() - 1].get());
      }
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