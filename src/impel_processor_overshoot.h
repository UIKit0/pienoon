// Copyright 2014 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef IMPEL_PROCESSOR_OVERSHOOT_H_
#define IMPEL_PROCESSOR_OVERSHOOT_H_

#include "impel_processor_base_classes.h"

namespace impel {


struct OvershootImpelInit : public ImpelInitWithVelocity {
  IMPEL_INIT_REGISTER();

  OvershootImpelInit() :
      ImpelInitWithVelocity(kType),
      accel_per_difference(0.0f),
      wrong_direction_multiplier(0.0f),
      max_delta_time(0)
  {}

  // Acceleration is a multiple of abs('state_.position' - 'target_.position').
  // Bigger differences cause faster acceleration.
  float accel_per_difference;

  // When accelerating away from the target, we multiply our acceleration by
  // this amount. We need counter-acceleration to be stronger so that the
  // amplitude eventually dies down; otherwise, we'd just have a pendulum.
  float wrong_direction_multiplier;

  // The algorithm is iterative. When the iteration step gets too big, the
  // behavior becomes erratic. This value clamps the iteration step.
  ImpelTime max_delta_time;
};

struct OvershootImpelData : public ImpelDataWithVelocity {
  OvershootImpelInit init;

  virtual void Initialize(const ImpelInit& init_param) {
    ImpelDataWithVelocity::Initialize(init_param);
    init = static_cast<const OvershootImpelInit&>(init_param);
  }
};


class OvershootImpelProcessor :
    public ImpelProcessorWithVelocity<OvershootImpelData, OvershootImpelInit> {

 public:
  IMPEL_PROCESSOR_REGISTER(OvershootImpelProcessor, OvershootImpelInit);
  virtual ~OvershootImpelProcessor() {}
  virtual void AdvanceFrame(ImpelTime delta_time);

 protected:
  float CalculateVelocity(ImpelTime delta_time, const OvershootImpelData& d) const;
  float CalculateValue(ImpelTime delta_time, const OvershootImpelData& d) const;
};

} // namespace impel

#endif // IMPEL_PROCESSOR_OVERSHOOT_H_

