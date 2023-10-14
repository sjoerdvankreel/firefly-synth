#pragma once

#include <plugin_base/topo/param.hpp>

#include <base/source/fstring.h>
#include <public.sdk/source/vst/vsteditcontroller.h>

namespace plugin_base::vst3 {

class inf_param:
public Steinberg::Vst::Parameter
{
  param_topo const* const _topo;
public:
  Steinberg::Vst::ParamValue toNormalized(Steinberg::Vst::ParamValue plain) const override
  { return _topo->domain.raw_to_normalized(plain).value(); }
  Steinberg::Vst::ParamValue toPlain(Steinberg::Vst::ParamValue normalized) const override
  { return _topo->domain.normalized_to_raw(normalized_value(normalized)); }

  void toString(Steinberg::Vst::ParamValue normalized, Steinberg::Vst::String128 string) const override;
  bool fromString(Steinberg::Vst::TChar const* string, Steinberg::Vst::ParamValue& normalized) const override;
  inf_param(param_topo const* topo, Steinberg::Vst::ParameterInfo const& info) : Parameter(info), _topo(topo) {}
};

}