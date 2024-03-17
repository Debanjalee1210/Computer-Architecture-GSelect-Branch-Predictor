#ifndef __CPU_PRED_G_SELECT_PRED_HH__
#define __CPU_PRED_G_SELECT_PRED_HH__

#include <vector>

#include "base/sat_counter.hh"
#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "params/GSelectBP.hh"


/**
 * Implements a GSelect predictor that uses the hash of PC and 
 * global history register to index into a table of counters.
 */
class GSelectBP : public BPredUnit
{
  public:
    /**
     * GSelect branch predictor constructor.
     */
    GSelectBP(const GSelectBPParams &params);

    virtual void uncondBranch(ThreadID tid, Addr pc, void * &bp_history);

    /**
     * Looks up the given address in the branch predictor and returns
     * a true/false value as to whether it is taken.
     * @param branch_addr The address of the branch to look up.
     * @param bp_history Pointer to any bp history state.
     * @return Whether or not the branch is taken.
     */
    bool lookup(ThreadID tid, Addr branch_addr, void * &bp_history);

    /**
     * Updates the branch predictor to Not Taken if a BTB entry is
     * invalid or not found.
     * @param branch_addr The address of the branch to look up.
     * @param bp_history Pointer to any bp history state.
     * @return Whether or not the branch is taken.
     */
    void btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history);

    /**
     * Updates the branch predictor with the actual result of a branch.
     * @param branch_addr The address of the branch to update.
     * @param taken Whether or not the branch was taken.
     */
    void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed, const StaticInstPtr & inst, Addr corrTarget);

    /* Function to revert the global history register which was 
    speculatively updated with branch prediction*/
    void squash(ThreadID tid, void *bp_history);

  private:

      struct BPHistory {
        unsigned globalHistoryReg;
        // the final taken/not-taken prediction
        // true: predict taken
        // false: predict not-taken
        bool finalPred;
      };

    /**
     *  Returns the taken/not taken prediction given the value of the
     *  counter.
     *  @param count The value of the counter.
     *  @return The prediction based on the counter value.
     */
    inline bool getPrediction(uint8_t &count);

    /** Calculates the predictor index based on the PC & Global History. */
    inline unsigned getPredictorIndex(Addr &branch_addr, unsigned historyReg);

    /** function to update global history register. */
    void updateGlobalHistReg(ThreadID tid, bool taken);

    /** Size of the gselect predictor. */
    const unsigned gSelectPredictorSize;

    /*Global Branch History Register*/
    std::vector<unsigned> globalHistoryReg;

    /*Mask to get the global history bits*/
    unsigned globalHistoryMask;

    /** Number of bits of the gselect predictor's counters. */
    const unsigned phtCounterBits;

    /** Array of counters that make up the gselect predictor. */
    std::vector<SatCounter8> predictorCounters;

    /** Mask to get index bits. */
    const unsigned indexMask;
};

#endif // __CPU_PRED_2BIT_LOCAL_PRED_HH__
