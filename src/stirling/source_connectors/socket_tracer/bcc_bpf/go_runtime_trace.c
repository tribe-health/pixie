/*
 * This code runs using bpf in the Linux kernel.
 * Copyright 2018- The Pixie Authors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

// LINT_C_FILE: Do not remove this line. It ensures cpplint treats this as a C file.

struct tgid_goid_t {
  uint32_t tgid;
  int64_t goid;
};

// Maps threads {tgid,pid} to the active goid being used.
// This is structured as a two layer-map:
//   The outer map, tgid_goid_map, maps tgid -> pid_goid_map.
//   The inner map, pid_goid_map, maps pid -> goid.
// The helper functions get_goid() and set_goid() below abstract away this two level access.
// The two-level access make clean-up easier when a process is terminated.
BPF_HASH(pid_goid_map, uint32_t, int64_t, /* capacity */ 1024);
BPF_HASH_OF_MAPS(tgid_goid_map, uint32_t, "pid_goid_map", /* capacity */ 1024);

static __inline int64_t* get_goid(uint32_t tgid, uint32_t pid) {
  int64_t* goid_ptr = NULL;
  void* inner_map = tgid_goid_map.lookup(&tgid);
  if (inner_map) {
    goid_ptr = bpf_map_lookup_elem(inner_map, &pid);
  }
  return goid_ptr;
}

static __inline void set_goid(uint32_t tgid, uint32_t pid, int64_t goid) {
  void* inner_map = tgid_goid_map.lookup(&tgid);
  if (inner_map) {
    bpf_map_update_elem(inner_map, &pid, &goid, /* flags */ 0);
  }
}

// This probe captures when a goroutine has its state change.
// In particular, it looks for when the goroutine switches to the 'running' state.
// Traced function signature:
//    func casgstatus(gp *g, oldval, newval uint32)
// TODO(oazizi): Investigate potential to consolidate with
//               `src/stirling/source_connectors/dynamic_tracer/dynamic_tracing/goid.cc`.
int probe_runtime_casgstatus(struct pt_regs* ctx) {
  const void* sp = (const void*)ctx->sp;
  if (sp == NULL) {
    return 0;
  }

  // This is the offset of 'goid' in golang's 'struct g'.
  // This value is set manually for now, but would eventually be set programatically via dwarf info.
  //
  //  $ llvm-dwarfdump -x -n goid https_client
  // https_client:  file format ELF64-x86-64
  //
  // 0x0003c2a3: DW_TAG_member
  //              DW_AT_name  ("goid")
  //              DW_AT_data_member_location  (152)
  //              DW_AT_type  (0x000000000003c7e6 "int64")
  //              DW_AT_unknown_2903  (0x00)
  const int kGoIDOffset = 152;

  // Get pointer to the 'struct g'.
  void* g_ptr_ptr = (void*)(sp + 8);
  void* g_ptr;
  bpf_probe_read(&g_ptr, sizeof(void*), g_ptr_ptr);
  if (g_ptr == NULL) {
    return 0;
  }

  // Get the goID.
  int64_t goid;
  bpf_probe_read(&goid, sizeof(int64_t), g_ptr + kGoIDOffset);

  int32_t newval = *(int32_t*)(sp + 20);

  const int kGRunningState = 2;
  if (newval == kGRunningState) {
    uint64_t id = bpf_get_current_pid_tgid();
    uint32_t tgid = id >> 32;
    uint32_t pid = id;

    set_goid(tgid, pid, goid);
  }

  return 0;
}
