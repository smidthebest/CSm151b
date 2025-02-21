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
    : BTB_(BTB_size, BTB_entry_t{false, 0x0, 0x0}), PHT_((1 << BHR_size), 0x0), BHR_(0x0), BTB_shift_(log2ceil(BTB_size)), BTB_mask_(BTB_size - 1), BHR_mask_((1 << BHR_size) - 1)
//--
{
}

GShare::~GShare()
{
  //--
}

uint32_t GShare::predict(uint32_t PC)
{
  uint32_t next_PC = PC + 4;
  bool predict_taken = false;

  // TODO:
  predict_taken = (PHT_[(((PC >> 2) ^ BHR_) & BHR_mask_)] >= 2);

  if (predict_taken)
  {
    uint32_t btb_idx = (PC >> 2) & BTB_mask_;
    if (BTB_[btb_idx].valid && (PC >> 2) == BTB_[btb_idx].tag)
    {
      next_PC = BTB_[btb_idx].target;
    }
  }

  DT(3, "*** GShare: predict PC=0x" << std::hex << PC << std::dec
                                    << ", next_PC=0x" << std::hex << next_PC << std::dec
                                    << ", predict_taken=" << predict_taken);
  return next_PC;
}

void GShare::update(uint32_t PC, uint32_t next_PC, bool taken)
{
  DT(3, "*** GShare: update PC=0x" << std::hex << PC << std::dec
                                   << ", next_PC=0x" << std::hex << next_PC << std::dec
                                   << ", taken=" << taken);

  uint32_t PHT_idx = (((PC >> 2) ^ BHR_) & BHR_mask_);
  if (taken)
  {
    if (PHT_[PHT_idx] < 3)
      PHT_[PHT_idx]++;
  }
  else
  {
    if (PHT_[PHT_idx] > 0)
      PHT_[PHT_idx]--;
  }

  uint32_t btb_idx = (PC >> 2) & BTB_mask_;
  if (taken)
  {
    BTB_[btb_idx].valid = true;
    BTB_[btb_idx].target = next_PC;
    BTB_[btb_idx].tag = (PC >> 2);
  }

  BHR_ = (BHR_ << 1 | taken) & BHR_mask_;
}

///////////////////////////////////////////////////////////////////////////////

GSharePlus::GSharePlus(uint32_t BTB_size, uint32_t BHR_size)
    : GHR_(0)
{
  BTB_shift_ = log2ceil(BTB_size);
  BTB_mask_ = BTB_size - 1;
  BTB_.resize(BTB_size, BTB_entry_t{false, 0x0, 0x0});

  (void)BHR_size; 

  base_tbl_.resize(BASE_SIZE, 0);

  tage_tbls_.resize(NUM_TBLS);
  for (int i = 0; i < NUM_TBLS; i++)
  {
    tage_tbls_[i].resize(NUM_TBLS);
  }
}

GSharePlus::~GSharePlus()
{
  //--
}

int GSharePlus::compute_idx(uint32_t PC, uint32_t GHR, int history_length) const
{
  uint32_t h = GHR & ((1 << history_length) - 1);
  uint32_t index = (PC ^ h ^ (history_length * 0x9e3779b9)) % TBL_SIZE;
  return static_cast<int>(index);
}

uint8_t GSharePlus::compute_tag(uint32_t PC, uint32_t GHR, int history_length) const
{
  uint8_t tag = static_cast<uint8_t>((PC ^ (GHR >> history_length)) & 0xFF);
  return tag;
}

uint32_t GSharePlus::predict(uint32_t PC)
{
  uint32_t next_PC = PC + 4;
  bool predict_taken = false;
  int predictor = -1;

  for (int i = 0; i < NUM_TBLS; i++)
  {
    int idx = compute_idx(PC, GHR_, 16 - i * 4);
    const auto &entry = tage_tbls_[i][idx];
    if (entry.valid && entry.tag == compute_tag(PC, GHR_, 16 - i * 4))
    {
      predictor = i;
      predict_taken = (entry.counter >= SAT_THRESHOLD);
      break;
    }
  }

  if (predictor == -1)
  {
    predict_taken = base_tbl_[PC % BASE_SIZE] >= SAT_THRESHOLD;
  }

  if (predict_taken)
  {
    uint32_t btb_idx = (PC >> 2) & BTB_mask_;
    if (BTB_[btb_idx].valid && (PC >> 2) == BTB_[btb_idx].tag)
    {
      next_PC = BTB_[btb_idx].target;
    }
  }

  DT(3, "*** GShare+: predict PC=0x" << std::hex << PC << std::dec
                                     << ", next_PC=0x" << std::hex << next_PC << std::dec
                                     << ", predict_taken=" << predict_taken);
  return next_PC;
}

void GSharePlus::update(uint32_t PC, uint32_t next_PC, bool taken)
{
  int provider_table = -1;
  int provider_index = -1;
  for (int i = 0; i < NUM_TBLS; i++)
  {
    int index = compute_idx(PC, GHR_, 16 - i * 4);
    auto &entry = tage_tbls_[i][index];
    if (entry.valid && (entry.tag == compute_tag(PC, GHR_, 16 - i * 4)))
    {
      provider_table = i;
      provider_index = index;
      break;
    }
  }

  int base_index = PC % BASE_SIZE;
  if (taken)
  {
    if (base_tbl_[base_index] < SAT_MAX)
      base_tbl_[base_index]++;
  }
  else
  {
    if (base_tbl_[base_index] > 0)
      base_tbl_[base_index]--;
  }

  if (provider_table != -1)
  {
    auto &entry = tage_tbls_[provider_table][provider_index];
    bool old_prediction = (entry.counter >= SAT_THRESHOLD);
    if (taken)
    {
      if (entry.counter < SAT_MAX)
        entry.counter++;
    }
    else
    {
      if (entry.counter > 0)
        entry.counter--;
    }

    bool new_prediction = (entry.counter >= SAT_THRESHOLD);
    if (old_prediction == new_prediction)
    {
      if (entry.useful < USEFUL_MAX)
        entry.useful++;
    }
    else
    {
      if (entry.useful > 0)
        entry.useful--;
    }
  }
  else
  {
    for (int i = 0; i < NUM_TBLS; i++)
    {
      int index = compute_idx(PC, GHR_, 16 - i * 4);
      auto &entry = tage_tbls_[i][index];
      if (!entry.valid || entry.useful == 0)
      {
        entry.valid = true;
        entry.tag = compute_tag(PC, GHR_, 16 - i * 4);
        entry.counter = taken ? (SAT_THRESHOLD + 0) : (SAT_THRESHOLD - 1);
        entry.useful = 0;
        break;
      }
    }
  }

  int btb_idx = (PC >> 2) & BTB_mask_;
  if (taken)
  {
    BTB_[btb_idx].valid = true;
    BTB_[btb_idx].target = next_PC;
    BTB_[btb_idx].tag = (PC >> 2);
  }

  GHR_ = ((GHR_ << 1) | taken) & GHR_mask_;

  DT(3, "*** GShare+: update PC=0x" << std::hex << PC << std::dec
                                    << ", next_PC=0x" << std::hex << next_PC << std::dec
                                    << ", taken=" << taken);

  // TODO: extra credit component
}
