#pragma once

#include <glog/logging.h>
#include <memory>

#include "src/carnot/plan/dag.h"
#include "src/carnot/plan/operators.h"
#include "src/carnot/plan/plan_graph.h"
#include "src/carnot/plan/proto/plan.pb.h"
#include "src/utils/status.h"

namespace pl {
namespace carnot {
namespace plan {

class PlanFragment final : public PlanGraph<planpb::PlanFragment, Operator, planpb::PlanNode> {
 public:
  explicit PlanFragment(int64_t id) : id_(id) {}
  static std::unique_ptr<PlanFragment> FromProto(const planpb::PlanFragment& pb, int64_t id);

 protected:
  int64_t id_;
};

}  // namespace plan
}  // namespace carnot
}  // namespace pl
