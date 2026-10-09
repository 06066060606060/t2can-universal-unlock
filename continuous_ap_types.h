#pragma once
#include <stdint.h>
enum class ContApMethod : uint8_t { Unset, StalkDouble, ScrollSingle, ScrollDouble };
enum class ContApRoute : uint8_t { None, BodyA, ChassisB, PartyA };
struct ContApConfig { bool enabled = false; ContApMethod method = ContApMethod::Unset; };
struct ContApStockFrame {
  ContApRoute route=ContApRoute::None; uint16_t id=0; uint8_t dlc=0;
  uint8_t data[8]={}; uint32_t epoch=0, rxMs=0; bool valid=false;
};
enum class ContApSignal : uint8_t { None, Ap, Turn, Brake, Gear, Torque, Stalk, Scroll };
struct ContApDecodedInputs {
  ContApSignal signal=ContApSignal::None; bool valid=false, idle=false;
  int32_t value=0; uint8_t left=0,right=0;
};
struct ContApObservation { bool seen=false,valid=false; int32_t value=0; uint32_t rxMs=0,epoch=0; };
enum class ContApState : uint8_t { Idle, WaitSignalOff, WaitDelayAndReady, SendGesture, WaitResult, RetryWait, CleanupPending };
enum class ContApReason : uint8_t { None, Disabled, Unsupported, WaitingActive, WaitingSignal, WaitingReady, Stale, DriverInput, Brake, Gear, ApState, Torque, Deadline, Changed, Transport, Complete, CleanupUnconfirmed };
enum class ContApCommandKind : uint8_t { None, Press, Release };
struct ContApInputs {
  uint32_t nowMs=0,epoch=0,generation=0; uint8_t profile=0,topology=0; ContApConfig config;
  ContApObservation ap,turn,brake,gear,torque,control;
  bool templateFresh=false,transportAllowed=false,ownershipAllowed=false,ambiguous=false;
};
struct ContApCommand {
  ContApCommandKind kind=ContApCommandKind::None; ContApMethod method=ContApMethod::Unset;
  ContApRoute route=ContApRoute::None; uint8_t phase=0;
  uint32_t generation=0,epoch=0,token=0,preparedAtMs=0;
};
struct ContApStatus {
  ContApState state=ContApState::Idle; ContApReason reason=ContApReason::WaitingActive;
  uint32_t attempts=0,txOk=0,txFail=0,cleanupSends=0;
  bool releaseOwed=false,cleanupUnconfirmed=false;
};

static inline const char *continuousApStateName(ContApState state){
  static const char *const names[]={"Idle","Waiting for signal off","Waiting for readiness","Sending gesture","Waiting for AP","Retry wait","Release pending"};
  const unsigned n=(unsigned)state;return n<7?names[n]:"Unknown";
}
static inline const char *continuousApReasonName(ContApReason reason){
  static const char *const names[]={"Ready","Disabled","Unsupported method for this vehicle connection","Waiting for AP active","Waiting for signal off","Waiting for readiness","Waiting for fresh vehicle state","Driver input","Brake applied","Drive gear required","AP unavailable","Steering torque","Episode limit","Settings or connection changed","CAN unavailable","Complete","Release unconfirmed"};
  const unsigned n=(unsigned)reason;return n<17?names[n]:"Unknown";
}
