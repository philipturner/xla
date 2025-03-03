#include "torch_xla_test.h"

#include <ATen/ATen.h>

#include "absl/memory/memory.h"
#include "tensorflow/compiler/xla/xla_client/sys_util.h"
#include "tensorflow/compiler/xla/xla_client/tf_logging.h"
#include "torch_xla/csrc/device.h"
#include "torch_xla/csrc/helpers.h"
#include "torch_xla/csrc/tensor.h"

namespace at {
// This function is defined in the codegenerated RegisterDispatchKey.cpp file.
extern TORCH_API void RegisterXLAXLANativeFunctions();
extern TORCH_API void RegisterXLAAutogradXLANativeFunctions();
}  // namespace at

namespace torch_xla {
namespace cpp_test {

void XlaTest::SetUp() {
  at::RegisterXLAXLANativeFunctions();
  at::RegisterXLAAutogradXLANativeFunctions();
  at::manual_seed(42);
  XLATensor::SetRngSeed(GetCurrentDevice(), 42);
  start_msnap_ = absl::make_unique<MetricsSnapshot>();
}

void XlaTest::TearDown() {
  static bool dump_metrics =
      xla::sys_util::GetEnvBool("XLA_TEST_DUMP_METRICS", false);
  if (dump_metrics) {
    MakeEndSnapshot();

    std::string diffs = start_msnap_->DumpDifferences(*end_msnap_,
                                                      /*ignore_se=*/nullptr);
    if (!diffs.empty()) {
      TF_LOG(INFO)
          << ::testing::UnitTest::GetInstance()->current_test_info()->name()
          << " Metrics Differences:\n"
          << diffs;
    }
  }
}

void XlaTest::ExpectCounterNotChanged(
    const std::string& counter_regex,
    const std::unordered_set<std::string>* ignore_set) {
  MakeEndSnapshot();
  auto changed =
      start_msnap_->CounterChanged(counter_regex, *end_msnap_, ignore_set);
  for (auto& change_counter : changed) {
    TF_LOG(INFO) << "Counter '" << change_counter.name
                 << "' changed: " << change_counter.before << " -> "
                 << change_counter.after;
  }
  EXPECT_TRUE(changed.empty());
}

void XlaTest::ExpectCounterChanged(
    const std::string& counter_regex,
    const std::unordered_set<std::string>* ignore_set) {
  MakeEndSnapshot();
  auto changed =
      start_msnap_->CounterChanged(counter_regex, *end_msnap_, ignore_set);
  EXPECT_TRUE(!changed.empty());
}

void XlaTest::ResetCounters() {
  start_msnap_ = std::move(end_msnap_);
  end_msnap_ = nullptr;
}

void XlaTest::MakeEndSnapshot() {
  if (end_msnap_ == nullptr) {
    end_msnap_ = absl::make_unique<MetricsSnapshot>();
  }
}

void XlaTest::CommonSetup() {
  XlaHelpers::set_mat_mul_precision(xla::PrecisionConfig::HIGHEST);
}

void TorchXlaTest::SetUpTestCase() { CommonSetup(); }

void AtenXlaTensorTestBase::SetUpTestCase() { CommonSetup(); }

}  // namespace cpp_test
}  // namespace torch_xla
