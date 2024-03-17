#include "cpu/pred/gselect.hh"

#include "base/intmath.hh"
#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/Fetch.hh"
#include "debug/GSDebug.hh"

GSelectBP::GSelectBP(const GSelectBPParams& params)
    : BPredUnit(params)
    , gSelectPredictorSize(params.PredictorSize)
    , globalHistoryReg(params.numThreads, 0)
    , globalHistoryMask(mask(params.globalHistoryBits))
    , phtCounterBits(params.PHTCtrBits)
    , predictorCounters(gSelectPredictorSize, SatCounter8(phtCounterBits))
    , indexMask(mask(params.globalHistoryBits))
{
    /*sanity check for invalid size*/
    if (!isPowerOf2(gSelectPredictorSize)) {
        fatal("Invalid local predictor size!\n");
    }

    /*Debug logs*/
    DPRINTF(GSDebug, "index mask: %#x\n", indexMask);

    DPRINTF(GSDebug, "gselect predictor size: %i\n",
            gSelectPredictorSize);

    DPRINTF(GSDebug, "PHT counter bits: %i\n", phtCounterBits);

    DPRINTF(GSDebug, "instruction shift amount: %i\n",
            instShiftAmt);
}

void GSelectBP::btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history)
{
    DPRINTF(GSDebug, "Inside btbupdate");
    /* Make the LSB in the global history register 0 to represent NOT Taken if a BTB entry is
     * invalid or not found*/
    globalHistoryReg[tid] &= (globalHistoryMask & ~ULL(1)); 
}


bool GSelectBP::lookup(ThreadID tid, Addr branch_addr, void * &bp_history)
{
    
    bool taken;
    unsigned predictor_idx = getPredictorIndex(branch_addr, globalHistoryReg[tid]);

    /*sanity check before indexing*/
    assert(predictor_idx < gSelectPredictorSize);
    DPRINTF(GSDebug, "Looking up index %u\n", predictor_idx);

    BPHistory *history = new BPHistory;
    /*copy the global history register into BPHistory struct*/
    history->globalHistoryReg = globalHistoryReg[tid]; 

    /*get the counter value from the pattern history table*/
    uint8_t counter_val = predictorCounters[predictor_idx];
    taken = getPrediction(counter_val);

    DPRINTF(GSDebug, "Inside lookup. Global History Register: %u , Branch Address: %#x\n", globalHistoryReg[tid], branch_addr);
    DPRINTF(GSDebug, "prediction is %i.\n",
            (int)counter_val);

    /*copy the final prediction into BPHistory struct*/
    history->finalPred = taken;
    bp_history = static_cast<void*>(history);

    /*Speculative update of global history register*/
    updateGlobalHistReg(tid, taken);
    
    return taken;
}

void GSelectBP::update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed, const StaticInstPtr & inst, Addr corrTarget)
{
    DPRINTF(GSDebug, "Inside update");
    /*Sanity check for null pointer*/
    assert(bp_history);
    unsigned predictor_idx;

    BPHistory *history = static_cast<BPHistory*>(bp_history);

    // Update the local predictor.
    predictor_idx = getPredictorIndex(branch_addr, history->globalHistoryReg);

    DPRINTF(GSDebug, "Looking up index %u\n", predictor_idx);

    /*sanity check before indexing*/
    assert(predictor_idx < gSelectPredictorSize);

    if (taken) {
        DPRINTF(GSDebug, "Branch updated as taken.\n");
        /*Increment the counter if taken*/
        predictorCounters[predictor_idx]++;
    } else {
        DPRINTF(GSDebug, "Branch updated as not taken.\n");
        /*Decrement the counter if NOT taken*/
        predictorCounters[predictor_idx]--;
    }

    /*check if branch was mispredicted*/
    if(squashed)
    {
        /*if mispredicted update the global history register based on actual branch execution*/
        if(taken)
            globalHistoryReg[tid] = (history->globalHistoryReg << 1) | 1;
        else
            globalHistoryReg[tid] = (history->globalHistoryReg << 1);
            globalHistoryReg[tid] &= globalHistoryMask;
    }
    else
    {
        /*free the memory*/
        delete history;
    }
}

inline bool GSelectBP::getPrediction(uint8_t &count)
{
    DPRINTF(GSDebug, "Inside getPrediction");
    /*Get the MSB of the count which gives the branch prediction
      Branch Taken if MSB = 1
      Branch Not Taken if MSB = 0
    */
    return (count >> (phtCounterBits - 1));
}

inline unsigned GSelectBP::getPredictorIndex(Addr &branch_addr, unsigned historyReg)
{
    DPRINTF(GSDebug, "Inside getPredictorIndex");
    /* Shift the PC, then bitwise AND with mask to obtain lower 8 bits of PC 
    XNOR 8-bits of global branch history with 8 bits of PC obtained above*/
    return ~(((branch_addr >> instShiftAmt) & indexMask) ^ historyReg) & globalHistoryMask;
}

void GSelectBP::uncondBranch(ThreadID tid, Addr pc, void *&bp_history)
{
    DPRINTF(GSDebug, "Inside uncondBranch. Global History Register: %u , Branch Address: %#x\n", globalHistoryReg[tid], pc);
    BPHistory *history = new BPHistory;
    history->globalHistoryReg = globalHistoryReg[tid];
    /*Prediction is always Taken for Unconidtional Branches*/
    history->finalPred = true;
    bp_history = static_cast<void*>(history);
    updateGlobalHistReg(tid, true);
}

void GSelectBP::updateGlobalHistReg(ThreadID tid, bool taken)
{
    /*Update the global history register with the latest branch prediction
      by inserting the branch prediction at the LSB
      Insert 1 if taken or else insert 0 for NOT taken*/
    globalHistoryReg[tid] = taken ? (globalHistoryReg[tid] << 1) | 1 :
                               (globalHistoryReg[tid] << 1);
    globalHistoryReg[tid] &= globalHistoryMask;
    DPRINTF(GSDebug, "Inside updateGlobalHistReg. Updated globalHistoryReg: %u", globalHistoryReg[tid]);
}

void GSelectBP::squash(ThreadID tid, void *bp_history)
{
    DPRINTF(GSDebug, "Global History before squashing: %u", globalHistoryReg[tid]);
    BPHistory *history = static_cast<BPHistory*>(bp_history);
    /*Restore the global history register from the BPHistory Struct which was updated 
    before the speculative update of branch history register*/
    globalHistoryReg[tid] = history->globalHistoryReg;
    DPRINTF(GSDebug, "Global History after squashing: %u", globalHistoryReg[tid]);

    /*free the memory*/
    delete history;
}
