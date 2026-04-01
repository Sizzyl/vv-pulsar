/*
Hybrid Drift Implementation
Original code by Ismy and CLF78, with fixes by Ro
Adapted for vv-pulsar - always enabled, replaces automatic drift
*/

#include <kamek.hpp>

namespace HybridDrift {

// Main hybrid drift function - fixes drift behavior for all vehicle types
asmFunc GetHybridDrift1() {
    ASM(
        lwz r0, 0x14(r3);
        
        // Check if we're an inside drifting bike, if we are, end the code
        // (Bug is only for outside drift bikes)
        lwz r12, 0(r28);
        lwz r12, 0(r12);
        lwz r12, 0x14(r12);
        lwz r12, 0(r12);
        lwz r12, 0x4(r12);
        cmpwi r12, 2;
        beqlr;
        
        // Check if we're a kart, if we are, skip to the end
        cmpwi r12, 0;
        beq skip;
        
        // Check if we're drifting, if we are, end the code
        outsidefix:
        lwz r3, 4(r3);
        andis. r3, r3, 0x1000;
        bnelr;
        
        // Fix slide issue (Load a dummy value to force the next branch)
        skip:
        li r0, 0;
        blr;
    )
}

// Hook into the game at the drift check location
kmCall(0x8057930C, GetHybridDrift1);

// Additional hook for second drift check
asmFunc GetHybridDrift2() {
    ASM(
        lwz r0, 0x14(r3);
        
        lwz r12, 0(r30);
        lwz r12, 0(r12);
        lwz r12, 0x14(r12);
        lwz r12, 0(r12);
        lwz r12, 0x4(r12);
        cmpwi r12, 2;
        beqlr;
        
        cmpwi r12, 0;
        beq skip;
        
        outsidefix:
        lwz r3, 4(r3);
        andis. r3, r3, 0x1000;
        bnelr;
        
        skip:
        li r0, 0;
        blr;
    )
}

kmCall(0x805793F8, GetHybridDrift2);

} // namespace HybridDrift
