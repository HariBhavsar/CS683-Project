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

#ifndef OPERABLE_H
#define OPERABLE_H

#include <champsim_constants.h>
#include <iostream>

namespace champsim
{

class operable
{

public:
  class levelPredictorEntry {
    public:
    uint64_t tag;
    bool invalid;
    bool confirm;
    levelPredictorEntry() {
      invalid = true;
      confirm = false;
      lru = 0;
    }
    uint64_t lru;
  };

  class levelPredictor {
    private:
    void invalidateEntry (uint64_t addr, bool LLC);
    int getL2Set (uint64_t cl_addr);
    int getLLCSet (uint64_t cl_addr);
    int insert (uint64_t addr, bool LLC); 

    public:
    // levelPredictorEntry** table = nullptr;///
    levelPredictorEntry** l2Tracker;
    levelPredictorEntry** llcTracker;
    // std::vector<levelPredictorEntry> *extras;
    champsim::channel* l1DToLP = nullptr;
    champsim::channel* l1IToLP = nullptr;
    champsim::channel* l2ToLP = nullptr;
    champsim::channel* llcToLP = nullptr;
    champsim::channel* l1DToL2 = nullptr;
    champsim::channel* l1IToL2 = nullptr;
    champsim::channel* l2ToLLC = nullptr;
    champsim::channel* llcToDRAM = nullptr;

    champsim::operable* l1D;
    champsim::operable* l1I;
    champsim::operable* l2C;
    champsim::operable* llc;

    int l2NumSets = -1;
    int l2NumWays = -1;
    int llcNumSets = -1;
    int llcNumWays = -1;
    int l2AccCount = -1;
    int llcAccCount = -1;
    int numWays = -1;

    int wherePresent (uint64_t addr);
    void writeBack (uint64_t addr);
    void confirmAddr (uint64_t addr, bool LLC);

    levelPredictor() {
      std::cout<<"Constructor called\n";
      // table = nullptr;
      l2Tracker = nullptr;
      llcTracker = nullptr;
      l1DToLP = nullptr;
      l1IToLP = nullptr;
      l2ToLP = nullptr;
      llcToLP = nullptr;
      l1DToL2 = nullptr;
      l1IToL2 = nullptr;
      l2ToLLC = nullptr;
      llcToDRAM = nullptr;
      l2NumSets = -1;
      l2NumWays = -1;
      llcNumSets = -1;
      llcNumWays = -1;
      l2AccCount = 0;
      llcAccCount = 0;
      numWays = -1;
    };
  };

  const double CLOCK_SCALE;
  double leap_operation = 0;
static std::vector<operable::levelPredictor*> lp;  // Declaration

  static bool isConstructed;

  uint64_t current_cycle = 0;
  bool warmup = true;

  explicit operable(double scale) : CLOCK_SCALE(scale - 1) {}

  long _operate()
  {
    // skip periodically
    if (leap_operation >= 1) {
      leap_operation -= 1;
      return 0;
    }

    auto result = operate();

    leap_operation += CLOCK_SCALE;
    ++current_cycle;

    return result;
  }

  virtual void initialize() {} // LCOV_EXCL_LINE
  virtual long operate() = 0;
  virtual void begin_phase() {}       // LCOV_EXCL_LINE
  virtual void end_phase(unsigned) {} // LCOV_EXCL_LINE
  virtual void print_deadlock() {}    // LCOV_EXCL_LINE
  virtual uint64_t invalidate_entry (uint64_t inval_addr) {return 0;}
  virtual void purgeFromInflightWrites(uint64_t addr) {return;}
  virtual void purgeFromWriteQueue(uint64_t addr) {return;}
};

} // namespace champsim

#endif
