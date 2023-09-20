#include <plugin_base/io.hpp>
#include <plugin_base/gui.hpp>
#include <plugin_base/topo.hpp>

#include <vector>

using namespace juce;

namespace plugin_base {

static inline int constexpr label_width = 40;
static inline int constexpr label_height = 15;
static inline int constexpr groupbox_padding = 6;
static inline int constexpr groupbox_padding_top = 16;

void
ui_listener::ui_changed(int index, plain_value plain)
{
  ui_begin_changes(index);
  ui_changing(index, plain);
  ui_end_changes(index);
}

static Justification 
justification_type(param_label_align align, param_label_justify justify)
{
  switch (align)
  {
  case param_label_align::top:
  case param_label_align::bottom:
    switch (justify) {
    case param_label_justify::center: return Justification::centred;
    case param_label_justify::near: return Justification::centredLeft;
    case param_label_justify::far: return Justification::centredRight;
    default: break; }
    break;
  case param_label_align::left:
    switch (justify) {
    case param_label_justify::near: return Justification::topRight;
    case param_label_justify::far: return Justification::bottomRight;
    case param_label_justify::center: return Justification::centredRight;
    default: break; }
    break;
  case param_label_align::right:
    switch (justify) {
    case param_label_justify::near: return Justification::topLeft;
    case param_label_justify::far: return Justification::bottomLeft;
    case param_label_justify::center: return Justification::centredLeft;
    default: break; }
    break;
  default:
    break;
  }
  assert(false);
  return Justification::centred;
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
  { setText(desc->name, dontSendNotification);  }
};

class param_value_label:
public param_base, public Label
{
  bool const _both;
public:
  void plugin_changed(plain_value plain) override final;
  param_value_label(plugin_gui* gui, param_desc const* desc, bool both, plain_value initial):
  param_base(gui, desc), Label(), _both(both) { plugin_changed(initial); }
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
  if(_both) text = _desc->name + " " + text;
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
  case param_edit::knob: setSliderStyle(Slider::RotaryVerticalDrag); break;
  case param_edit::vslider: setSliderStyle(Slider::LinearVertical); break;
  case param_edit::hslider: setSliderStyle(Slider::LinearHorizontal); break;
  default: assert(false); break;
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
  Rectangle<int> bounds(getLocalBounds());
  Rectangle<int> child_bounds(
    bounds.getX() + groupbox_padding, 
    bounds.getY() + groupbox_padding_top, 
    bounds.getWidth() - 2 * groupbox_padding, 
    bounds.getHeight() - groupbox_padding - groupbox_padding_top);
  assert(getNumChildComponents() < 2);
  if (getNumChildComponents() == 1)
    getChildComponent(0)->setBounds(child_bounds);
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
  void add(Component& child, bool vertical, int position) 
  { add(child, gui_position { vertical? position: 0, vertical? 0: position }); }

  grid_component(gui_dimension const& dimension) :
  _dimension(dimension) { }
  grid_component(bool vertical, int count) :
  _dimension(vertical ? count : 1, vertical ? 1 : count) {}
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

  for(int i = 0; i < _dimension.row_sizes.size(); i++)
    if(_dimension.row_sizes[i] > 0)
      grid.templateRows.add(Grid::Fr(_dimension.row_sizes[i]));
    else if (_dimension.row_sizes[i] < 0)
      grid.templateRows.add(Grid::Px(-_dimension.row_sizes[i]));
    else
      assert(false);

  for(int i = 0; i < _dimension.column_sizes.size(); i++)
    if(_dimension.column_sizes[i] > 0)
      grid.templateColumns.add(Grid::Fr(_dimension.column_sizes[i]));
    else if(_dimension.column_sizes[i] < 0)
      grid.templateColumns.add(Grid::Px(-_dimension.column_sizes[i]));
    else
      assert(false);

  for (int i = 0; i < _positions.size(); i++)
  {
    GridItem item(getChildComponent(i));
    item.row.start = _positions[i].row + 1;
    item.column.start = _positions[i].column + 1;
    item.row.end = _positions[i].row + 1 + _positions[i].row_span;
    item.column.end = _positions[i].column + 1 + _positions[i].column_span;
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
  _grid = &make_component<grid_component>(_desc->plugin->dimension);
  for(auto iter = _desc->modules.begin(); iter != _desc->modules.end(); iter += iter->module->slot_count)
    _grid->add(make_modules(&(*iter)), iter->module->position);

  auto const& topo = *_desc->plugin;
  setOpaque(true);
  addAndMakeVisible(_grid);
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

Component&
plugin_gui::make_modules(module_desc const* slots)
{  
  if (slots[0].module->slot_count == 1)
    return make_single_module(slots[0], false);
  else
    return make_multi_module(slots);  
}

Component&
plugin_gui::make_single_module(module_desc const& slot, bool tabbed)
{
  if(tabbed) return make_sections(slot);
  auto& result = make_component<group_component>();
  result.setText(slot.name);
  result.addAndMakeVisible(make_sections(slot));
  return result;
}

Component&
plugin_gui::make_multi_module(module_desc const* slots)
{
  auto make_single = [this](module_desc const& m, bool tabbed) -> Component& {
    return make_single_module(m, tabbed); };
  return make_multi_slot(*slots[0].module, slots, make_single);
}

Component&
plugin_gui::make_sections(module_desc const& module)
{
  auto const& topo = *module.module;
  auto& result = make_component<grid_component>(topo.dimension);
  for (int s = 0; s < topo.sections.size(); s++)
    result.add(make_section(module, topo.sections[s]), topo.sections[s].position);
  return result;
}

Component&
plugin_gui::make_section(module_desc const& module, section_topo const& section)
{
  auto const& params = module.params;
  auto& grid = make_component<grid_component>(section.dimension);
  for (auto iter = params.begin(); iter != params.end(); iter += iter->param->slot_count)
    if(iter->param->section == section.section)
      grid.add(make_params(module, &(*iter)), iter->param->position);
  
  if(module.module->sections.size() == 1)
    return grid;
  auto& result = make_component<group_component>();
  result.setText(section.name);
  result.addAndMakeVisible(grid);
  return result;
}

Component&
plugin_gui::make_params(module_desc const& module, param_desc const* params)
{
  if (params[0].param->slot_count == 1)
    return make_single_param(module, params[0]);
  else
    return make_multi_param(module, params);
}

Component&
plugin_gui::make_multi_param(module_desc const& module, param_desc const* slots)
{
  auto make_single = [this, &module](param_desc const& p, bool tabbed) -> Component& {
    return make_single_param(module, p); };
  return make_multi_slot(*slots[0].param, slots, make_single);
}

Component& 
plugin_gui::make_param_label_edit(
  module_desc const& module, param_desc const& param, 
  gui_dimension const& dimension, gui_position const& label_position, gui_position const& edit_position)
{
  auto& result = make_component<grid_component>(dimension);
  result.add(make_param_label(module, param), label_position);
  result.add(make_param_edit(module, param), edit_position);
  return result;
}

Component&
plugin_gui::make_single_param(module_desc const& module, param_desc const& param)
{
  if(param.param->label_contents == param_label_contents::none)
    return make_param_edit(module, param);

  switch(param.param->label_align)
  {
  case param_label_align::top:
    return make_param_label_edit(module, param, gui_dimension({ -label_height, 1 }, { 1 }), { 0, 0 }, { 1, 0 });
  case param_label_align::bottom:
    return make_param_label_edit(module, param, gui_dimension({ 1, -label_height, }, { 1 }), { 1, 0 }, { 0, 0 });
  case param_label_align::left:
    return make_param_label_edit(module, param, gui_dimension({ 1 }, { -label_width, 1 }), { 0, 0 }, { 0, 1 });
  case param_label_align::right:
    return make_param_label_edit(module, param, gui_dimension({ 1 }, { 1, -label_width }), { 0, 1 }, { 0, 0 });
  default:
    assert(false);
    return *((Component*)nullptr);
  }
}

Component&
plugin_gui::make_param_edit(module_desc const& module, param_desc const& param)
{
  plain_value initial((*_ui_state)[module.topo][module.slot][param.topo][param.slot]);
  switch (param.param->edit)
  {
  case param_edit::knob:
  case param_edit::hslider:
  case param_edit::vslider:
    return make_component<param_slider>(this, &param, initial);
  case param_edit::text:
    return make_component<param_textbox>(this, &param, initial);
  case param_edit::list:
    return make_component<param_combobox>(this, &param, initial);
  case param_edit::toggle:
    return make_component<param_toggle_button>(this, &param, initial);
  default:
    assert(false);
    return *((Component*)nullptr);
  }
}

Component&
plugin_gui::make_param_label(module_desc const& module, param_desc const& param)
{
  Label* result = {};
  auto contents = param.param->label_contents;
  plain_value initial((*_ui_state)[module.topo][module.slot][param.topo][param.slot]);
  switch (contents)
  {
  case param_label_contents::name:
    result = &make_component<param_name_label>(&param);
    break;
  case param_label_contents::both:
  case param_label_contents::value:
    result = &make_component<param_value_label>(this, &param, contents == param_label_contents::both, initial);
    break;
  default:
    assert(false);
    break;
  }
  result->setJustificationType(justification_type(param.param->label_align, param.param->label_justify));
  return *result;
}

template <class Topo, class Slot, class MakeSingle>
Component&
plugin_gui::make_multi_slot(Topo const& topo, Slot const* slots, MakeSingle make_single)
{
  switch (topo.layout)
  {
  case gui_layout::vertical:
  case gui_layout::horizontal:
  {
    bool vertical = topo.layout == gui_layout::vertical;
    auto& result = make_component<grid_component>(vertical, topo.slot_count);
    for (int i = 0; i < topo.slot_count; i++)
      result.add(make_single(slots[i], false), vertical, i);
    return result;
  }
  case gui_layout::tabbed:
  {
    auto& result = make_component<TabbedComponent>(TabbedButtonBar::Orientation::TabsAtTop);
    for (int i = 0; i < topo.slot_count; i++)
      result.addTab(slots[i].name, Colours::black, &make_single(slots[i], true), false);
    return result;
  }
  default:
    assert(false);
    return *((Component*)nullptr);
  }
}

}