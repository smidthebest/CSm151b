// Copyright 2024 blaise
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

#include <iostream>
#include <assert.h>
#include <util.h>
#include "types.h"
#include "core.h"
#include "debug.h"

using namespace tinyrv;

///////////////////////////////////////////////////////////////////////////////

GShare::GShare(uint32_t BTB_size, uint32_t BHR_size)
   : BTB_(BTB_size, BTB_entry_t{false, 0x0, 0x0})
   , PHT_((1 << BHR_size), 0x0)
    , BHR_(0x0)
   , BTB_shift_(log2ceil(BTB_size))
   , BTB_mask_(BTB_size-1)
   , BHR_mask_((1 << BHR_size)-1) 
  //--
{}

GShare::~GShare() {
  //--
}

uint32_t GShare::predict(uint32_t PC) {
  uint32_t next_PC = PC + 4;
  bool predict_taken = false;

  // TODO:
  predict_taken = (PHT_[(((PC >> BTB_shift_) ^ BHR_) & BHR_mask_)] >= 2); 

  if(predict_taken){
    uint32_t btb_idx = (PC >> 2) & BTB_mask_; 
    if(BTB_[btb_idx].valid && PC == BTB_[btb_idx].tag){
      next_PC = BTB_[btb_idx].target; 
    }
  }

  DT(3, "*** GShare: predict PC=0x" << std::hex << PC << std::dec
        << ", next_PC=0x" << std::hex << next_PC << std::dec
        << ", predict_taken=" << predict_taken);
  return next_PC;
}

void GShare::update(uint32_t PC, uint32_t next_PC, bool taken) {
  DT(3, "*** GShare: update PC=0x" << std::hex << PC << std::dec
        << ", next_PC=0x" << std::hex << next_PC << std::dec
        << ", taken=" << taken);

  uint32_t PHT_idx = (((PC >> BTB_shift_) ^ BHR_) & BHR_mask_); 
  if(taken){
    if(PHT_[PHT_idx] < 3) PHT_[PHT_idx]++; 
  }
  else {
    if(PHT_[PHT_idx] > 0) PHT_[PHT_idx]--; 
  }

  uint32_t btb_idx = (PC >> 2) & BTB_mask_; 
  if(taken){
    BTB_[btb_idx].valid = true; 
    BTB_[btb_idx].target = next_PC; 
    BTB_[btb_idx].tag = PC; 
  }

  BHR_ = (BHR_ << 1 | taken) & BHR_mask_; 
}

///////////////////////////////////////////////////////////////////////////////

GSharePlus::GSharePlus(uint32_t BTB_size, uint32_t BHR_size) {
  (void) BTB_size;
  (void) BHR_size;
}

GSharePlus::~GSharePlus() {
  //--
}

uint32_t GSharePlus::predict(uint32_t PC) {
  uint32_t next_PC = PC + 4;
  bool predict_taken = false;
  (void) PC;
  (void) next_PC;
  (void) predict_taken;

  // TODO: extra credit component

  DT(3, "*** GShare+: predict PC=0x" << std::hex << PC << std::dec
        << ", next_PC=0x" << std::hex << next_PC << std::dec
        << ", predict_taken=" << predict_taken);
  return next_PC;
}

void GSharePlus::update(uint32_t PC, uint32_t next_PC, bool taken) {
  (void) PC;
  (void) next_PC;
  (void) taken;

  DT(3, "*** GShare+: update PC=0x" << std::hex << PC << std::dec
        << ", next_PC=0x" << std::hex << next_PC << std::dec
        << ", taken=" << taken);

  // TODO: extra credit component
}


