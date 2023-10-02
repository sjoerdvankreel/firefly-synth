#include <plugin_base/io.hpp>
#include <plugin_base/topo.hpp>
#include <plugin_base/gui.hpp>
#include <plugin_base/gui_lnf.hpp>

#include <vector>
#include <algorithm>

using namespace juce;

namespace plugin_base {

static inline int constexpr label_width = 80;
static inline int constexpr label_height = 15;
static inline int constexpr groupbox_padding = 6;
static inline int constexpr groupbox_padding_top = 16;

static std::unique_ptr<gui_lnf> _lnf = {};

void 
gui_terminate()
{ 
  LookAndFeel::setDefaultLookAndFeel(nullptr);
  _lnf.reset();
  shutdownJuce_GUI(); 
}

void 
gui_init()
{ 
  initialiseJuce_GUI(); 
  _lnf = std::make_unique<gui_lnf>();
  LookAndFeel::setDefaultLookAndFeel(_lnf.get());
}

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
    case param_label_justify::near: return Justification::topLeft;
    case param_label_justify::far: return Justification::bottomLeft;
    case param_label_justify::center: return Justification::centredLeft;
    default: break; }
    break;
  case param_label_align::right:
    switch (justify) {
    case param_label_justify::near: return Justification::topRight;
    case param_label_justify::far: return Justification::bottomRight;
    case param_label_justify::center: return Justification::centredRight;
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
private:
  std::vector<int> _enabled_values = {};
  std::vector<int> _enabled_indices = {};
  std::vector<int> _visibility_values = {};
  std::vector<int> _visibility_indices = {};

  bool select_ui_state(
    std::vector<int> const& indices, 
    std::vector<int>& values, param_ui_state_selector selector);
  void setup_ui_state_indices(
    std::vector<int> const& topo_indices, std::vector<int>& indices);

public:
  virtual ~param_base();
  void plugin_changed(int index, plain_value plain) override final;

protected:
  plugin_gui* const _gui;
  param_desc const* const _param;  
  module_desc const* const _module;

  void init();
  virtual void own_param_changed(plain_value plain) = 0;
  param_base(plugin_gui* gui, module_desc const* module, param_desc const* param);
};

class param_name_label:
public param_base, public Label
{
protected:
  void own_param_changed(plain_value plain) override {}
public:
  param_name_label(plugin_gui* gui, module_desc const* module, param_desc const* param):
  param_base(gui, module, param), Label() { setText(param->name, dontSendNotification);  }
};

class param_value_label:
public param_base, public Label
{
  bool const _both;
protected:
  void own_param_changed(plain_value plain) override final;
public:
  param_value_label(plugin_gui* gui, module_desc const* module, param_desc const* param, bool both):
  param_base(gui, module, param), Label(), _both(both) { init(); }
};

class param_slider:
public param_base, public Slider
{
protected:
  void own_param_changed(plain_value plain) override final
  { setValue(_param->param->plain_to_raw(plain), dontSendNotification); }

public: 
  param_slider(plugin_gui* gui, module_desc const* module, param_desc const* param);
  void stoppedDragging() override { _gui->ui_end_changes(_param->global); }
  void startedDragging() override { _gui->ui_begin_changes(_param->global); }
  void valueChanged() override { _gui->ui_changing(_param->global, _param->param->raw_to_plain(getValue())); }
};

class param_combobox :
public param_base, public ComboBox, public ComboBox::Listener
{
protected:
  void own_param_changed(plain_value plain) override final
  { setSelectedItemIndex(plain.step() - _param->param->min); }

public:
  ~param_combobox() { removeListener(this); }
  param_combobox(plugin_gui* gui, module_desc const* module, param_desc const* param);
  void comboBoxChanged(ComboBox*) override final
  { _gui->ui_changed(_param->global, _param->param->raw_to_plain(getSelectedItemIndex() + _param->param->min)); }
};

class param_toggle_button :
public param_base, public ToggleButton, public Button::Listener
{
  bool _checked = false;
protected:
  void own_param_changed(plain_value plain) override final;

public:
  void buttonClicked(Button*) override {}
  void buttonStateChanged(Button*) override;
  ~param_toggle_button() { removeListener(this); }
  param_toggle_button(plugin_gui* gui, module_desc const* module, param_desc const* param);
};

class param_textbox :
public param_base, public TextEditor, public TextEditor::Listener
{
  std::string _last_parsed;
protected:
  void own_param_changed(plain_value plain) override final
  { setText(_last_parsed = _param->param->plain_to_text(plain), false); }

public:
  void textEditorTextChanged(TextEditor&) override;
  void textEditorFocusLost(TextEditor&) override { setText(_last_parsed, false); }
  void textEditorReturnKeyPressed(TextEditor&) override { setText(_last_parsed, false); }
  void textEditorEscapeKeyPressed(TextEditor&) override { setText(_last_parsed, false); }

  ~param_textbox() { removeListener(this); }
  param_textbox(plugin_gui* gui, module_desc const* module, param_desc const* param);
};

param_base::
param_base(plugin_gui* gui, module_desc const* module, param_desc const* param): 
_gui(gui), _module(module), _param(param)
{ 
  _gui->add_plugin_listener(_param->global, this);
  setup_ui_state_indices(_param->param->enabled_indices, _enabled_indices);
  setup_ui_state_indices(_param->param->visibility_indices, _visibility_indices);
}

param_base::
~param_base()
{
  for(int i = 0; i < _visibility_indices.size(); i++)
    _gui->remove_plugin_listener(_visibility_indices[i], this);
  for (int i = 0; i < _enabled_indices.size(); i++)
    _gui->remove_plugin_listener(_enabled_indices[i], this);
  _gui->remove_plugin_listener(_param->global, this);
}

bool 
param_base::select_ui_state(
  std::vector<int> const& indices, 
  std::vector<int>& values, param_ui_state_selector selector)
{
  values.clear();
  for (int i = 0; i < indices.size(); i++)
  {
    auto const& mapping = _gui->desc()->mappings[indices[i]];
    values.push_back(mapping.value_at(_gui->ui_state()).step());
  }
  return selector(values);
}

void
param_base::init()
{
  // Must be called by subclass constructor as we dynamic_cast to Component inside.
  auto const& own_mapping = _gui->desc()->mappings[_param->global];
  plugin_changed(_param->global, own_mapping.value_at(_gui->ui_state()));
  if (_enabled_indices.size() != 0)
  {
    auto const& enabled_mapping = _gui->desc()->mappings[_enabled_indices[0]];
    plugin_changed(_enabled_indices[0], enabled_mapping.value_at(_gui->ui_state()));
  }
  if (_visibility_indices.size() != 0)
  {
    auto const& visibility_mapping = _gui->desc()->mappings[_visibility_indices[0]];
    plugin_changed(_visibility_indices[0], visibility_mapping.value_at(_gui->ui_state()));
  }
}

void 
param_base::setup_ui_state_indices(
  std::vector<int> const& topo_indices, std::vector<int>& indices)
{
  for (int i = 0; i < topo_indices.size(); i++)
  {
    auto const& param_topo_to_index = _gui->desc()->param_topo_to_index;
    auto const& slots = param_topo_to_index[_module->topo][_module->slot][topo_indices[i]];
    bool single_slot = _module->module->params[topo_indices[i]].slot_count == 1;
    int state_index = single_slot ? slots[0] : slots[_param->slot];
    indices.push_back(state_index);
    _gui->add_plugin_listener(state_index, this);
  }
}

void
param_base::plugin_changed(int index, plain_value plain)
{
  if (index == _param->global)
  {
    own_param_changed(plain);
    return;
  }
  
  auto enabled_iter = std::find(_enabled_indices.begin(), _enabled_indices.end(), index);
  if (enabled_iter != _enabled_indices.end())
    dynamic_cast<Component&>(*this).setEnabled(select_ui_state(
      _enabled_indices, _enabled_values, _param->param->enabled_selector));

  auto visibility_iter = std::find(_visibility_indices.begin(), _visibility_indices.end(), index);
  if (visibility_iter != _visibility_indices.end())
    dynamic_cast<Component&>(*this).setVisible(select_ui_state(
      _visibility_indices, _visibility_values, _param->param->visibility_selector));
}

void
param_textbox::textEditorTextChanged(TextEditor&)
{
  plain_value plain;
  std::string text(getText().toStdString());
  if (!_param->param->text_to_plain(text, plain)) return;
  _last_parsed = text;
  _gui->ui_changed(_param->global, plain);
}

void
param_value_label::own_param_changed(plain_value plain)
{ 
  std::string text = _param->param->plain_to_text(plain);
  if(_both) text = _param->name + " " + text;
  setText(text, dontSendNotification); 
}

void 
param_toggle_button::own_param_changed(plain_value plain)
{
  _checked = plain.step() != 0;
  setToggleState(plain.step() != 0, dontSendNotification);
}

void 
param_toggle_button::buttonStateChanged(Button*)
{ 
  if(_checked == getToggleState()) return;
  plain_value plain = _param->param->raw_to_plain(getToggleState() ? 1 : 0);
  _checked = getToggleState();
  _gui->ui_changed(_param->global, plain);
}

param_textbox::
param_textbox(plugin_gui* gui, module_desc const* module, param_desc const* param) :
param_base(gui, module, param), TextEditor()
{
  addListener(this);
  init();
}

param_toggle_button::
param_toggle_button(plugin_gui* gui, module_desc const* module, param_desc const* param):
param_base(gui, module, param), ToggleButton()
{ 
  auto value = param->param->default_plain();
  _checked = value.step() != 0;
  addListener(this);
  init();
}

param_combobox::
param_combobox(plugin_gui* gui, module_desc const* module, param_desc const* param) :
param_base(gui, module, param), ComboBox()
{
  switch (param->param->type)
  {
  case param_type::name:
    for (int i = 0; i < param->param->names.size(); i++)
      addItem(param->param->names[i], i + 1);
    break;
  case param_type::item:
    for (int i = 0; i < param->param->items.size(); i++)
      addItem(param->param->items[i].name, i + 1);
    break;
  case param_type::step:
    for (int i = param->param->min; i <= param->param->max; i++)
      addItem(std::to_string(i), param->param->min + i + 1);
    break;
  default:
    assert(false);
    break;
  }
  addListener(this);
  setEditableText(false);
  init();
}

param_slider::
param_slider(plugin_gui* gui, module_desc const* module, param_desc const* param) :
param_base(gui, module, param), Slider()
{
  switch (param->param->edit)
  {
  case param_edit::knob: setSliderStyle(Slider::RotaryVerticalDrag); break;
  case param_edit::vslider: setSliderStyle(Slider::LinearVertical); break;
  case param_edit::hslider: setSliderStyle(Slider::LinearHorizontal); break;
  default: assert(false); break;
  }
  setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
  if (!param->param->is_real()) setRange(param->param->min, param->param->max, 1);
  else setNormalisableRange(
    NormalisableRange<double>(param->param->min, param->param->max,
    [this](double s, double e, double v) { return _param->param->normalized_to_raw(normalized_value(v)); },
    [this](double s, double e, double v) { return _param->param->raw_to_normalized(v).value(); }));
  setDoubleClickReturnValue(true, param->param->default_raw(), ModifierKeys::noModifiers);
  param_base::init();
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
  if(_desc->params[index]->param->dir == param_dir::input)
    for (int i = 0; i < _ui_listeners.size(); i++)
      _ui_listeners[i]->ui_changed(index, plain);
}

void 
plugin_gui::ui_begin_changes(int index)
{
  if (_desc->params[index]->param->dir == param_dir::input)
    for(int i = 0; i < _ui_listeners.size(); i++)
      _ui_listeners[i]->ui_begin_changes(index);
}

void
plugin_gui::ui_end_changes(int index)
{
  if (_desc->params[index]->param->dir == param_dir::input)
    for (int i = 0; i < _ui_listeners.size(); i++)
      _ui_listeners[i]->ui_end_changes(index);
}

void
plugin_gui::ui_changing(int index, plain_value plain)
{
  if (_desc->params[index]->param->dir == param_dir::input)
    for (int i = 0; i < _ui_listeners.size(); i++)
      _ui_listeners[i]->ui_changing(index, plain);
}

void
plugin_gui::plugin_changed(int index, plain_value plain)
{
  for(int i = 0; i < _plugin_listeners[index].size(); i++)
    _plugin_listeners[index][i]->plugin_changed(index, plain);
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
  setOpaque(true);
  auto const& topo = *_desc->plugin;
  addAndMakeVisible(&make_container());
  setSize(topo.gui_default_width, topo.gui_default_width * topo.gui_aspect_ratio_height / topo.gui_aspect_ratio_width);
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
plugin_gui::make_container()
{
  auto& result = make_component<grid_component>(gui_dimension({ -20, 1 }, { 1 }));
  result.add(make_top_bar(), { 0, 0 });
  result.add(make_content(), { 1, 0 });
  return result;
}

// TODO just for now.
// Give plugin more control over this.
Component&
plugin_gui::make_top_bar()
{
  auto& result = make_component<grid_component>(gui_dimension({ 1 }, { -100, -100 }));

  auto& save = make_component<TextButton>();
  save.setButtonText("Save");
  result.add(save, { 0, 1 });
  save.onClick = [this]() {
    int flags = FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting;
    FileChooser* chooser = new FileChooser("Save", File(), String("*.") + _desc->plugin->preset_extension, true, false, this);
    chooser->launchAsync(flags, [this](FileChooser const& chooser) {
      auto path = chooser.getResult().getFullPathName();
      delete& chooser;
      if (path.length() == 0) return;
      plugin_io(_desc).save_file(path.toStdString(), *_ui_state);
    });};

  auto& load = make_component<TextButton>();
  load.setButtonText("Load");
  result.add(load, { 0, 0 });
  load.onClick = [this]() {
    int flags = FileBrowserComponent::openMode;
    FileChooser* chooser = new FileChooser("Load", File(), String("*.") + _desc->plugin->preset_extension, true, false, this);
    chooser->launchAsync(flags, [this](FileChooser const& chooser) {
      auto path = chooser.getResult().getFullPathName();
      delete &chooser;
      if(path.length() == 0) return;

      auto icon = MessageBoxIconType::WarningIcon;
      auto result = plugin_io(_desc).load_file(path.toStdString(), *_ui_state);
      if(result.error.size())
      {
        auto options = MessageBoxOptions::makeOptionsOk(icon, "Error", result.error, String(), this);
        AlertWindow::showAsync(options, nullptr);
        return;
      }

      state_loaded();
      if (result.warnings.size())
      {
        String warnings;
        for(int i = 0; i < result.warnings.size() && i < 5; i++)
          warnings += String(result.warnings[i]) + "\n";
        if(result.warnings.size() > 5)
          warnings += String(std::to_string(result.warnings.size() - 5)) + " more...\n";
        auto options = MessageBoxOptions::makeOptionsOk(icon, "Warning", warnings, String(), this);
        AlertWindow::showAsync(options, nullptr);
      }
    });};

  return result;
}

Component&
plugin_gui::make_content()
{
  auto& result = make_component<grid_component>(_desc->plugin->dimension);
  for (auto iter = _desc->modules.begin(); iter != _desc->modules.end(); iter += iter->module->slot_count)
    result.add(make_modules(&(*iter)), iter->module->position);
  return result;
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
  Component* result = nullptr;
  switch (param.param->edit)
  {
  case param_edit::knob:
  case param_edit::hslider:
  case param_edit::vslider:
    result = &make_component<param_slider>(this, &module, &param); break;
  case param_edit::text:
    result = &make_component<param_textbox>(this, &module, &param); break;
  case param_edit::list:
    result = &make_component<param_combobox>(this, &module, &param); break;
  case param_edit::toggle:
    result = &make_component<param_toggle_button>(this, &module, &param); break;
  default:
    assert(false);
    return *((Component*)nullptr);
  }
  result->setEnabled(param.param->dir == param_dir::input);
  return *result;
}

Component&
plugin_gui::make_param_label(module_desc const& module, param_desc const& param)
{
  Label* result = {};
  auto contents = param.param->label_contents;
  switch (contents)
  {
  case param_label_contents::name:
    result = &make_component<param_name_label>(this, &module, &param);
    break;
  case param_label_contents::both:
  case param_label_contents::value:
    result = &make_component<param_value_label>(this, &module, &param, contents == param_label_contents::both);
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