#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/utility.hpp>
#include <plugin_base/gui/controls.hpp>

#include <juce_gui_basics/detail/juce_LookAndFeelHelpers.h>
#include <cassert>

using namespace juce;

namespace plugin_base {

static int const conic_count = 256;

static void
draw_tabular_cell_bg(Graphics& g, Component* c, int radius)
{
  g.setColour(Colours::black.withAlpha(0.5f));
  g.fillRoundedRectangle(c->getLocalBounds().reduced(1).toFloat(), radius);
}

static int
get_combobox_mod_target_indicator_width(ComboBox const& box, Font const& font)
{
  param_combobox const* param_cb = dynamic_cast<param_combobox const*>(&box);
  if (param_cb == nullptr) return 0;
  auto drop_action = param_cb->get_drop_target_action();
  if (drop_action == drop_target_action::none) return 0;
  return font.getStringWidth("[N/A]") + 2;
}

static void 
draw_conic_arc(
  Graphics&g, float left, float top, float size, float start_angle, 
  float end_angle, Colour color1, Colour color2, int segment_count, 
  float min, float max, float stroke)
{
  float overlap = 0.01;
  float angle_range = end_angle - start_angle;
  for (int i = (int)(min * segment_count); i < segment_count; i++)
  {
    Path path;
    if ((float)i / segment_count >= max) break;
    g.setColour(color1.interpolatedWith(color2, (float)i / (segment_count - 1)));
    float this_start_angle = start_angle + (float)i / segment_count * angle_range;
    float this_end_angle = start_angle + (float)(i + 1) / segment_count * angle_range;
    if (i < segment_count - 1) this_end_angle += overlap;
    path.addArc(left, top, size, size, this_start_angle, this_end_angle, true);
    g.strokePath(path, PathStrokeType(stroke));
  }
}

static Colour
override_color_if_present(var const& json, std::string const& name, Colour const& current)
{
  auto juce_name = String(name);
  if (!json.hasProperty(juce_name)) return current;
  auto color_text = json[StringRef(juce_name)].toString();
  return Colour::fromString(color_text);
}

static section_topo_gui_theme_settings 
override_settings(section_topo_gui_theme_settings const& base, var const& json)
{
  section_topo_gui_theme_settings result = section_topo_gui_theme_settings(base);
  if (json.hasProperty("tab_width")) result.tab_width = (int)json["tab_width"];
  if (json.hasProperty("header_width")) result.header_width = (int)json["header_width"];
  return result;
}

static gui_colors 
override_colors(gui_colors const& base, var const& json)
{
  gui_colors result = gui_colors(base);
  result.tab_text = override_color_if_present(json, "tab_text", result.tab_text);
  result.tab_text_inactive = override_color_if_present(json, "tab_text_inactive", result.tab_text_inactive);
  result.tab_button = override_color_if_present(json, "tab_button", result.tab_button);
  result.tab_header = override_color_if_present(json, "tab_header", result.tab_header);
  result.graph_grid = override_color_if_present(json, "graph_grid", result.graph_grid);
  result.graph_text = override_color_if_present(json, "graph_text", result.graph_text);
  result.graph_mod_indicator = override_color_if_present(json, "graph_mod_indicator", result.graph_mod_indicator);
  result.graph_background = override_color_if_present(json, "graph_background", result.graph_background);
  result.graph_area = override_color_if_present(json, "graph_area", result.graph_area);
  result.graph_line = override_color_if_present(json, "graph_line", result.graph_line);
  result.bubble_outline = override_color_if_present(json, "bubble_outline", result.bubble_outline);  
  result.slider_background = override_color_if_present(json, "slider_background", result.slider_background);
  result.slider_highlight = override_color_if_present(json, "slider_highlight", result.slider_highlight);
  result.slider_shadow = override_color_if_present(json, "slider_shadow", result.slider_shadow);
  result.slider_automation = override_color_if_present(json, "slider_automation", result.slider_automation);
  result.slider_can_modulate = override_color_if_present(json, "slider_can_modulate", result.slider_can_modulate);
  result.section_outline = override_color_if_present(json, "section_outline", result.section_outline);
  result.section_background = override_color_if_present(json, "section_background", result.section_background);
  result.edit_text = override_color_if_present(json, "edit_text", result.edit_text);
  result.label_text = override_color_if_present(json, "label_text", result.label_text);
  result.table_header = override_color_if_present(json, "table_header", result.table_header);
  result.control_tick = override_color_if_present(json, "control_tick", result.control_tick);
  result.control_text = override_color_if_present(json, "control_text", result.control_text);
  result.control_outline = override_color_if_present(json, "control_outline", result.control_outline);
  result.control_background = override_color_if_present(json, "control_background", result.control_background);
  result.scrollbar_thumb = override_color_if_present(json, "scrollbar_thumb", result.scrollbar_thumb);
  result.scrollbar_background = override_color_if_present(json, "scrollbar_background", result.scrollbar_background);
  result.popup_text = override_color_if_present(json, "popup_text", result.popup_text);
  return gui_colors(result); 
}

lnf::
lnf(plugin_desc const* desc, std::string const& theme, int custom_section, int module_section, int module) :
_theme(theme), _desc(desc), _custom_section(custom_section), _module_section(module_section), _module(module)
{
  assert(module_section == -1 || module >= 0);
  assert(custom_section == -1 || module == -1);

  auto theme_folder = get_resource_location(desc->config) / resource_folder_themes / _theme;
  auto font_path = theme_folder / "font.ttf";
  std::vector<char> typeface = file_load(font_path);

  // If resource data is missing we go with the default font.
  if(typeface.size()) _typeface = Typeface::createSystemTypefaceFor(typeface.data(), typeface.size());
  assert(-1 <= module && module < (int)_desc->plugin->modules.size());

  auto theme_path = theme_folder / "theme.json";
  std::vector<char> theme_contents = file_load(theme_path);
  theme_contents.push_back('\0');
  assert(theme_contents.size());
  juce::String theme_string = juce::String(theme_contents.data());
  var theme_json;
  auto parse_result = JSON::parse(theme_string, theme_json);
  assert(parse_result.ok());
  init_theme(theme_folder, theme_json);

  auto control_text_high = colors().control_text.brighter(_global_settings.lighten);
  auto control_bg_high = colors().control_background.brighter(_global_settings.lighten);

  setColour(Label::ColourIds::textColourId, colors().label_text);

  setColour(ToggleButton::ColourIds::textColourId, colors().control_text);
  setColour(ToggleButton::ColourIds::tickColourId, colors().control_tick);
  
  setColour(TextButton::ColourIds::textColourOnId, colors().label_text);
  setColour(TextButton::ColourIds::textColourOffId, colors().label_text);
  setColour(TextButton::ColourIds::buttonOnColourId, control_bg_high);
  setColour(TextButton::ColourIds::buttonColourId, colors().control_background);

  setColour(TextEditor::ColourIds::textColourId, colors().edit_text);
  setColour(TextEditor::ColourIds::backgroundColourId, colors().control_background);
  setColour(CaretComponent::ColourIds::caretColourId, colors().edit_text);

  setColour(TabbedButtonBar::ColourIds::tabTextColourId, colors().tab_text);
  setColour(TabbedComponent::ColourIds::outlineColourId, Colours::transparentBlack);
  setColour(TabbedButtonBar::ColourIds::tabOutlineColourId, Colours::transparentBlack);
  setColour(TabbedButtonBar::ColourIds::frontOutlineColourId, Colours::transparentBlack);

  setColour(ComboBox::ColourIds::textColourId, colors().control_text);
  setColour(ComboBox::ColourIds::arrowColourId, colors().control_tick);
  setColour(ComboBox::ColourIds::outlineColourId, colors().control_outline);
  setColour(ComboBox::ColourIds::backgroundColourId, colors().control_background);
  setColour(ComboBox::ColourIds::focusedOutlineColourId, colors().control_outline);

  setColour(ScrollBar::ColourIds::thumbColourId, colors().scrollbar_thumb);
  setColour(ScrollBar::ColourIds::backgroundColourId, colors().scrollbar_background);

  setColour(PopupMenu::ColourIds::textColourId, colors().popup_text);
  setColour(PopupMenu::ColourIds::highlightedTextColourId, colors().control_text);
  setColour(PopupMenu::ColourIds::backgroundColourId, colors().control_background);
  setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, colors().control_background.brighter(_global_settings.lighten));
}

void 
lnf::init_theme(std::filesystem::path const& theme_folder, var const& json)
{
  // keep these in sync with required elements
  if(!json.hasProperty("default_colors")) return;
  if (!json.hasProperty("global_settings")) return;
  if (!json.hasProperty("default_settings")) return; 

  if (json.hasProperty("graph_background_images"))
  {
    var graph_background_images = json["graph_background_images"];
    for (int i = 0; i < graph_background_images.size(); i++)
    {
      var this_bg_image = graph_background_images[i];
      if (this_bg_image.hasProperty("graph") && this_bg_image.hasProperty("image"))
      {
        std::string image = this_bg_image["image"].toString().toStdString();
        if(!image.empty())
        {
          std::string graph = this_bg_image["graph"].toString().toStdString();
          std::string image_path = (theme_folder / image).string();
          _global_settings.graph_background_images[graph] = image_path;
        }
      }
    }
  }

  assert(json.hasProperty("default_colors"));
  var default_colors = json["default_colors"];
  _default_colors = override_colors(_default_colors, default_colors);

  assert(json.hasProperty("default_settings"));
  var default_settings = json["default_settings"];
  _default_settings = override_settings(_default_settings, default_settings);

  if (json.hasProperty("overrides"))
  {
    var overrides = json["overrides"];
    assert(overrides.isArray());
    for (int i = 0; i < overrides.size(); i++)
    {
      var this_override = overrides[i];
      auto this_settings = _default_settings;
      auto this_colors = gui_colors(_default_colors);
      if(this_override.hasProperty("colors")) this_colors = override_colors(_default_colors, this_override["colors"]);
      if (this_override.hasProperty("settings")) this_settings = override_settings(_default_settings, this_override["settings"]);
      if (this_override.hasProperty("custom_sections"))
      {
        var custom_sections = this_override["custom_sections"];
        assert(custom_sections.isArray());
        for(int j = 0; j < custom_sections.size(); j++)
        {
          _section_settings[custom_sections[j].toString().toStdString()] = this_settings;
          _section_colors[custom_sections[j].toString().toStdString()] = gui_colors(this_colors);
        }
      }
      if (this_override.hasProperty("module_sections"))
      {
        var module_sections = this_override["module_sections"];
        assert(module_sections.isArray());
        for (int j = 0; j < module_sections.size(); j++)
        {
          _module_settings[module_sections[j].toString().toStdString()] = this_settings;
          _module_colors[module_sections[j].toString().toStdString()] = gui_colors(this_colors);
        }
      }
    }
  }

  assert(json.hasProperty("global_settings"));
  var global_settings = json["global_settings"];
  if (global_settings.hasProperty("lighten"))
    _global_settings.lighten = (float)global_settings["lighten"];
  if (global_settings.hasProperty("mac_font_height"))
    _global_settings.mac_font_height = (float)global_settings["mac_font_height"];
  if (global_settings.hasProperty("windows_font_height"))
    _global_settings.windows_font_height = (float)global_settings["windows_font_height"];
  if (global_settings.hasProperty("linux_font_height"))
    _global_settings.linux_font_height = (float)global_settings["linux_font_height"];
  if (global_settings.hasProperty("table_cell_radius"))
    _global_settings.table_cell_radius = (int)global_settings["table_cell_radius"];
  if (global_settings.hasProperty("text_editor_radius"))
    _global_settings.text_editor_radius = (int)global_settings["text_editor_radius"];
  if (global_settings.hasProperty("scroll_thumb_radius"))
    _global_settings.scroll_thumb_radius = (int)global_settings["scroll_thumb_radius"];
  if (global_settings.hasProperty("combo_radius"))
    _global_settings.combo_radius = (int)global_settings["combo_radius"];
  if (global_settings.hasProperty("button_radius"))
    _global_settings.button_radius = (int)global_settings["button_radius"];
  if (global_settings.hasProperty("section_radius"))
    _global_settings.section_radius = (int)global_settings["section_radius"];
  if (global_settings.hasProperty("param_section_radius"))
    _global_settings.param_section_radius = (int)global_settings["param_section_radius"];
  if (global_settings.hasProperty("param_section_vpadding"))
    _global_settings.param_section_vpadding = (int)global_settings["param_section_vpadding"];
  if (global_settings.hasProperty("knob_padding"))
    _global_settings.knob_padding = (int)global_settings["knob_padding"];
  if (global_settings.hasProperty("tabular_knob_padding"))
    _global_settings.tabular_knob_padding = (int)global_settings["tabular_knob_padding"];
  if (global_settings.hasProperty("min_scale"))
    _global_settings.min_scale = (float)global_settings["min_scale"];
  if (global_settings.hasProperty("max_scale"))
    _global_settings.max_scale = (float)global_settings["max_scale"];
  if (global_settings.hasProperty("default_width_fx"))
    _global_settings.default_width_fx = (int)global_settings["default_width_fx"];
  if (global_settings.hasProperty("aspect_ratio_width_fx"))
    _global_settings.aspect_ratio_width_fx = (int)global_settings["aspect_ratio_width_fx"];
  if (global_settings.hasProperty("aspect_ratio_height_fx"))
    _global_settings.aspect_ratio_height_fx = (int)global_settings["aspect_ratio_height_fx"];
  if (global_settings.hasProperty("default_width_instrument"))
    _global_settings.default_width_instrument = (int)global_settings["default_width_instrument"];
  if (global_settings.hasProperty("aspect_ratio_width_instrument"))
    _global_settings.aspect_ratio_width_instrument = (int)global_settings["aspect_ratio_width_instrument"];
  if (global_settings.hasProperty("aspect_ratio_height_instrument"))
    _global_settings.aspect_ratio_height_instrument = (int)global_settings["aspect_ratio_height_instrument"];
}

gui_colors 
lnf::module_gui_colors(std::string const& module_full_name)
{ 
  if(_module_colors.contains(module_full_name))
    return gui_colors(_module_colors.at(module_full_name)); 
  return gui_colors(_default_colors);
}

gui_colors 
lnf::section_gui_colors(std::string const& section_full_name) 
{ 
  if (_section_colors.contains(section_full_name))
    return gui_colors(_section_colors.at(section_full_name));
  return gui_colors(_default_colors);
}

gui_colors const& 
lnf::colors() const
{
  if(_custom_section != -1)
  {
    auto full_name = _desc->plugin->gui.custom_sections[_custom_section].full_name;
    return _section_colors.contains(full_name)? _section_colors.at(full_name): _default_colors;
  }
  if(_module != -1)
  {
    auto full_name = _desc->plugin->modules[_module].info.tag.full_name;
    return _module_colors.contains(full_name) ? _module_colors.at(full_name) : _default_colors;
  }
  return _default_colors;
}

Font 
lnf::font() const
{
  // Handle the case for missing resources.
  Font result;
  if(_typeface.get())
    result = Font(_typeface);
  result.setHeight(_global_settings.get_font_height());
  result.setStyleFlags(_desc->plugin->gui.font_flags);
  return result;
}

Font 
lnf::getLabelFont(Label& label) 
{ 
  Font result = font();
  auto fit = dynamic_cast<autofit_label*>(&label);
  if (fit)
  {
    if (fit->bold()) result = result.boldened();
    if(fit->font_height() != -1) result = result.withHeight(fit->font_height());
  }
  return result;
}

module_section_gui const&
lnf::get_module_section_gui() const
{
  assert(_module_section != -1);
  return _desc->plugin->gui.module_sections[_module_section];
}

int
lnf::tab_width() const
{
  auto const& section = get_module_section_gui();
  if(section.tabbed) return -1;
  auto full_name = _desc->plugin->modules[_module].info.tag.full_name;
  return _module_settings.contains(full_name) ? _module_settings.at(full_name).tab_width : _default_settings.tab_width;
}

Path 
lnf::getTickShape(float h)
{
  Path result;
  result.addArc(0, 0, h, h, 0, 2 * pi32 + 1);
  return result;
}

void 
lnf::positionComboBoxText(ComboBox& box, Label& label)
{
  int mod_ind_width = get_combobox_mod_target_indicator_width(box, label.getFont());
  if (mod_ind_width != 0) mod_ind_width += 2;
  label.setBounds(1, 1, box.getWidth() - 10 - mod_ind_width, box.getHeight() - 2);
  label.setFont(getComboBoxFont(box));
}   

int	
lnf::getTabButtonBestWidth(TabBarButton& b, int)
{ 
  int result = tab_width();
  if (result == -1)
  {
    bool auto_size = get_module_section_gui().auto_size_tab_headers;
    float bar_width = b.getTabbedButtonBar().getWidth();
    int tab_count = b.getTabbedButtonBar().getNumTabs();
    if(!auto_size) return bar_width / tab_count;
    return font().getStringWidth(b.getButtonText()) + _global_settings.section_radius + 10;
  }
  auto full_name = _desc->plugin->modules[_module].info.tag.full_name;
  int header_width = _default_settings.header_width;
  if(_module_settings.contains(full_name)) header_width = _module_settings.at(full_name).header_width;
  if(b.getIndex() == 0) result += header_width;
  return result;
}

void 
lnf::drawTabbedButtonBarBackground(TabbedButtonBar& bar, juce::Graphics& g)
{
  g.setColour(colors().tab_header);
  g.fillRoundedRectangle(bar.getLocalBounds().toFloat(), _global_settings.section_radius);
}

void
lnf::getIdealPopupMenuItemSize(String const& text, bool separator, int standardHeight, int& w, int& h)
{
  LookAndFeel_V4::getIdealPopupMenuItemSize(text, separator, standardHeight, w, h);
  h = _global_settings.get_font_height() + 8;
}

void
lnf::drawBubble(Graphics& g, BubbleComponent& c, Point<float> const& pos, Rectangle<float> const& body)
{
  g.setColour(colors().control_background);
  g.fillRoundedRectangle(body, 2);
  g.setColour(colors().bubble_outline);
  g.drawRoundedRectangle(body, 2, 1);
}

void 
lnf::drawTooltip(Graphics& g, String const& text, int w, int h)
{
  Rectangle<int> bounds(w, h);
  g.setColour(colors().control_background);
  g.fillRect(bounds.toFloat());
  g.setColour(colors().bubble_outline);
  g.drawRect(bounds.toFloat().reduced(0.5f, 0.5f), 1.0f);
  auto layout = detail::LookAndFeelHelpers::layoutTooltipText(text, findColour(TooltipWindow::textColourId));
  layout.draw(g, Rectangle<float>(w, h));
}

void 
lnf::drawTextEditorOutline(juce::Graphics& g, int w, int h, TextEditor& te)
{
  auto cornerSize = global_settings().text_editor_radius;
  if (!te.isEnabled()) return;
  if (dynamic_cast<AlertWindow*> (te.getParentComponent()) != nullptr) return;
  if (te.hasKeyboardFocus(true) && !te.isReadOnly())
    g.setColour(te.findColour(TextEditor::focusedOutlineColourId));
  else
    g.setColour(te.findColour(TextEditor::outlineColourId));
  g.drawRoundedRectangle(0, 0, w, h, cornerSize, 2);
}

void
lnf::drawTickBox(
  Graphics& g, Component& c, float x, float y, float w, float h,
  bool ticked, bool isEnabled, bool highlighted, bool down)
{
  Rectangle<float> tickBounds(x, y, w, h);
  g.setColour(colors().control_outline);
  g.drawRoundedRectangle(tickBounds, 4.0f, 1.0f);
  if (!ticked) return;
  auto tick = getTickShape(0.67f);
  if(c.isEnabled()) g.setColour(c.findColour(ToggleButton::tickColourId));
  g.fillPath(tick, tick.getTransformToScaleToFit(tickBounds.reduced(4, 5).toFloat(), true));
}

void 
lnf::drawPopupMenuItemWithOptions(
  Graphics& g, Rectangle<int> const& area, bool highlighted, 
  PopupMenu::Item const& item, PopupMenu::Options const& options)
{
  PopupMenu::Item new_item = item;
  if(item.itemID == -1)
    new_item.isEnabled = true; // just for painting submenu headers, not actually enabled
  LookAndFeel_V4::drawPopupMenuItemWithOptions(g, area, highlighted, new_item, options);
}
 
void
lnf::drawScrollbar(Graphics& g, ScrollBar& bar, int x, int y, int w, int h,
  bool vertical, int pos, int size, bool over, bool down)
{   
  g.setColour(findColour(ScrollBar::ColourIds::backgroundColourId));
  g.fillRect(bar.getLocalBounds());

  Rectangle<int> thumbBounds;
  if (vertical) thumbBounds = { x, pos, w, size };
  else thumbBounds = { pos, y, size, h };
  auto c = bar.findColour(ScrollBar::ColourIds::thumbColourId);
  g.setColour(over ? c.brighter(0.25f) : c);
  g.fillRoundedRectangle(thumbBounds.reduced(1).toFloat(), global_settings().scroll_thumb_radius);
}

void
lnf::drawLabel(Graphics& g, Label& label)
{
  g.fillAll(label.findColour(Label::backgroundColourId));
     
  if (auto afl = dynamic_cast<autofit_label*>(&label))
    if (afl->tabular())
      draw_tabular_cell_bg(g, &label, global_settings().table_cell_radius);

  if (!label.isBeingEdited()) 
  {
    auto area = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());
    g.setFont(getLabelFont(label));

    // if we are part of a disabled combobox, paint with a grayscale control_text
    auto parent_param_combo = dynamic_cast<param_combobox*>(label.getParentComponent());
    if (parent_param_combo && !parent_param_combo->isEnabled())
      g.setColour(color_to_grayscale(colors().control_text));
    else
      g.setColour(label.findColour(Label::textColourId));

    g.drawText(label.getText(), area, label.getJustificationType(), false);
    g.setColour(label.findColour(Label::outlineColourId));
  }
  else if (label.isEnabled())
    g.setColour(label.findColour(Label::outlineColourId));
  g.drawRect(label.getLocalBounds());
}

void
lnf::drawButtonText(Graphics& g, TextButton& button, bool, bool)
{
  Font font(getTextButtonFont(button, button.getHeight()));
  g.setFont(font);
  int id = button.getToggleState() ? TextButton::textColourOnId : TextButton::textColourOffId;
  g.setColour(button.findColour(id).withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));
  const int yIndent = jmin(4, button.proportionOfHeight(0.3f));
  const int cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;
  const int fontHeight = roundToInt(font.getHeight() * 0.6f);
  const int leftIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
  const int rightIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
  const int textWidth = button.getWidth() - leftIndent - rightIndent;
  if (textWidth > 0)
    g.drawText(button.getButtonText(), leftIndent, yIndent, textWidth, button.getHeight() - yIndent * 2, Justification::centred, false);
  
  if (!dynamic_cast<menu_button*>(&button) &&
    !dynamic_cast<patch_menu*>(&button)) return;
  
  Path arrow;
  float w = 6;
  float h = 4;
  float x = button.getWidth() - 5 - w;
  float y = button.getHeight() / 2 - h / 2 + 1;
  arrow.startNewSubPath(x, y);
  arrow.lineTo(x + w, y);
  arrow.lineTo(x + w / 2, y + h);
  arrow.closeSubPath();
  g.setColour(colors().control_text);
  g.fillPath(arrow);
}
 
void 
lnf::drawButtonBackground(
  Graphics& g, Button& button, Colour const& backgroundColour, 
  bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{                    
  // this is a 1:1 copy of LookAndFeel_V4::drawButtonBackground with 2 modifications: 
  // 1: we pick the corner size from theme_settings, whereas base class hardcodes it as 6
  // 2: reduce local bounds by more than 0.5

  auto cornerSize = global_settings().button_radius;    

  auto bounds = button.getLocalBounds().toFloat().reduced(1, 1); 
  auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
    .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

  if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
    baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);
  g.setColour(baseColour);

  auto flatOnLeft = button.isConnectedOnLeft();
  auto flatOnRight = button.isConnectedOnRight();
  auto flatOnTop = button.isConnectedOnTop();
  auto flatOnBottom = button.isConnectedOnBottom();

  if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom)
  {
    Path path;
    path.addRoundedRectangle(bounds.getX(), bounds.getY(),
      bounds.getWidth(), bounds.getHeight(),
      cornerSize, cornerSize,
      !(flatOnLeft || flatOnTop),
      !(flatOnRight || flatOnTop),
      !(flatOnLeft || flatOnBottom),
      !(flatOnRight || flatOnBottom));

    g.fillPath(path);
    g.setColour(button.findColour(ComboBox::outlineColourId));
    g.strokePath(path, PathStrokeType(1.0f));
  }
  else
  {
    g.fillRoundedRectangle(bounds, cornerSize);
    g.setColour(button.findColour(ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
  }
}

void
lnf::drawComboBox(Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box)
{
  bool tabular = false;
  int box_width = width;
  auto drop_action = drop_target_action::none;
  int apply_mod_width = get_combobox_mod_target_indicator_width(box, g.getCurrentFont());

  param_combobox* param_cb = dynamic_cast<param_combobox*>(&box);
  if (param_cb != nullptr)
  {
    if (param_cb->param()->param->gui.tabular)
    {
      tabular = true;
      draw_tabular_cell_bg(g, &box, global_settings().table_cell_radius);
    }
    drop_action = param_cb->get_drop_target_action();
    if (drop_action != drop_target_action::none)
    {
      apply_mod_width = apply_mod_width;
      box_width -= apply_mod_width + 2;
    }
  }

  int arrowPad = 4;
  int arrowWidth = 6;
  int arrowHeight = 4;
  int const fixedHeight = combo_height(tabular) - (tabular? 4: 0);
  int const comboTop = height < fixedHeight ? 0 : (height - fixedHeight) / 2;
  auto cornerSize = box.findParentComponentOfClass<ChoicePropertyComponent>() != nullptr ? 0.0f : global_settings().combo_radius;
  Rectangle<int> boxBounds(tabular? 3: 1, comboTop, box_width - 2 - (tabular? 4: 0), fixedHeight);
  g.setColour(colors().control_background); 
  g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);

  Path path;
  path.startNewSubPath(box_width - arrowWidth - arrowPad, height / 2 - arrowHeight / 2 + 1);
  path.lineTo(box_width - arrowWidth / 2 - arrowPad, height / 2 + arrowHeight / 2 + 1);
  path.lineTo(box_width - arrowPad, height / 2 - arrowHeight / 2 + 1);
  path.closeSubPath();  

  Colour tick = colors().control_tick;
  if (!box.isEnabled()) tick = color_to_grayscale(tick);
  g.setColour(tick);
  g.fillPath(path);

  if (!param_cb || drop_action == drop_target_action::none) return;

  char const* drop_icon = nullptr;
  if (drop_action == drop_target_action::ok) drop_icon = "[OK]";
  else if (drop_action == drop_target_action::not_now) drop_icon = "[X]";
  else if (drop_action == drop_target_action::never) drop_icon = "[N/A]";
  else assert(false);

  auto apply_mod_box = Rectangle<int>(
    boxBounds.getTopRight().x + 2,
    boxBounds.getTopLeft().y,
    apply_mod_width,
    boxBounds.getHeight()); 
  g.setColour(Colours::darkgrey);
  g.fillRoundedRectangle(apply_mod_box.toFloat(), 2.0f);
  g.setColour(colors().control_text);
  g.setFont(font());
  g.drawText(drop_icon, apply_mod_box.toFloat(), Justification::centred);
}

void 
lnf::drawToggleButton(Graphics& g, ToggleButton& tb, bool highlighted, bool down)
{
  bool tabular = false;
  if (auto ps = dynamic_cast<param_toggle_button*>(&tb))
    if (ps->param()->param->gui.tabular)
      tabular = true;
  if(tabular)
    draw_tabular_cell_bg(g, &tb, global_settings().table_cell_radius);

  int left = tb.getWidth() / 2 - toggle_height(tabular) / 2;
  int pad = tabular? 3: 1;
  int height = tb.getHeight();
  int const fixedHeight = toggle_height(tabular);
  int const toggleTop = height < fixedHeight ? 0 : (height - fixedHeight) / 2;
  Rectangle<int> boxBounds(left + pad, toggleTop + pad, fixedHeight - pad * 2, fixedHeight - pad * 2);
  
  auto automation_color = colors().slider_automation;
  if (!tb.isEnabled()) automation_color = color_to_grayscale(automation_color);

  // background, shadow, highlight
  g.setColour(colors().slider_background);
  g.fillEllipse(boxBounds.toFloat());
  draw_conic_arc(g, boxBounds.getTopLeft().x, boxBounds.getTopLeft().y, boxBounds.getWidth(), 
    pi32, 1.5f * pi32, colors().slider_shadow, automation_color, conic_count / 2, 0.0f, 1.0f, 1.0f);
  draw_conic_arc(g, boxBounds.getTopLeft().x, boxBounds.getTopLeft().y, boxBounds.getWidth(),
    1.5f * pi32, 2.0f * pi32, automation_color, colors().slider_highlight, conic_count / 2, 0.0f, 1.0f, 1.0f);
  draw_conic_arc(g, boxBounds.getTopLeft().x, boxBounds.getTopLeft().y, boxBounds.getWidth(),
    0.0f, 0.5f * pi32, colors().slider_highlight, automation_color, conic_count / 2, 0.0f, 1.0f, 1.0f);
  draw_conic_arc(g, boxBounds.getTopLeft().x, boxBounds.getTopLeft().y, boxBounds.getWidth(),
    0.5f * pi32, pi32, automation_color, colors().slider_shadow, conic_count / 2, 0.0f, 1.0f, 1.0f);

  // toggle
  if (!tb.getToggleState()) return;
  g.setColour(automation_color);
  g.fillEllipse(boxBounds.toFloat().reduced(5.0f, 5.0f));
}

void 
lnf::drawTabButton(TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown)
{
  int radius = _global_settings.section_radius;
  int strip_left = radius + 2;
  bool is_section = _module_section != -1 && _desc->plugin->gui.module_sections[_module_section].tabbed;
  auto justify = is_section ? Justification::left : Justification::centred;
  
  float button_lighten = (button.getToggleState() || isMouseOver) ? _global_settings.lighten : 0;
  auto text_color = (is_section || button.getToggleState()) ? colors().tab_text : colors().tab_text_inactive;
  g.setColour(colors().tab_button.brighter(button_lighten));

  // no header, evenly distributed, left tab has rounded corners
  // right tab always has rounded corners
  bool left_no_header = button.getIndex() == 0 && tab_width() == -1;
  bool right_most = button.getIndex() > 0 && button.getIndex() == button.getTabbedButtonBar().getNumTabs() - 1;
  if (left_no_header || right_most)
  {
    Path path;
    auto rect = button.getActiveArea().toFloat();
    path.addRoundedRectangle(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), 
      radius, radius, left_no_header, right_most, left_no_header, right_most);
    g.fillPath(path);
  }

  // any tab not left or rightmost
  if (!left_no_header && !right_most && button.getIndex() > 0)
    g.fillRoundedRectangle(button.getActiveArea().toFloat(), 0);

  // fill text for all tab types excluding left with header
  if(left_no_header || button.getIndex() > 0)
  {
    g.setFont(font());
    auto text_area = button.getTextArea();
    if (is_section) text_area.removeFromLeft(strip_left);
    g.setColour(text_color);
    g.drawText(button.getButtonText(), text_area, justify, false);
    return;
  }

  // leftmost tab with header
  auto const& header = button.getTabbedButtonBar().getTitle();
  auto headerArea = button.getActiveArea().toFloat();
  auto buttonArea = headerArea.removeFromRight(tab_width());
  g.setColour(colors().tab_button);
  if(button.getTabbedButtonBar().getNumTabs() == 1)
    g.fillRoundedRectangle(headerArea, radius);
  else   
  {
    Path path;
    path.addRoundedRectangle(
      headerArea.getX(), headerArea.getY(), headerArea.getWidth(), 
      headerArea.getHeight(), radius, radius, true, false, true, false);
    g.fillPath(path);
  }

  auto textArea = button.getTextArea();
  textArea.removeFromLeft(radius + 2);
  g.setFont(font());
  g.setColour(button.findColour(TabbedButtonBar::tabTextColourId));
  g.drawText(header, textArea, Justification::left, false);

  // case header only
  if(button.getTabbedButtonBar().getNumTabs() == 1) return;

  // case tab header and first button
  buttonArea.removeFromLeft(1);
  g.setColour(colors().tab_button.brighter(button_lighten));
  g.fillRect(buttonArea);
  if (is_section) buttonArea.removeFromLeft(strip_left);
  g.setColour(text_color);
  g.drawText(button.getButtonText(), buttonArea, justify, false);
}

void 
lnf::drawRotarySlider(Graphics& g, int, int, int, int, float pos, float, float, Slider& s)
{
  float scale_factor = 1;
  float size_base = s.getHeight();
  auto ps = dynamic_cast<param_slider*>(&s);
  bool tabular = ps && ps->param()->param->gui.tabular;
  float padding = tabular ? _global_settings.tabular_knob_padding : _global_settings.knob_padding;

  // cell background
  if(tabular) 
  {
    size_base = 0.9 * std::min(s.getHeight(), s.getWidth());
    scale_factor = size_base / s.getHeight();
    draw_tabular_cell_bg(g, &s, global_settings().table_cell_radius);
  }
  
  float size = size_base - padding;
  float left = (s.getWidth() - size) / 2;
  float top = (s.getHeight() - size) / 2;

  if (tabular)
  {
    left += 1;
    top += 1;
    size -= 2;
  }

  bool bipolar = s.getMinimum() < 0;
  float end_angle = (180 + 340) * pi32 / 180;
  float start_angle = (180 + 20) * pi32 / 180;
  float angle_gap = end_angle - start_angle;
  float angle_range = end_angle - start_angle;

  // background, shadow, highlight
  g.setColour(colors().slider_background);
  g.fillEllipse(left + 1, top + 1, size - 2, size - 2);
  draw_conic_arc(g, left, top, size, pi32, 2.0f * pi32, 
    colors().slider_shadow, colors().slider_highlight, conic_count / 2, 0.0f, 1.0f, 1.0f);
  draw_conic_arc(g, left, top, size, 0.0f, pi32, 
    colors().slider_highlight, colors().slider_shadow, conic_count / 2, 0.0f, 1.0f, 1.0f);

  left += 3;
  top += 3;
  size -= 6;
  int stroke = 2;

  // automation indication
  auto automation_color = colors().slider_automation;
  if (!s.isEnabled()) automation_color = color_to_grayscale(automation_color);
  if(!bipolar) draw_conic_arc(g, left, top, size, start_angle, end_angle,
    automation_color, automation_color, conic_count, 0, pos, stroke);
  else if (pos >= 0.5f) draw_conic_arc(g, left, top, size, start_angle + angle_range / 2, end_angle, 
    automation_color, automation_color, conic_count / 2, 0, (pos - 0.5f) * 2, stroke);
  else draw_conic_arc(g, left, top, size, start_angle, start_angle + angle_range / 2, 
    automation_color, automation_color, conic_count / 2, pos * 2, 1, stroke);

  // automation indication
  if(!bipolar) g.setColour(automation_color.withAlpha(std::max(0.0f, 1.0f - pos * 10.0f)));
  else g.setColour(automation_color.withAlpha(1.0f - std::max(0.0f, std::abs(0.5f - pos) * 20.0f)));
  float dot_y = top + size / 2.0f + size / 2.0f * std::sin(start_angle - pi32 * 0.25f + angle_gap + pos * angle_range) - 2.0f;
  float dot_x = left + size / 2.0f + size / 2.0f * std::cos(start_angle - pi32 * 0.25f + angle_gap + pos * angle_range) - 2.0f;
  g.fillEllipse(dot_x, dot_y, 4.0f, 4.0f);

  if (!ps) return;
    
  // modulatable indicator
  if(ps->param()->param->dsp.can_modulate(ps->param()->info.slot))
  {
    g.setColour(colors().slider_can_modulate);
    g.fillEllipse(left + size / 3, top + size / 3, size / 3, size / 3);
  }

  if (ps->max_mod_indicator() < 0.0f) return;

  // modulation indication
  Path path;
  g.setColour(colors().slider_automation.darker());
  float half_mod_angle = start_angle + 0.5f * angle_range;
  float min_mod_angle = start_angle + ps->min_mod_indicator() * angle_range;
  float max_mod_angle = start_angle + ps->max_mod_indicator() * angle_range;
  if (!bipolar)
  {
    if(ps->max_mod_indicator() - ps->min_mod_indicator() <= 0.05f)
      path.addArc(left - 3, top - 3, size + 6, size + 6, start_angle, max_mod_angle, true);
    else
      path.addArc(left - 3, top - 3, size + 6, size + 6, min_mod_angle, max_mod_angle, true);
  }
  else if(ps->max_mod_indicator() - ps->min_mod_indicator() > 0.05f)
    path.addArc(left - 3, top - 3, size + 6, size + 6, min_mod_angle, max_mod_angle, true);
  else if(ps->max_mod_indicator() >= 0.5f)
    path.addArc(left - 3, top - 3, size + 6, size + 6, half_mod_angle, max_mod_angle, true);
  else
    path.addArc(left - 3, top - 3, size + 6, size + 6, max_mod_angle, half_mod_angle, true);
   g.strokePath(path, PathStrokeType(2));
}

void 	
lnf::drawLinearSlider(Graphics& g, int x, int y, int w, int h, float p, float, float, Slider::SliderStyle style, Slider& s)
{
  int padh = 2;
  float height = 8;
  float pos = (p - x) / w;
  bool bipolar = s.getMinimum() < 0;
  float left = padh / 2;
  float top = (s.getHeight() - height) / 2;
  float width = s.getWidth() - padh;
  float centerx = left + width / 2;

  auto ps = dynamic_cast<param_slider*>(&s);
  float min_mod_pos = ps ? ps->min_mod_indicator() : -1.0f;
  float max_mod_pos = ps ? ps->max_mod_indicator() : -1.0f;

  assert(style == Slider::SliderStyle::LinearHorizontal);

  // output meter
  auto const& param_topo = *ps->param()->param;
  if (ps && ps->param()->param->gui.edit_type == gui_edit_type::output_meter)
  {
    int block_pad = 2;
    int block_count = 20;
    float block_width_base = width / block_count;
    float actual_block_width = (int)(block_width_base - block_pad);
    float actual_cell_width = actual_block_width + block_pad;
    float pos_adjust = pos * (param_topo.domain.max - param_topo.domain.min);
    int block_count_off = block_count * std::clamp(pos_adjust, 0.0f, 1.0f);

    g.setColour(colors().control_background);
    for (int i = 0; i < block_count_off; i++)
      g.fillRect(left + i * actual_cell_width, top, actual_block_width, height);
    g.setColour(colors().control_background.brighter());
    for (int i = block_count_off; i < block_count; i++)
      g.fillRect(left + i * actual_cell_width, top, actual_block_width, height);
    return;
  }

  // in table mode dont align right against the next one
  // normally thats not a point because theres labels in between
  if(ps != nullptr && ps->param()->param->gui.tabular)
  {
    padh = 2;
    draw_tabular_cell_bg(g, &s, global_settings().table_cell_radius);
  }
  
  // highlight
  g.setGradientFill(ColourGradient(colors().slider_highlight.withAlpha(0.0f), left, 0, colors().slider_highlight, width, 0, false));
  g.fillRoundedRectangle(left, top, width, height, 2);

  // background
  g.setColour(colors().slider_background);
  g.fillRoundedRectangle(left + 1, top + 1, width - 2, height - 2, 2);

  // modulation indicator
  g.setColour(colors().slider_background);
  if(max_mod_pos >= 0.0f)
    if(!bipolar)
    {
      if (max_mod_pos - min_mod_pos <= 0.05f)
      {
        g.fillRect(left + 1, top, max_mod_pos * (width - 2), 1.0f);
        g.fillRect(left + 1, top + height - 1, max_mod_pos * (width - 2), 1.0f);
      } 
      else
      {
        g.fillRect(left + 1 + min_mod_pos * (width - 2), top, (max_mod_pos - min_mod_pos) * (width - 2), 1.0f);
        g.fillRect(left + 1 + min_mod_pos * (width - 2), top + height - 1, (max_mod_pos - min_mod_pos) * (width - 2), 1.0f);
      }
    } else
    {
      if (max_mod_pos >= 0.5f)
      {
        g.fillRect(centerx, top, (max_mod_pos - 0.5f) * 2 * (width - 2) / 2, 1.0f);
        g.fillRect(centerx, top + height - 1, (max_mod_pos - 0.5f) * 2 * (width - 2) / 2, 1.0f);
      }
      if(min_mod_pos < 0.5f)
      {
        float trackw = (0.5f - min_mod_pos) * 2 * (width - 2) / 2; 
        g.fillRect(centerx - trackw, top, trackw, 1.0f);
        g.fillRect(centerx - trackw, top + height - 1, trackw, 1.0f);
      }
    }

  auto automation_color = colors().slider_automation;
  if (!s.isEnabled()) automation_color = color_to_grayscale(automation_color);

  // automation indication
  g.setColour(automation_color);
  float trackw = (0.5f - pos) * 2 * (width - 4) / 2;
  if (!bipolar)
    g.fillRoundedRectangle(left + 2, top + 2, pos * (width - 4), height - 4, 2);
  else if (pos >= 0.5)
    g.fillRoundedRectangle(centerx, top + 2, (pos - 0.5f) * 2 * (width - 4) / 2, height - 4, 2);
  else
    g.fillRoundedRectangle(centerx - trackw, top + 2, trackw, height - 4, 2);

  // automation indication
  if (!bipolar)
  {
    g.setColour(automation_color.withAlpha(std::max(0.0f, 1.0f - pos * 10.0f)));
    g.fillEllipse(left + 1, top + 1, height - 2, height - 2);
  }
  else
  {
    g.setColour(automation_color.withAlpha(std::clamp(1.0f - std::max(0.0f, std::abs(0.5f - pos) * 20.0f), 0.0f, 1.0f)));
    g.fillEllipse(left + 1 + width / 2 - (height - 2) / 2, top + 1, (height - 2), (height - 2));
  }

  if(!ps) return;

  // modulatable indicator
  if (ps->param()->param->dsp.can_modulate(ps->param()->info.slot))
  {
    g.setColour(colors().slider_can_modulate.withAlpha(0.75f));
    g.fillEllipse(left + width - (height - 4) - 2, top + (height - 4) / 2, (height - 4), (height - 4));
  }
}

}