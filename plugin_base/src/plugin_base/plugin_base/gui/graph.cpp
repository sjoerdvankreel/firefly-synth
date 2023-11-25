#include <plugin_base/gui/graph.hpp>

using namespace juce;

namespace plugin_base {

void 
graph::render(std::vector<float> const& data, bool bipolar)
{
  _data = data;
  _bipolar = bipolar;
  for (int i = 0; i < _data.size(); i++)
    _data[i] = 1 - _data[i];
  repaint();
}

void
graph::paint(Graphics& g)
{
  Path p;
  float vpad = 1;
  float w = getWidth();
  float h = getHeight();
  g.fillAll(_lnf->colors().graph_background);
  if(_data.size() == 0) return;

  float count = _data.size();
  p.startNewSubPath(0, vpad + _data[0] * (h - 2 * vpad));
  for(int i = 1; i < _data.size(); i++)
    p.lineTo(i / count * w, vpad + _data[i] * (h - 2 * vpad));
  Path pStroke(p);
  p.closeSubPath();
  
  g.setColour(_lnf->colors().graph_foreground.withAlpha(0.5f));
  g.fillPath(p);
  g.setColour(_lnf->colors().graph_foreground);
  g.strokePath(pStroke, PathStrokeType(1));
}

}