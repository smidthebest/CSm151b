// Copyright 2024 Blaise Tine
//
// Licensed under the Apache License;
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <vector>

namespace tinyrv
{

  class BranchPredictor
  {
  public:
    virtual ~BranchPredictor() {}

    virtual uint32_t predict(uint32_t PC)
    {
      return PC + 4;
    };

    virtual void update(uint32_t PC, uint32_t next_PC, bool taken)
    {
      (void)PC;
      (void)next_PC;
      (void)taken;
    };
  };

  class GShare : public BranchPredictor
  {
  public:
    GShare(uint32_t BTB_size, uint32_t BHR_size);

    ~GShare() override;

    uint32_t predict(uint32_t PC) override;
    void update(uint32_t PC, uint32_t next_PC, bool taken) override;

    // TODO: Add your own methods here
    struct BTB_entry_t
    {
      bool valid;
      uint32_t tag;
      uint32_t target;
    };
  private:
    std::vector<BTB_entry_t> BTB_; 
    std::vector<uint8_t> PHT_; 
    uint8_t BHR_; 
    uint32_t BTB_shift_; 
    uint32_t BTB_mask_; 
    uint32_t BHR_mask_; 

  };

  class GSharePlus : public BranchPredictor
  {
  public:
    GSharePlus(uint32_t BTB_size, uint32_t BHR_size);

    ~GSharePlus() override;

    uint32_t predict(uint32_t PC) override;
    void update(uint32_t PC, uint32_t next_PC, bool taken) override;

    // TODO: extra credit component
  private: 
    static const int NUM_TBLS = 3; 
    static const int HISTS[NUM_TBLS]; 
    static const uint32_t TBL_SIZE = 128; 
    static const uint32_t BASE_SIZE = 1024; 

    static const int SAT_MAX = 3; 
    static const int SAT_THRESHOLD = 2; 
    static const int USEFUL_MAX = 3; 

    struct TAGE_entry {
        bool valid, 
        uint8_t tag, 
        int8_t counter, 
        uint8_t useful 
    }; 

    std::vector<std::vector<TAGE_entry>> tage_tbls_; 
    std::vector<uint8_t> base_tbl_; 

    uint32_t GHR_; 
    static const int GHR_LENGTH = 16;
    static const uint32_t GHR_mask_ = (1 << GHR_LENGTH) - 1;

    std::vector<BTB_entry_t> BTB_;
    uint32_t BTB_shift_;
    uint32_t BTB_mask_;

    int computeTAGEIndex(uint32_t PC, uint32_t GHR, int history_length) const;
    uint8_t computeTAG(uint32_t PC, uint32_t GHR, int history_length) const;

  };

}
