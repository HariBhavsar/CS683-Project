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
    bool isInLLC;
    bool isInBoth;
    bool invalid;
    levelPredictorEntry() {
      invalid = true;
    }
  };

  class levelPredictor {
    public:
    levelPredictorEntry** table = nullptr;
    champsim::channel* l1DToLP = nullptr;
    champsim::channel* l1IToLP = nullptr;
    champsim::channel* l2ToLP = nullptr;
    champsim::channel* llcToLP = nullptr;
    champsim::channel* l1DToL2 = nullptr;
    champsim::channel* l1IToL2 = nullptr;
    champsim::channel* l2ToLLC = nullptr;
    champsim::channel* llcToDRAM = nullptr;
    int l2NumSets = -1;
    int l2NumWays = -1;
    int llcNumSets = -1;
    int llcNumWays = -1;
    int indexingBits = -1;
    int numWays = -1;

    int getSet (uint64_t cl_addr);

    int wherePresent (uint64_t addr);

    void invalidateEntry (uint64_t addr, bool LLC);

    int insert (uint64_t addr, bool LLC); 

    levelPredictor() {
      std::cout<<"Constructor called\n";
      table = nullptr;
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
      indexingBits = -1;
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
};

} // namespace champsim

#endif
