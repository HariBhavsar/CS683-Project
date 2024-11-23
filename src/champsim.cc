/*
 *    Copyright 2023 The ChampSim Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "champsim.h"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <vector>

#include "environment.h"
#include "ooo_cpu.h"
#include "operable.h"
#include "phase_info.h"
#include "tracereader.h"
#include <fmt/chrono.h>
#include <fmt/core.h>

constexpr int DEADLOCK_CYCLE{500};

auto start_time = std::chrono::steady_clock::now();

std::chrono::seconds elapsed_time() { return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time); }

namespace champsim
{

  // operable::levelPredictor** lp = new operable::levelPredictor* [NUM_CPUS];
  bool operable::isConstructed = false;
  
  void operable::levelPredictor::confirmAddr(uint64_t addr, bool LLC) {

    uint64_t cl_addr = (addr >> LOG2_BLOCK_SIZE);
    

    // if (cl_addr == 78282) {
    //   std::cout << "confirm called for sp block with LLC = " << LLC << "\n";
    // }

    if (LLC) {

      int llcSet = getLLCSet(cl_addr);

      for (int i=0; i<llcNumWays; i++) {
        if (!(llcTracker[llcSet][i].invalid) && (llcTracker[llcSet][i].tag == cl_addr)) {
          assert (!llcTracker[llcSet][i].confirm);
          llcTracker[llcSet][i].confirm = true;
          return;
        }
      }
      // if (cl_addr == (78282)) {
      //   // std :: cout << "Printing set = " << llcSet <<"\n<tag> \t <lru> \t <invalid> \t <confirm>\n";
      //   for (int i=0; i<llcNumWays; i++) {
      //     std::cout << llcTracker[llcSet][i].tag << " \t " << llcTracker[llcSet][i].lru << " \t " << llcTracker[llcSet][i].invalid << " \t " << llcTracker[llcSet][i].confirm <<"\n";
      //   }
      // }
      // assert(false && "Damn hyperpredictor too far ahead!");

    }
    else {

      int l2Set = getL2Set(cl_addr);

      for (int i=0; i<l2NumWays; i++) {
        if (!(l2Tracker[l2Set][i].invalid) && (l2Tracker[l2Set][i].tag == cl_addr)) {
          assert (!l2Tracker[l2Set][i].confirm);
          l2Tracker[l2Set][i].confirm = true;
          return;
        }
      }

      // assert(false && "Damn hyperpredictor too far ahead of L2!");

    }

  }

  void operable::levelPredictor::invalidateEntry (uint64_t addr, bool LLC) {

    uint64_t cl_addr = (addr >> LOG2_BLOCK_SIZE);


    // if (cl_addr == 78282) {
    //   std::cout << "invalidate called for sp block with LLC = " << LLC << "\n";
    // }

    if (LLC) {
      int llcSet = getLLCSet(cl_addr);
      for (int i=0; i<llcNumWays; i++) {
        if (!(llcTracker[llcSet][i].invalid) && (llcTracker[llcSet][i].tag == cl_addr)) {
          // if (!llcTracker[llcSet][i].confirm) {
            // std :: cout << addr << "\n";
          // }
          // assert(llcTracker[llcSet][i].confirm && "Must be for sure in LLC or else predictor too fast");
          llcTracker[llcSet][i].invalid = true;
          llcTracker[llcSet][i].confirm = false;
          llcAccCount++;
          return;
        }
      }
      assert(false && "Invalidating some llc address that wasn't even found in LLC!");
    }
    else {
      int l2Set = getL2Set(cl_addr);
      for (int i=0; i<l2NumWays; i++) {
        if (!(l2Tracker[l2Set][i].invalid) && (l2Tracker[l2Set][i].tag == cl_addr)) {
          l2Tracker[l2Set][i].invalid = true;
          // assert(l2Tracker[l2Set][i].confirm && "Must be for sure in L2 or else predictor too fast");
          l2Tracker[l2Set][i].confirm = false;
          l2AccCount++;
          return;
        }
      }
      assert(false && "Invalidating some l2 address that wasn't even found in L2!");
    }
  }

  int operable::levelPredictor::insert(uint64_t addr, bool LLC) {

    uint64_t cl_addr = (addr >> LOG2_BLOCK_SIZE);

    // if (cl_addr == 78282) {
    //   std::cout << "insert called for sp block with LLC = " << LLC << "\n";
    // }

    if (LLC) {
      int llcSet = getLLCSet(cl_addr);
      int freeWay = -1;
      int lruEntry = 0;
      for (size_t i=0; i < llcNumWays; i++) {
        if (!(llcTracker[llcSet][i].invalid) && (llcTracker[llcSet][i].tag == cl_addr)) {
          // std::cout << addr << std::endl;
          assert(false && "Inserting address into llc but address already in llc");
        }
        else if (llcTracker[llcSet][i].invalid) {
          freeWay = i;
        }
        else if (!(llcTracker[llcSet][i].invalid) && (llcTracker[llcSet][i].lru < llcTracker[llcSet][lruEntry].lru)) {
          lruEntry = i;
        }
      }
      if (freeWay != -1) {
        llcTracker[llcSet][freeWay].invalid = false;
        llcTracker[llcSet][freeWay].tag = cl_addr;
        llcTracker[llcSet][freeWay].confirm = false;
        llcTracker[llcSet][freeWay].lru = llcAccCount;
        llcAccCount++;
      }
      else {
        // simulate eviction, need to evict lru entry
        // if (!(!(llcTracker[llcSet][lruEntry].invalid) && llcTracker[llcSet][lruEntry].confirm)) {
        //   // std :: cout << llcTracker[llcSet][lruEntry].tag << "\n";

        //   // std :: cout << "Printing set = " << llcSet <<"\n<tag> \t <lru> \t <invalid> \t <confirm>\n";
        //   // for (int i=0; i<llcNumWays; i++) {
        //   //   std::cout << llcTracker[llcSet][i].tag << " \t " << llcTracker[llcSet][i].lru << " \t " << llcTracker[llcSet][i].invalid << " \t " << llcTracker[llcSet][i].confirm <<"\n";
        //   // }

        // }
        // assert(!(llcTracker[llcSet][lruEntry].invalid) && llcTracker[llcSet][lruEntry].confirm && "LLC LRU entry must be confirm present in LLC!");
        // if ((llcTracker[llcSet][lruEntry].tag) == (3699576 >> LOG2_BLOCK_SIZE)) {
        //   std::cout << "Predicting eviction of sp. address from LLC\n";
        // }
        llcTracker[llcSet][lruEntry].invalid = false;
        llcTracker[llcSet][lruEntry].tag = cl_addr;
        llcTracker[llcSet][lruEntry].lru = llcAccCount;
        llcTracker[llcSet][lruEntry].confirm = false;
        llcAccCount++;
      }
    }
    else {
      int l2Set = getL2Set(cl_addr);
      int freeWay = -1;
      int lruEntry = 0;
      for (size_t i=0; i<l2NumWays; i++) {
        if (!(l2Tracker[l2Set][i].invalid) && (l2Tracker[l2Set][i].tag == cl_addr)) {
          // std::cout << "addr = " << addr << std::endl;
          assert(false && "Inserting address into l2 but address already in l2");
        }
        else if (l2Tracker[l2Set][i].invalid) {
          freeWay = i;
        }
        else if (!(l2Tracker[l2Set][i].invalid) && (l2Tracker[l2Set][i].lru < l2Tracker[l2Set][lruEntry].lru)) {
          lruEntry = i;
        }
      }
      if (freeWay != -1) {
        l2Tracker[l2Set][freeWay].invalid = false;
        l2Tracker[l2Set][freeWay].tag = cl_addr;
        l2Tracker[l2Set][freeWay].lru = l2AccCount;
        l2Tracker[l2Set][freeWay].confirm = false;
        l2AccCount++;
      }
      else {
        // simulate eviction, need to evict lru entry => put it in LLC
        // assert(!(l2Tracker[l2Set][lruEntry].invalid) && l2Tracker[l2Set][lruEntry].confirm && "L2 LRU entry must be confirmed and valid!");
        // if ((l2Tracker[l2Set][lruEntry].tag) == (3699576 >> LOG2_BLOCK_SIZE)) {
          
        //   std::cout << "Set = " << l2Set << "\n";
        //   std::cout<<"Printing <tag> \t <lru> \t <invalid>\n";
        //   for (int i=0; i < l2NumWays; i++) {
        //     std :: cout << l2Tracker[l2Set][i].tag << " \t " << l2Tracker[l2Set][i].lru << " \t " << l2Tracker[l2Set][i].invalid<<"\n";
        //   }

        //   std::cout << "Predicting eviction of sp. address from L2, triggering address is " << addr << "\n";
        // }
        insert((l2Tracker[l2Set][lruEntry].tag << LOG2_BLOCK_SIZE),true);
        l2Tracker[l2Set][lruEntry].invalid = false;
        l2Tracker[l2Set][lruEntry].confirm = false;
        l2Tracker[l2Set][lruEntry].tag = cl_addr;
        l2Tracker[l2Set][lruEntry].lru = l2AccCount;
        l2AccCount++;
      }
    }


  }

  int operable::levelPredictor::wherePresent(uint64_t addr) {
    // returns 0 if addr in DRAM, 1 if in L2 and 2 if in LLC
    uint64_t cl_addr = (addr >> LOG2_BLOCK_SIZE);

    // if (cl_addr == 78282) {
    //   std::cout << "where present called for sp block\n";
    // }

    int l2Set = getL2Set(cl_addr);
    for (size_t i=0; i < l2NumWays; i++) {
      if (!(l2Tracker[l2Set][i].invalid) && (l2Tracker[l2Set][i].tag == cl_addr)) {
        invalidateEntry(addr,false);
        return 1;
      }
    }

    int llcSet = getLLCSet(cl_addr);
    for (size_t i=0; i < llcNumWays; i++) {
      if (!(llcTracker[llcSet][i].invalid) && (llcTracker[llcSet][i].tag == cl_addr) && (llcTracker[llcSet][i].confirm)) {
        invalidateEntry(addr,true);
        return 2;
      }
      else if (!(llcTracker[llcSet][i].invalid) && (llcTracker[llcSet][i].tag == cl_addr)) {
        // we should predict L2 here!
        invalidateEntry(addr,true); // not 100% sure about this, might cause bt
        // std :: cout << "Predicting bt for " << addr << "\n";
        return 1;
      }
    }
    
    return 0;

  }

  void operable::levelPredictor::writeBack(uint64_t addr) {
    // returns 0 if addr in DRAM, 1 if in L2 and 2 if in LLC
    insert(addr,false);

  }

  int operable::levelPredictor::getL2Set(uint64_t cl_addr) {
      // takes cache line address and returns set in LP it maps to 
      uint64_t tmp = cl_addr & champsim::bitmask(champsim::lg2(l2NumSets));
      return tmp;

  }

  int operable::levelPredictor::getLLCSet(uint64_t cl_addr) {
      // takes cache line address and returns set in LP it maps to 
      uint64_t tmp = cl_addr & champsim::bitmask(champsim::lg2(llcNumSets));
      return tmp;

  }  

phase_stats do_phase(phase_info phase, environment& env, std::vector<tracereader>& traces)
{
  auto [phase_name, is_warmup, length, trace_index, trace_names] = phase;
  auto operables = env.operable_view();

  // Initialize phase
  for (champsim::operable& op : operables) {
    op.warmup = is_warmup;
    op.begin_phase();
  }

  // Perform phase
  int stalled_cycle{0};
  std::vector<bool> phase_complete(std::size(env.cpu_view()), false);
  while (!std::accumulate(std::begin(phase_complete), std::end(phase_complete), true, std::logical_and{})) {
    auto next_phase_complete = phase_complete;

    // Operate
    long progress{0};
    for (champsim::operable& op : operables) {
      progress += op._operate();
    }

    if (progress == 0) {
      ++stalled_cycle;
    } else {
      stalled_cycle = 0;
    }

    if (stalled_cycle >= DEADLOCK_CYCLE) {
      std::for_each(std::begin(operables), std::end(operables), [](champsim::operable& c) { c.print_deadlock(); });
      abort();
    }

    std::sort(std::begin(operables), std::end(operables),
              [](const champsim::operable& lhs, const champsim::operable& rhs) { return lhs.leap_operation < rhs.leap_operation; });

    // Read from trace
    for (O3_CPU& cpu : env.cpu_view()) {
      auto& trace = traces.at(trace_index.at(cpu.cpu));
      for (auto pkt_count = cpu.IN_QUEUE_SIZE - static_cast<long>(std::size(cpu.input_queue)); !trace.eof() && pkt_count > 0; --pkt_count)
        cpu.input_queue.push_back(trace());

      // If any trace reaches EOF, terminate all phases
      if (trace.eof())
        std::fill(std::begin(next_phase_complete), std::end(next_phase_complete), true);
    }

    // Check for phase finish
    for (O3_CPU& cpu : env.cpu_view()) {
      // Phase complete
      next_phase_complete[cpu.cpu] = next_phase_complete[cpu.cpu] || (cpu.sim_instr() >= length);
    }

    for (O3_CPU& cpu : env.cpu_view()) {
      if (next_phase_complete[cpu.cpu] != phase_complete[cpu.cpu]) {
        for (champsim::operable& op : operables)
          op.end_phase(cpu.cpu);

        fmt::print("{} finished CPU {} instructions: {} cycles: {} cumulative IPC: {:.4g} (Simulation time: {:%H hr %M min %S sec})\n", phase_name, cpu.cpu,
                   cpu.sim_instr(), cpu.sim_cycle(), std::ceil(cpu.sim_instr()) / std::ceil(cpu.sim_cycle()), elapsed_time());
      }
    }

    phase_complete = next_phase_complete;
  }

  for (O3_CPU& cpu : env.cpu_view()) {
    fmt::print("{} complete CPU {} instructions: {} cycles: {} cumulative IPC: {:.4g} (Simulation time: {:%H hr %M min %S sec})\n", phase_name, cpu.cpu,
               cpu.sim_instr(), cpu.sim_cycle(), std::ceil(cpu.sim_instr()) / std::ceil(cpu.sim_cycle()), elapsed_time());
  }

  phase_stats stats;
  stats.name = phase.name;

  for (std::size_t i = 0; i < std::size(trace_index); ++i)
    stats.trace_names.push_back(trace_names.at(trace_index.at(i)));

  auto cpus = env.cpu_view();
  std::transform(std::begin(cpus), std::end(cpus), std::back_inserter(stats.sim_cpu_stats), [](const O3_CPU& cpu) { return cpu.sim_stats; });
  std::transform(std::begin(cpus), std::end(cpus), std::back_inserter(stats.roi_cpu_stats), [](const O3_CPU& cpu) { return cpu.roi_stats; });

  auto caches = env.cache_view();
  std::transform(std::begin(caches), std::end(caches), std::back_inserter(stats.sim_cache_stats), [](const CACHE& cache) { return cache.sim_stats; });
  std::transform(std::begin(caches), std::end(caches), std::back_inserter(stats.roi_cache_stats), [](const CACHE& cache) { return cache.roi_stats; });

  auto dram = env.dram_view();
  std::transform(std::begin(dram.channels), std::end(dram.channels), std::back_inserter(stats.sim_dram_stats),
                 [](const DRAM_CHANNEL& chan) { return chan.sim_stats; });
  std::transform(std::begin(dram.channels), std::end(dram.channels), std::back_inserter(stats.roi_dram_stats),
                 [](const DRAM_CHANNEL& chan) { return chan.roi_stats; });

  return stats;
}

// simulation entry point
std::vector<phase_stats> main(environment& env, std::vector<phase_info>& phases, std::vector<tracereader>& traces)
{
  for (champsim::operable& op : env.operable_view())
    op.initialize();

  std::vector<phase_stats> results;
  for (auto phase : phases) {
    auto stats = do_phase(phase, env, traces);
    if (!phase.is_warmup)
      results.push_back(stats);
  }

  return results;
}
} // namespace champsim
