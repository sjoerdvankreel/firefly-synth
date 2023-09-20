#include <plugin_base/io.hpp>
#include <plugin_base/gui.hpp>
#include <plugin_base/topo.hpp>

#include <vector>

using namespace juce;

namespace plugin_base {

void 
ui_listener::ui_changed(int index, plain_value plain)
{
  ui_begin_changes(index);
  ui_changing(index, plain);
  ui_end_changes(index);
}

// control types bound to a parameter topo & plugin value

class param_base:
public plugin_listener
{
protected:
  plugin_gui* const _gui;
  param_desc const* const _desc;
  param_base(plugin_gui* gui, param_desc const* desc): 
  _gui(gui), _desc(desc) 
  { _gui->add_plugin_listener(_desc->global, this); }
public:
  virtual ~param_base() 
  { _gui->remove_plugin_listener(_desc->global, this); }
};

class param_name_label:
public Label
{
public:
  param_name_label(param_desc const* desc) 
  { setText(desc->short_name, dontSendNotification);  }
};

class param_value_label:
public param_base, public Label
{
public:
  void plugin_changed(plain_value plain) override final;
  param_value_label(plugin_gui* gui, param_desc const* desc, plain_value initial):
  param_base(gui, desc), Label() { plugin_changed(initial); }
};

class param_slider:
public param_base, public Slider
{
public: 
  param_slider(plugin_gui* gui, param_desc const* desc, plain_value initial);
  void valueChanged() override
  { _gui->ui_changing(_desc->global, _desc->param->raw_to_plain(getValue())); }
  void plugin_changed(plain_value plain) override final 
  { setValue(_desc->param->plain_to_raw(plain), dontSendNotification); }
  void stoppedDragging() override { _gui->ui_end_changes(_desc->global); }
  void startedDragging() override { _gui->ui_begin_changes(_desc->global); }
};

class param_combobox :
public param_base, public ComboBox, public ComboBox::Listener
{
public:
  ~param_combobox() { removeListener(this); }
  param_combobox(plugin_gui* gui, param_desc const* desc, plain_value initial);
  void plugin_changed(plain_value plain) override final
  { setSelectedItemIndex(plain.step() - _desc->param->min); }
  void comboBoxChanged(ComboBox*) override final
  { _gui->ui_changed(_desc->global, 
    _desc->param->raw_to_plain(getSelectedItemIndex() + _desc->param->min)); }
};

class param_toggle_button :
public param_base, public ToggleButton, public Button::Listener
{
  bool _checked = false;
public:
  ~param_toggle_button() { removeListener(this); }
  param_toggle_button(plugin_gui* gui, param_desc const* desc, plain_value initial);
  
  void buttonClicked(Button*) override {}
  void buttonStateChanged(Button*) override;
  void plugin_changed(plain_value plain) override final 
  { setToggleState(plain.step() != 0, dontSendNotification); }
};

class param_textbox :
public param_base, public TextEditor, public TextEditor::Listener
{
  std::string _last_parsed;
public:
  ~param_textbox() { removeListener(this); }
  param_textbox(plugin_gui* gui, param_desc const* desc, plain_value initial);

  void textEditorTextChanged(TextEditor&) override;
  void plugin_changed(plain_value plain) override final
  { setText(_last_parsed = _desc->param->plain_to_text(plain), false); }

  void textEditorFocusLost(TextEditor&) override { setText(_last_parsed, false); }
  void textEditorReturnKeyPressed(TextEditor&) override { setText(_last_parsed, false); }
  void textEditorEscapeKeyPressed(TextEditor&) override { setText(_last_parsed, false); }
};

void
param_textbox::textEditorTextChanged(TextEditor&)
{
  plain_value plain;
  std::string text(getText().toStdString());
  if (!_desc->param->text_to_plain(text, plain)) return;
  _last_parsed = text;
  _gui->ui_changed(_desc->global, plain);
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
param_toggle_button::buttonStateChanged(Button*)
{ 
  if(_checked == getToggleState()) return;
  plain_value plain = _desc->param->raw_to_plain(getToggleState() ? 1 : 0);
  _checked = getToggleState();
  _gui->ui_changed(_desc->global, plain);
}

param_textbox::
param_textbox(plugin_gui* gui, param_desc const* desc, plain_value initial) :
param_base(gui, desc), TextEditor()
{
  addListener(this);
  plugin_changed(initial);
}

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

// resizes single child on resize

class group_component :
  public GroupComponent {
public:
  void resized() override;
};

void
group_component::resized()
{
  assert(getNumChildComponents() < 2);
  if (getNumChildComponents() == 1)
    getChildComponent(0)->setBounds(getLocalBounds().reduced(15));
}

// grid component as opposed to grid layout
// resizes children on resize

class grid_component:
public Component
{
  gui_dimension const _dimension;
  std::vector<gui_position> _positions = {};

public:
  void resized() override;
  void add(Component& child, gui_position const& position);
  grid_component(gui_dimension const& dimension): _dimension(dimension) {}
};

void
grid_component::add(Component& child, gui_position const& position)
{
  addAndMakeVisible(child);
  _positions.push_back(position);
}

void 
grid_component::resized()
{
  Grid grid;
  for(int i = 0; i < _dimension.rows; i++)
    grid.templateRows.add(Grid::Fr(1));
  for(int i = 0; i < _dimension.columns; i++) 
    grid.templateColumns.add(Grid::Fr(1));
  for (int i = 0; i < _positions.size(); i++)
  {
    GridItem item(getChildComponent(i));
    item.row.start = _positions[i].row + 1;
    item.column.start = _positions[i].column + 1;
    item.row.end = _positions[i].row + 1 + _positions[i].dimension.rows;
    item.column.end = _positions[i].column + 1 + _positions[i].dimension.columns;
    grid.items.add(item);
  }
  grid.performLayout(getLocalBounds());
}

// main plugin gui

void 
plugin_gui::ui_changed(int index, plain_value plain)
{
  for (int i = 0; i < _ui_listeners.size(); i++)
    _ui_listeners[i]->ui_changed(index, plain);
}

void 
plugin_gui::ui_begin_changes(int index)
{
  for(int i = 0; i < _ui_listeners.size(); i++)
    _ui_listeners[i]->ui_begin_changes(index);
}

void
plugin_gui::ui_end_changes(int index)
{
  for (int i = 0; i < _ui_listeners.size(); i++)
    _ui_listeners[i]->ui_end_changes(index);
}

void
plugin_gui::ui_changing(int index, plain_value plain)
{
  for (int i = 0; i < _ui_listeners.size(); i++)
    _ui_listeners[i]->ui_changing(index, plain);
}

void
plugin_gui::plugin_changed(int index, plain_value plain)
{
  for(int i = 0; i < _plugin_listeners[index].size(); i++)
    _plugin_listeners[index][i]->plugin_changed(plain);
}

void 
plugin_gui::remove_ui_listener(ui_listener* listener)
{
  auto iter = std::find(_ui_listeners.begin(), _ui_listeners.end(), listener);
  if(iter != _ui_listeners.end()) _ui_listeners.erase(iter);
}

void 
plugin_gui::remove_plugin_listener(int index, plugin_listener* listener)
{
  auto iter = std::find(_plugin_listeners[index].begin(), _plugin_listeners[index].end(), listener);
  if (iter != _plugin_listeners[index].end()) _plugin_listeners[index].erase(iter);
}

void
plugin_gui::state_loaded()
{
  int param_global = 0;
  for (int m = 0; m < _desc->plugin->modules.size(); m++)
  {
    auto const& module = _desc->plugin->modules[m];
    for (int mi = 0; mi < module.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
        for (int pi = 0; pi < module.params[p].slot_count; pi++)
          ui_changed(param_global++, (*_ui_state)[m][mi][p][pi]);
  }
}

plugin_gui::
plugin_gui(plugin_desc const* desc, jarray<plain_value, 4>* ui_state) :
_desc(desc), _ui_state(ui_state), _plugin_listeners(desc->param_count)
{
  int mi = 0;
  auto const& topo = *_desc->plugin;
  _grid = &make_component<grid_component>(topo.dimension);
  setOpaque(true);
  addAndMakeVisible(_grid);
  for(int m = 0; m < _desc->plugin->modules.size(); m++)
  {
    auto const& module = _desc->plugin->modules[m];
    add_module_slots(module, &_desc->modules[mi]);
    mi += module.slot_count;
  }
  setSize(topo.gui_default_width, topo.gui_default_width / topo.gui_aspect_ratio);
}

template <class T, class... U> T& 
plugin_gui::make_component(U&&... args) 
{
  auto component = std::make_unique<T>(std::forward<U>(args)...);
  T* result = component.get();
  _components.emplace_back(std::move(component));
  return *result;
}

void 
plugin_gui::add_sections(GroupComponent& container, module_topo const& module)
{
  auto& grid = make_component<grid_component>(module.dimension);
  container.addAndMakeVisible(grid);
  for (int s = 0; s < module.sections.size(); s++)
  {
    auto const& section = module.sections[s];
    auto& group = make_component<group_component>();
    group.setText(section.name);
    grid.add(group, section.position);
  }
}

void 
plugin_gui::add_module_slots(module_topo const& module, module_desc const* slots)
{  
  if (module.slot_count == 1)
  {
    auto& group = make_component<group_component>();
    group.setText(slots[0].name);
    add_sections(group, *slots[0].module);
    _grid->add(group, module.position);
    return;
  }

  switch (module.layout)
  {
  case gui_layout::vertical:
  case gui_layout::horizontal:
  {
    bool is_vertical = module.layout == gui_layout::vertical;
    int slow_rows = is_vertical ? module.slot_count : 1;
    int slow_columns = is_vertical? 1: module.slot_count;
    auto& slot_grid = make_component<grid_component>(gui_dimension { slow_rows, slow_columns });
    for (int i = 0; i < module.slot_count; i++)
    {
      auto& group = make_component<group_component>();
      group.setText(slots[i].name);
      add_sections(group, *slots[i].module);
      int row = (is_vertical? i: 0);
      int column = (is_vertical ? 0: i);
      slot_grid.add(group, { row, column, 1, 1 });
    }
    _grid->add(slot_grid, module.position);
    break;
  }
  default:
    assert(false);
    break;
  }
}

}