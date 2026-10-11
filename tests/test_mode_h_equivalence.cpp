// Differential oracle: byte-identical immutable v3.28.3 headers in fixtures.
// Compare every named member rather than padding, with identical input streams.
#include <stdint.h>
#include <cstddef>
#include <initializer_list>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace legacy {
#include "fixtures/mode_h_legacy/nag_human_v4_pure.h"
#include "fixtures/mode_h_legacy/nag_mode_h_variant_pure.h"
}
#include "../mode_h_pure.h"
static_assert(sizeof(legacy::NagHumanV1ConfigPure) == sizeof(ModeHBaseConfigPure), "persisted config size");
static_assert(alignof(legacy::NagHumanV1ConfigPure) == alignof(ModeHBaseConfigPure), "persisted config alignment");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, waitMinMs) == offsetof(ModeHBaseConfigPure, waitMinMs), "persisted offset waitMinMs");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, waitMaxMs) == offsetof(ModeHBaseConfigPure, waitMaxMs), "persisted offset waitMaxMs");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, rampInMinMs) == offsetof(ModeHBaseConfigPure, rampInMinMs), "persisted offset rampInMinMs");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, rampInMaxMs) == offsetof(ModeHBaseConfigPure, rampInMaxMs), "persisted offset rampInMaxMs");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, interactMinMs) == offsetof(ModeHBaseConfigPure, interactMinMs), "persisted offset interactMinMs");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, interactMaxMs) == offsetof(ModeHBaseConfigPure, interactMaxMs), "persisted offset interactMaxMs");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, rampOutMinMs) == offsetof(ModeHBaseConfigPure, rampOutMinMs), "persisted offset rampOutMinMs");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, rampOutMaxMs) == offsetof(ModeHBaseConfigPure, rampOutMaxMs), "persisted offset rampOutMaxMs");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, refractoryMinMs) == offsetof(ModeHBaseConfigPure, refractoryMinMs), "persisted offset refractoryMinMs");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, refractoryMaxMs) == offsetof(ModeHBaseConfigPure, refractoryMaxMs), "persisted offset refractoryMaxMs");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, peakMinRaw) == offsetof(ModeHBaseConfigPure, peakMinRaw), "persisted offset peakMinRaw");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, peakMaxRaw) == offsetof(ModeHBaseConfigPure, peakMaxRaw), "persisted offset peakMaxRaw");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, correctionMinRaw) == offsetof(ModeHBaseConfigPure, correctionMinRaw), "persisted offset correctionMinRaw");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, correctionMaxRaw) == offsetof(ModeHBaseConfigPure, correctionMaxRaw), "persisted offset correctionMaxRaw");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, directionPersistencePct) == offsetof(ModeHBaseConfigPure, directionPersistencePct), "persisted offset directionPersistencePct");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, correctionProbabilityPct) == offsetof(ModeHBaseConfigPure, correctionProbabilityPct), "persisted offset correctionProbabilityPct");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, hoOverridePct) == offsetof(ModeHBaseConfigPure, hoOverridePct), "persisted offset hoOverridePct");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, stopThresholdKphX100) == offsetof(ModeHBaseConfigPure, stopThresholdKphX100), "persisted offset stopThresholdKphX100");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, moveThresholdKphX100) == offsetof(ModeHBaseConfigPure, moveThresholdKphX100), "persisted offset moveThresholdKphX100");
static_assert(offsetof(legacy::NagHumanV1ConfigPure, moveConfirmMs) == offsetof(ModeHBaseConfigPure, moveConfirmMs), "persisted offset moveConfirmMs");
static_assert(sizeof(legacy::NagHumanV4ConfigPure) == sizeof(ModeHConfigPure), "persisted config size");
static_assert(alignof(legacy::NagHumanV4ConfigPure) == alignof(ModeHConfigPure), "persisted config alignment");
static_assert(offsetof(legacy::NagHumanV4ConfigPure, base) == offsetof(ModeHConfigPure, base), "persisted offset base");
static_assert(offsetof(legacy::NagHumanV4ConfigPure, carrierMinRaw) == offsetof(ModeHConfigPure, carrierMinRaw), "persisted offset carrierMinRaw");
static_assert(offsetof(legacy::NagHumanV4ConfigPure, carrierMaxRaw) == offsetof(ModeHConfigPure, carrierMaxRaw), "persisted offset carrierMaxRaw");
static_assert(offsetof(legacy::NagHumanV4ConfigPure, hoPolicy) == offsetof(ModeHConfigPure, hoPolicy), "persisted offset hoPolicy");
static_assert(offsetof(legacy::NagHumanV4ConfigPure, ho1ThresholdRaw) == offsetof(ModeHConfigPure, ho1ThresholdRaw), "persisted offset ho1ThresholdRaw");
static_assert(offsetof(legacy::NagHumanV4ConfigPure, ho2ThresholdRaw) == offsetof(ModeHConfigPure, ho2ThresholdRaw), "persisted offset ho2ThresholdRaw");
static_assert(offsetof(legacy::NagHumanV4ConfigPure, visualRescueEnabled) == offsetof(ModeHConfigPure, visualRescueEnabled), "persisted offset visualRescueEnabled");
static_assert(offsetof(legacy::NagHumanV4ConfigPure, visualRescueDelayMs) == offsetof(ModeHConfigPure, visualRescueDelayMs), "persisted offset visualRescueDelayMs");
static uint32_t seedContext, stepContext, scenarioContext;
static unsigned long long checks=0, steps=0;
static uint32_t phasesSeen=0,blocksSeen=0;
static bool rescueSeen=false,simpleSeen=false,correctionSeen=false;
#define SAME(member) do { ++checks; if(a.member!=b.member) { std::fprintf(stderr,"Mismatch %s seed=%u step=%u scenario=%u\n",#member,seedContext,stepContext,scenarioContext); std::abort(); } } while(0)
#define REQUIRE(expr) do { ++checks; if(!(expr)) { std::fprintf(stderr,"Failed %s at %d\n",#expr,__LINE__);std::abort(); } } while(0)
static void compareBaseConfig(const legacy::NagHumanV1ConfigPure &a,const ModeHBaseConfigPure &b) {
  SAME(waitMinMs);
  SAME(waitMaxMs);
  SAME(rampInMinMs);
  SAME(rampInMaxMs);
  SAME(interactMinMs);
  SAME(interactMaxMs);
  SAME(rampOutMinMs);
  SAME(rampOutMaxMs);
  SAME(refractoryMinMs);
  SAME(refractoryMaxMs);
  SAME(peakMinRaw);
  SAME(peakMaxRaw);
  SAME(correctionMinRaw);
  SAME(correctionMaxRaw);
  SAME(directionPersistencePct);
  SAME(correctionProbabilityPct);
  SAME(hoOverridePct);
  SAME(stopThresholdKphX100);
  SAME(moveThresholdKphX100);
  SAME(moveConfirmMs);
}
static void compareEvent(const legacy::NagHumanV1EventPure &a,const ModeHBaseEventPure &b) {
  SAME(waitDurationMs);
  SAME(rampInDurationMs);
  SAME(interactDurationMs);
  SAME(rampOutDurationMs);
  SAME(refractoryDurationMs);
  SAME(peakRaw);
  SAME(correctionRaw);
  SAME(direction);
  SAME(type);
}
static void compareBaseState(const legacy::NagHumanV1StatePure &a,const ModeHBaseStatePure &b) {
  SAME(phase);
  SAME(motion);
  SAME(movingConfirmed);
  SAME(moveCandidateActive);
  SAME(eventValid);
  SAME(carrier);
  SAME(moveCandidateStartMs);
  SAME(phaseStartMs);
  SAME(eventStartMs);
  SAME(sessionStartMs);
  SAME(rngState);
  SAME(previousDirection);
  SAME(rampBaselineRaw);
  SAME(outputRaw);
  compareEvent(a.event,b.event);
  SAME(eventCount);
  SAME(simpleCount);
  SAME(correctionCount);
  SAME(hoDecisionValid);
  SAME(hoOverrideActive);
  SAME(hoDecisionStartMs);
  SAME(hoDecisionDurationMs);
}
static void compareResult(const legacy::NagHumanV1StepResultPure &a,const ModeHBaseStepResultPure &b) {
  SAME(tx);
  SAME(setHo);
  SAME(hoOverrideValid);
  SAME(hoLevel);
  SAME(carrier);
  SAME(raw);
  SAME(phase);
  SAME(motion);
  SAME(blockReason);
}
static void compareConfig(const legacy::NagHumanV4ConfigPure &a,const ModeHConfigPure &b) {
  compareBaseConfig(a.base,b.base);
  SAME(carrierMinRaw);
  SAME(carrierMaxRaw);
  SAME(hoPolicy);
  SAME(ho1ThresholdRaw);
  SAME(ho2ThresholdRaw);
  SAME(visualRescueEnabled);
  SAME(visualRescueDelayMs);
}
static void compareState(const legacy::NagHumanV4StatePure &a,const ModeHStatePure &b) {
  compareBaseState(a.base,b.base);
  SAME(carrierRngState);
  SAME(lastCarrierMagnitudeRaw);
  SAME(lastCarrierApplied);
  SAME(rampOutTargetRaw);
  SAME(rampOutTargetValid);
  SAME(visualWarningEpochSeen);
  SAME(visualRescuePendingSinceMs);
  SAME(visualRescueCount);
  SAME(visualRescuePending);
}

static uint32_t nextRandom(uint32_t &x) { x^=x<<13;x^=x>>17;x^=x<<5;return x; }
static void defaultsAndMigrations() {
  auto a=legacy::nagHumanV4DefaultConfigPure(); auto b=modeHDefaultConfigPure();
  compareConfig(a,b);
  REQUIRE(legacy::nagHumanV4ConfigValidPure(a)==modeHConfigValidPure(b));
  for(unsigned version=16;version<=17;++version) for(unsigned tweak=0;tweak<4;++tweak) {
    a=legacy::nagHumanV4DefaultConfigPure();b=modeHDefaultConfigPure();
    if(version==16) { a.base.peakMinRaw=b.base.peakMinRaw=150;a.base.peakMaxRaw=b.base.peakMaxRaw=210;a.base.waitMaxMs=b.base.waitMaxMs=2000;a.visualRescueEnabled=b.visualRescueEnabled=false; }
    else a.ho2ThresholdRaw=b.ho2ThresholdRaw=175;
    if(tweak==1)++a.base.peakMaxRaw,++b.base.peakMaxRaw;
    if(tweak==2)++a.carrierMaxRaw,++b.carrierMaxRaw;
    if(tweak==3)++a.visualRescueDelayMs,++b.visualRescueDelayMs;
    const bool x=version==16?legacy::nagHumanV4MigrateV16DefaultPure(a):legacy::nagHumanV4MigrateV17DefaultPure(a);
    const bool y=version==16?modeHMigrateV16DefaultPure(b):modeHMigrateV17DefaultPure(b);
    REQUIRE(x==y);REQUIRE(x==(tweak==0));compareConfig(a,b);
  }
  for(unsigned field=0;field<9;++field) for(unsigned value: {0u,9u,10u,80u,100u,255u,300u,2000u,65535u}) {
    a=legacy::nagHumanV4DefaultConfigPure();b=modeHDefaultConfigPure();
    switch(field) {
      case 0:a.base.peakMinRaw=b.base.peakMinRaw=value;break;
      case 1:a.base.peakMaxRaw=b.base.peakMaxRaw=value;break;
      case 2:a.base.waitMinMs=b.base.waitMinMs=value;break;
      case 3:a.base.refractoryMaxMs=b.base.refractoryMaxMs=value;break;
      case 4:a.carrierMinRaw=b.carrierMinRaw=value;break;
      case 5:a.carrierMaxRaw=b.carrierMaxRaw=value;break;
      case 6:a.hoPolicy=b.hoPolicy=(uint8_t)value;break;
      case 7:a.ho2ThresholdRaw=b.ho2ThresholdRaw=value;break;
      case 8:a.visualRescueDelayMs=b.visualRescueDelayMs=value;break;
    }
    REQUIRE(legacy::nagHumanV4ConfigValidPure(a)==modeHConfigValidPure(b));compareConfig(a,b);
  }
  for(unsigned previous=0;previous<8;++previous)for(unsigned current=0;current<8;++current) {
    REQUIRE(legacy::nagHumanV4VisualWarningEdgePure(previous,current)==modeHVisualWarningEdgePure(previous,current));
    REQUIRE(legacy::nagHumanV4VisualWarningActivePure(current)==modeHVisualWarningActivePure(current));
  }
}
static void torqueAndHandsOn() {
  auto a=legacy::nagHumanV4DefaultConfigPure();auto b=modeHDefaultConfigPure();
  legacy::NagHumanV4StatePure sa{};ModeHStatePure sb{};
  legacy::nagHumanV4InitPure(sa,0);modeHInitPure(sb,0);
  for(unsigned raw=0;raw<=4095;++raw) {
    REQUIRE(legacy::nagHumanV4CarrierOutputPure(sa,a,raw)==modeHCarrierOutputPure(sb,b,raw));compareState(sa,sb);
    for(unsigned policy=0;policy<4;++policy) {
      const auto x=legacy::nagHumanV3HoDecisionPure(policy,legacy::nagHumanV3AbsTorqueRawPure(raw),40,200);
      const auto y=modeHHandsOnHoDecisionPure(policy,modeHHandsOnAbsTorqueRawPure(raw),40,200);
      REQUIRE(x.overrideValid==y.overrideValid);REQUIRE(x.level==y.level);
    }
  }
}
static void timelines() {
  const uint16_t torques[]={0,1,2044,2045,2046,2049,2050,2051,2054,2055,2056,4094,4095};
  const uint16_t speeds[]={0,500,502,503,507,508,509,625,4095};
  for(unsigned seedIndex=0;seedIndex<64;++seedIndex) for(unsigned policy=0;policy<3;++policy)
  for(unsigned rollover=0;rollover<2;++rollover) for(unsigned scenario=0;scenario<4;++scenario) {
    seedContext=seedIndex==0?0u:seedIndex==1?UINT32_MAX:seedIndex*2654435761u;scenarioContext=scenario;
    auto a=legacy::nagHumanV4DefaultConfigPure();auto b=modeHDefaultConfigPure();
    a.hoPolicy=b.hoPolicy=policy;
    if(scenario==1) { a.base.peakMinRaw=b.base.peakMinRaw=100;a.base.peakMaxRaw=b.base.peakMaxRaw=300;a.carrierMinRaw=b.carrierMinRaw=80;a.carrierMaxRaw=b.carrierMaxRaw=80;a.visualRescueDelayMs=b.visualRescueDelayMs=0; }
    if(scenario==2) { a.visualRescueEnabled=b.visualRescueEnabled=false;a.base.correctionProbabilityPct=b.base.correctionProbabilityPct=100; }
    if(scenario==3) { a.visualRescueDelayMs=b.visualRescueDelayMs=2000;a.base.correctionProbabilityPct=b.base.correctionProbabilityPct=0; }
    compareConfig(a,b);
    legacy::NagHumanV4StatePure sa{};ModeHStatePure sb{};
    legacy::nagHumanV4InitPure(sa,seedContext);modeHInitPure(sb,seedContext);compareState(sa,sb);
    uint32_t now=rollover?UINT32_MAX-12000u:1u,random=seedContext?seedContext:1u,epoch=0,enter=0;
    bool warning=false;
    for(unsigned i=0;i<2000;++i) {
      stepContext=i;const auto r=nextRandom(random);now+=(i%127==0?15000u:17u+r%84u);
      if(i%181==0) { ++epoch;enter=now;warning=true; }
      if(i%181==120)warning=false;
      const unsigned cycle=i%300;
      const bool run=scenario<2||cycle!=280;
      const bool valid=scenario<2||cycle!=281;
      const bool fresh=scenario<2||cycle!=282;
      const uint16_t speed=scenario<2||cycle<200?625:speeds[(cycle-200)%9];
      const uint16_t torque=i%2?torques[r%13]:(uint16_t)(r%4096);
      const auto x=legacy::nagHumanV4StepPure(sa,a,now,torque,run,valid,fresh,speed,epoch,enter,warning);
      const auto y=modeHStepPure(sb,b,now,torque,run,valid,fresh,speed,epoch,enter,warning);
      compareResult(x,y);compareState(sa,sb);++steps;
      phasesSeen|=1u<<y.phase;blocksSeen|=1u<<y.blockReason;
      rescueSeen|=sb.visualRescueCount>0;simpleSeen|=sb.base.simpleCount>0;correctionSeen|=sb.base.correctionCount>0;
      if(i==511||i==1555) { legacy::nagHumanV4ReseedPure(sa,r);modeHReseedPure(sb,r);compareState(sa,sb); }
      if(i==999) { const uint8_t phase=scenario%2?legacy::H1_PAUSED_STOPPED:legacy::H1_IDLE;legacy::nagHumanV4ResetRuntimePure(sa,phase);modeHResetRuntimePure(sb,phase);compareState(sa,sb); }
    }
  }
}
int main() {
  defaultsAndMigrations();torqueAndHandsOn();timelines();
  REQUIRE(phasesSeen==0x7fu);REQUIRE(blocksSeen==0x1fu);
  REQUIRE(rescueSeen&&simpleSeen&&correctionSeen);
  std::printf("Mode H differential: 1536 timelines, %llu steps, 4096 carrier inputs, 16384 HO decisions, 8 migrations, 81 validation cases, 64 visual edges; %llu field/assert comparisons PASS\n",steps,checks);
}
