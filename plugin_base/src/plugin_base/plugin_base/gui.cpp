#include <plugin_base/gui.hpp>
#include <plugin_base/topo.hpp>
#include <vector>

using namespace juce;

namespace plugin_base {

class param_base:
public plugin_listener
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
  param_name_label(param_desc const* desc);
};

class param_value_label:
public param_base,
public Label
{
public:
  param_value_label(plugin_gui* gui, param_desc const* desc, plain_value initial);
  void plugin_changed(plain_value plain) override final;
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
  void plugin_changed(plain_value plain) override final;
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
  void plugin_changed(plain_value plain) override final;
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
  void plugin_changed(plain_value plain) override final;
};

class param_textbox :
public param_base,
public TextEditor,
public TextEditor::Listener
{
  std::string _last_parsed;
public:
  ~param_textbox();
  param_textbox(plugin_gui* gui, param_desc const* desc, plain_value initial);
  void plugin_changed(plain_value plain) override final;
  void textEditorTextChanged(TextEditor&) override;
  void textEditorFocusLost(TextEditor&) override { setText(_last_parsed, false); }
  void textEditorReturnKeyPressed(TextEditor&) override { setText(_last_parsed, false); }
  void textEditorEscapeKeyPressed(TextEditor&) override { setText(_last_parsed, false); }
};

param_base::
param_base(plugin_gui* gui, param_desc const* desc) : 
_gui(gui), _desc(desc)
{ _gui->add_plugin_listener(_desc->global, this); }
param_base::
~param_base()
{ _gui->remove_plugin_listener(_desc->global, this); }

void
param_combobox::plugin_changed(plain_value plain)
{ setSelectedItemIndex(plain.step() - _desc->param->min); }
void
param_toggle_button::plugin_changed(plain_value plain)
{ setToggleState(plain.step() != 0, dontSendNotification); }
void
param_slider::plugin_changed(plain_value plain)
{ setValue(_desc->param->plain_to_raw(plain), dontSendNotification); }

void
param_textbox::plugin_changed(plain_value plain)
{
  _last_parsed = _desc->param->plain_to_text(plain);
  setText(_last_parsed, false);
}

void
param_value_label::plugin_changed(plain_value plain)
{ 
  std::string text = _desc->param->plain_to_text(plain);
  if(_desc->param->label == param_label::both)
    text = _desc->short_name + " " + text;
  setText(text, dontSendNotification); 
}

void 
param_slider::stoppedDragging()
{ _gui->ui_end_changes(_desc->global); }
void 
param_slider::startedDragging()
{ _gui->ui_begin_changes(_desc->global); }

void 
param_slider::valueChanged()
{ 
  auto value = _desc->param->raw_to_plain(getValue());
  _gui->ui_changing(_desc->global, value);
}

void 
param_toggle_button::buttonStateChanged(Button*)
{ 
  if(_checked == getToggleState()) return;
  plain_value plain = _desc->param->raw_to_plain(getToggleState() ? 1 : 0);
  _checked = getToggleState();
  _gui->ui_changed(_desc->global, plain);
}

void 
param_textbox::textEditorTextChanged(TextEditor&)
{
  plain_value plain;
  std::string text(getText().toStdString());
  if(!_desc->param->text_to_plain(text, plain)) return;
  _last_parsed = text;
  _gui->ui_changed(_desc->global, plain);
}

void
param_combobox::comboBoxChanged(ComboBox*)
{
  plain_value plain = _desc->param->raw_to_plain(getSelectedItemIndex() + _desc->param->min);
  _gui->ui_changed(_desc->global, plain);
}

param_name_label::
param_name_label(param_desc const* desc)
{ setText(desc->short_name, dontSendNotification); }

param_value_label::
param_value_label(plugin_gui* gui, param_desc const* desc, plain_value initial):
param_base(gui, desc), Label()
{ plugin_changed(initial); }

param_textbox::
~param_textbox()
{ removeListener(this); }
param_textbox::
param_textbox(plugin_gui* gui, param_desc const* desc, plain_value initial) :
param_base(gui, desc), TextEditor()
{
  addListener(this);
  plugin_changed(initial);
}

param_toggle_button::
~param_toggle_button()
{ removeListener(this); }
param_toggle_button::
param_toggle_button(plugin_gui* gui, param_desc const* desc, plain_value initial):
param_base(gui, desc), ToggleButton()
{ 
  auto value = desc->param->default_plain();
  _checked = value.step() != 0;
  addListener(this);
  plugin_changed(initial);
}

param_combobox::
~param_combobox()
{ removeListener(this); }
param_combobox::
param_combobox(plugin_gui* gui, param_desc const* desc, plain_value initial) :
param_base(gui, desc), ComboBox()
{
  switch (desc->param->type)
  {
  case param_type::name:
    for (int i = 0; i < desc->param->names.size(); i++)
      addItem(desc->param->names[i], i + 1);
    break;
  case param_type::item:
    for (int i = 0; i < desc->param->items.size(); i++)
      addItem(desc->param->items[i].name, i + 1);
    break;
  case param_type::step:
    for (int i = desc->param->min; i <= desc->param->max; i++)
      addItem(std::to_string(i), desc->param->min + i + 1);
    break;
  default:
    assert(false);
    break;
  }
  addListener(this);
  setEditableText(false);
  plugin_changed(initial);
}

param_slider::
param_slider(plugin_gui* gui, param_desc const* desc, plain_value initial) :
param_base(gui, desc), Slider()
{
  switch (desc->param->edit)
  {
  case param_edit::vslider:
    setSliderStyle(Slider::LinearVertical);
    break;
  case param_edit::hslider:
    setSliderStyle(Slider::LinearHorizontal);
    break;
  case param_edit::knob:
    setSliderStyle(Slider::RotaryVerticalDrag);
    break;
  default:
    assert(false);
    break;
  }
  setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
  if (!desc->param->is_real()) setRange(desc->param->min, desc->param->max, 1);
  else setNormalisableRange(
    NormalisableRange<double>(desc->param->min, desc->param->max,
    [this](double s, double e, double v) { return _desc->param->normalized_to_raw(normalized_value(v)); },
    [this](double s, double e, double v) { return _desc->param->raw_to_normalized(v).value(); }));
  setDoubleClickReturnValue(true, _desc->param->default_raw(), ModifierKeys::noModifiers);
  plugin_changed(initial);
}

void 
plugin_gui::add_ui_listener(ui_listener* listener)
{ _ui_listeners.push_back(listener); }

void 
plugin_gui::add_plugin_listener(int index, plugin_listener* listener)
{ _plugin_listeners[index].push_back(listener); }

void 
plugin_gui::ui_changed(int index, plain_value plain)
{
  ui_begin_changes(index);
  ui_changing(index, plain);
  ui_end_changes(index);
}

void 
plugin_gui::ui_begin_changes(int index)
{
  auto& listeners = _ui_listeners;
  for(int i = 0; i < listeners.size(); i++)
    listeners[i]->ui_begin_changes(index);
}

void
plugin_gui::ui_end_changes(int index)
{
  auto& listeners = _ui_listeners;
  for (int i = 0; i < listeners.size(); i++)
    listeners[i]->ui_end_changes(index);
}

void
plugin_gui::ui_changing(int index, plain_value plain)
{
  auto& listeners = _ui_listeners;
  for (int i = 0; i < listeners.size(); i++)
    listeners[i]->ui_changing(index, plain);
}

void
plugin_gui::plugin_changed(int index, plain_value plain)
{
  auto& listeners = _plugin_listeners[index];
  for(int i = 0; i < listeners.size(); i++)
    listeners[i]->plugin_changed(plain);
}

void 
plugin_gui::remove_ui_listener(ui_listener* listener)
{
  auto& listeners = _ui_listeners;
  auto iter = std::find(listeners.begin(), listeners.end(), listener);
  if(iter != listeners.end()) listeners.erase(iter);
}

void 
plugin_gui::remove_plugin_listener(int index, plugin_listener* listener)
{
  auto& listeners = _plugin_listeners[index];
  auto iter = std::find(listeners.begin(), listeners.end(), listener);
  if (iter != listeners.end()) listeners.erase(iter);
}

plugin_gui::
plugin_gui(plugin_desc const* desc, jarray<plain_value, 4> const& initial) :
_desc(desc), 
_plugin_listeners(desc->param_count)
{
  setOpaque(true);
  setSize(_desc->plugin->gui_default_width, _desc->plugin->gui_default_width / _desc->plugin->gui_aspect_ratio);
  for (int m = 0; m < _desc->modules.size(); m++)
  {
    auto const& module = _desc->modules[m];
    for (int p = 0; p < module.params.size(); p++)
    {
      auto const& param = module.params[p];
      plain_value initial_value = initial
        [module.topo][module.slot][param.topo][param.slot];
      if(param.param->edit == param_edit::toggle)
      {
        _children.emplace_back(std::make_unique<param_toggle_button>(this, &param, initial_value));
        addAndMakeVisible(_children[_children.size() - 1].get());
      }
      else if (param.param->edit == param_edit::list)
      {
        _children.emplace_back(std::make_unique<param_combobox>(this, &param, initial_value));
        addAndMakeVisible(_children[_children.size() - 1].get());
      }
      else if (param.param->edit == param_edit::text)
      {
        _children.emplace_back(std::make_unique<param_textbox>(this, &param, initial_value));
        addAndMakeVisible(_children[_children.size() - 1].get());
      }
      else
      {
        _children.emplace_back(std::make_unique<param_slider>(this, &param, initial_value));
        addAndMakeVisible(_children[_children.size() - 1].get());
      }
      _children[_children.size()-1]->setEnabled(param.param->dir == param_dir::input);

      if(param.param->label == param_label::none)
      {
        _children.emplace_back(std::make_unique<Label>());
        addAndMakeVisible(_children[_children.size() - 1].get());
      }
      else if (param.param->label == param_label::name)
      {
        _children.emplace_back(std::make_unique<param_name_label>(&param));
        addAndMakeVisible(_children[_children.size() - 1].get());
      }
      else
      {
        _children.emplace_back(std::make_unique<param_value_label>(this, &param, initial_value));
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
  for (int m = 0; m < _desc->modules.size(); m++)
  {
    auto const& module = _desc->modules[m];
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