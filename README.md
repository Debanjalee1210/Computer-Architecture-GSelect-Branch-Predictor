# Computer Architecture GSelect Branch Predictor
 GSelect Branch Predictor in Gem5\ 
In the gem5 simulator a branch predictor (BP) can be implemented by defining
relevant mechanisms through following functions: lookup, update, uncondBranch,
btbUpdate, and squash

1. lookup: This function looks up a given branch address to index appropriate registers or
counters and determines if the branch needs to be taken or not taken. If global history is
used, it can be updated following the prediction. The parameters for the function are
branch_address (to look up) and bp_history (pointer to a new object may be set,
where the object contains shared state corresponding to the lookup for this branch). The
method returns the prediction, i.e., whether this branch needs to be taken or not.
2. update: Based on the actual taken/not taken information, this function updates the BP
including counters and global history associated with the branch.
- branch_address: the branch’s address that will be used to index appropriate counter. -
taken: whether the branch is actually taken or not
- bp_history: pointer to the shared state (aka global history) corresponding to this branch
- squashed: it is set to true when there is a misprediction. If so, the counter’s value should
not be updated. Also, the global history can be restored back if it was updated during the
prediction of the branch.
3. uncondBranch: This method is called when the branch instruction is an unconditional
branch. Branch predictor will not do anything but update the global history with taken.
4. btbUpdate: This function is called only when there is a branch miss in BTB. This can happen
when the BP does not know where to jump. In this case, the branch is explicitly predicted as not
taken so that branch target address is not required. So, any updates to the global history needs
to be modified to ensure prediction as not taken.
5. squash: This function squashes all outstanding updates until a given sequence number.
Typically, the input parameter is a bp_history (Pointer to the global history object associated
with this branch). The predictor will need to retrieve previous state and delete the object.
