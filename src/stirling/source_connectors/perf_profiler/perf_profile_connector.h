/*
 * Copyright 2018- The Pixie Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "src/shared/types/types.h"
#include "src/stirling/bpf_tools/bcc_bpf_intf/upid.h"
#include "src/stirling/bpf_tools/bcc_wrapper.h"
#include "src/stirling/core/source_connector.h"
#include "src/stirling/core/types.h"
#include "src/stirling/source_connectors/perf_profiler/bcc_bpf_intf/stack_event.h"
#include "src/stirling/source_connectors/perf_profiler/stack_trace_id_cache.h"
#include "src/stirling/source_connectors/perf_profiler/stack_traces_table.h"
#include "src/stirling/source_connectors/perf_profiler/stringifier.h"
#include "src/stirling/source_connectors/perf_profiler/symbolizer.h"
#include "src/stirling/source_connectors/perf_profiler/types.h"
#include "src/stirling/utils/stat_counter.h"

namespace px {
namespace stirling {

namespace profiler {
static constexpr std::string_view kNotSymbolizedMessage = "<not symbolized>";
}  // namespace profiler

class PerfProfileConnector : public SourceConnector, public bpf_tools::BCCWrapper {
 public:
  static constexpr std::string_view kName = "perf_profiler";
  static constexpr auto kTables = MakeArray(kStackTraceTable);
  static constexpr uint32_t kPerfProfileTableNum = TableNum(kTables, kStackTraceTable);

  static std::unique_ptr<PerfProfileConnector> Create(std::string_view name) {
    return std::unique_ptr<PerfProfileConnector>(new PerfProfileConnector(name));
  }

  Status InitImpl() override;
  Status StopImpl() override;
  void TransferDataImpl(ConnectorContext* ctx, const std::vector<DataTable*>& data_tables) override;

  std::chrono::milliseconds SamplingPeriod() const { return sampling_period_; }
  std::chrono::milliseconds StackTraceSamplingPeriod() const {
    return stack_trace_sampling_period_;
  }

 private:
  // The time interval between stack trace samples, i.e. the sample rate used inside of BPF.
  const std::chrono::milliseconds stack_trace_sampling_period_;

  // Push period is set to 1/2 of the sample period such that we push each new
  // sample when it becomes available. This is a UX decision so that the user
  // gets fresh profiler data every 30 seconds (or worst case w/in 45 seconds).
  const std::chrono::milliseconds sampling_period_;
  const std::chrono::milliseconds push_period_;

  // StackTraceHisto: SymbolicStackTrace => observation-count
  using StackTraceHisto = absl::flat_hash_map<SymbolicStackTrace, uint64_t>;

  // RawHistoData: a list of stack trace keys that will need to be histogrammed.
  using RawHistoData = std::vector<stack_trace_key_t>;

  explicit PerfProfileConnector(std::string_view source_name);

  void ProcessBPFStackTraces(ConnectorContext* ctx, DataTable* data_table);

  // Read BPF data structures, build & incorporate records to the table.
  void CreateRecords(ebpf::BPFStackTable* stack_traces, ConnectorContext* ctx,
                     DataTable* data_table);

  StackTraceHisto AggregateStackTraces(ConnectorContext* ctx, ebpf::BPFStackTable* stack_traces);

  void CleanupSymbolizers(const absl::flat_hash_set<md::UPID>& deleted_upids);

  // data structures shared with BPF:
  std::unique_ptr<ebpf::BPFStackTable> stack_traces_a_;
  std::unique_ptr<ebpf::BPFStackTable> stack_traces_b_;

  std::unique_ptr<ebpf::BPFArrayTable<uint64_t>> profiler_state_;

  // Number of iterations, where each iteration is drains the information collectid in BPF.
  uint64_t transfer_count_ = 0;

  // Tracks unique stack trace ids, for the lifetime of Stirling:
  StackTraceIDCache stack_trace_ids_;

  // The raw histogram from BPF; it is populated on each iteration by a call to PollPerfBuffer().
  RawHistoData raw_histo_data_;

  // For converting stack trace addresses to symbols.
  std::unique_ptr<Symbolizer> k_symbolizer_;
  std::unique_ptr<Symbolizer> u_symbolizer_;

  // Keeps track of processes. Used to find destroyed processes on which to perform clean-up.
  // TODO(oazizi): Investigate ways of sharing across source_connectors.
  ProcTracker proc_tracker_;

  static void HandleHistoEvent(void* cb_cookie, void* data, int /*data_size*/);
  static void HandleHistoLoss(void* cb_cookie, uint64_t lost);

  // Called by HandleHistoEvent() to add the stack-trace-key to raw_histo_data_.
  void AcceptStackTraceKey(stack_trace_key_t* data);

  ebpf::BPFPerfBuffer* histogram_a_perf_buffer_;
  ebpf::BPFPerfBuffer* histogram_b_perf_buffer_;

  enum class StatKey {
    kBPFMapSwitchoverEvent,
    kCumulativeSumOfAllStackTraces,
    kLossHistoEvent,
  };

  const uint32_t stats_log_interval_;
  utils::StatCounter<StatKey> stats_;
};

}  // namespace stirling
}  // namespace px
