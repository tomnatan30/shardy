/* Copyright 2024 The Shardy Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include <memory>  // IWYU pragma: keep

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/IR/Value.h"
#include "mlir/IR/ValueRange.h"
#include "mlir/Pass/Pass.h"  // IWYU pragma: keep
#include "mlir/Support/LLVM.h"
#include "shardy/dialect/sdy/ir/data_flow_utils.h"
#include "shardy/dialect/sdy/ir/dialect.h"
#include "shardy/dialect/sdy/ir/utils.h"

namespace mlir {
namespace sdy {

#define GEN_PASS_DEF_ADDDATAFLOWEDGESPASS
#include "shardy/dialect/sdy/transforms/import/passes.h.inc"

namespace {

struct AddDataFlowEdgesPass
    : public impl::AddDataFlowEdgesPassBase<AddDataFlowEdgesPass> {
  using AddDataFlowEdgesPassBase::AddDataFlowEdgesPassBase;

  void runOnOperation() final {
    func::FuncOp funcOp = getOperation();

    IRRewriter rewriter(funcOp);
    funcOp.walk([&](Operation* op) {
      ValueRange edgeRoots = getDataFlowEdgeResultOwners(op);
      rewriter.setInsertionPointAfter(op);
      for (Value edgeRoot : edgeRoots) {
        if (!isStaticShapedType(edgeRoot.getType())) {
          // Skip non-static-shaped tensors, e.g., tokens.
          continue;
        }
        TensorShardingAttr sharding = nullptr;
        if (isa<OpResult>(edgeRoot)) {
          sharding = getSharding(edgeRoot);
        }
        auto dataFlowEdge = rewriter.create<DataFlowEdgeOp>(edgeRoot.getLoc(),
                                                            edgeRoot, sharding);
        rewriter.replaceAllUsesExcept(edgeRoot, dataFlowEdge, dataFlowEdge);
      }
    });
  }
};

}  // namespace

}  // namespace sdy
}  // namespace mlir
