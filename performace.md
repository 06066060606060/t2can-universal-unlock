[mcp2515.cpp] updated:
- SPI transfers using transferBytes() on ESP32.
- Buffers aligned with alignas(4), and CAN payload alignment in can.h.
- Dedicated READ RX BUFFER commands for reception.
- Dedicated LOAD TX BUFFER commands followed by RTS for transmission.
- A single READ STATUS operation to check the three hardware TX buffers through TXREQ bits 2, 4 and 6.
- DLC validation and handling of standard, extended and RTR frames.
I retained these paths and kept the configured 10 MHz SPI clock. The new instrumentation calls the existing driver.

Receive scheduling in [can_runtime.h]
Previously, BODY could process up to 32 frames, and CHASSIS up to 64 frames, before finishing a pass.
I replaced these large batches with smaller service slices:
CAN_RX_SLICE_FRAMES = 4;
CAN_RX_SLICE_US = 600;
CAN_RX_MAX_BURST_US = 2000;
CAN_RX_TASK_PRIORITY = 4;
These constants are centralized in [can_service_policy_pure.h].
- BODY and PARTY alternate which bus is serviced first.
- Each bus handles at most four frames per slice.
- The 600 µs limit is checked between frames or batches.
- BODY still processes every frame already prefetched.
- Tasks yield between slices so CHASSIS can run.
600 µs is a cooperative limit: a decoder, SPI transaction or preemption can exceed it.
I also removed the two-tick delay after an unsuccessful initial CHASSIS read, by calling:
mcpChassisReceive(&f, 0, &chassisReadUs);
The previous unconditional one-tick pause after every pass now depends on activity: tasks yield while receiving traffic, then pause for one tick when idle or after approximately 2 ms of continuous burst time.

Task priorities in [TMR-Universal-v3.21.2.ino]
Your three tasks remain:
- canA: BODY and PARTY reception.
- canB: CHASSIS reception.
- canSup: supervision and recovery.
Previously, canA had priority 5, while canB had priority 4. Both receive tasks now use priority 4 on core 1, allowing the yields to share execution more fairly.
The same priorities are applied when recovery recreates the tasks. canSup remains on core 0.

Driver integration in [can_core.h]
I wrapped the relevant driver calls with measurement functions:
canPerfRead(controller, bus, frame, &readUs);
canPerfSend(controller, bus, frame);
These wrappers record timings and results while using the existing readMessage() and sendMessage() implementations.
CHASSIS transmission now distinguishes controller saturation from other failures:
- ERROR_OK → ESP_OK
- ERROR_ALLTXBUSY → ESP_ERR_TIMEOUT
- Other send failures → ESP_FAIL
In [vehicle_logic.h], I also reduced the CHASSIS receive budget to four

RX overflow handling
I added overflow observation for BODY, PARTY and CHASSIS on every service pass. BODY previously checked this in its once-per-second status block.
The receive loops now call:
controller.clearRXnOVRFlags();
This clears only RX0OVR and RX1OVR in EFLG, preserving CANINTF. I changed the application calls; 
