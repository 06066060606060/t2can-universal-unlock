from pathlib import Path
R=Path(__file__).resolve().parents[1]
d=(R/'dashboard_source.html').read_text()
a=(R/'web_api.h').read_text()
c=(R/'can_core.h').read_text()
v4=(R/'nag_human_v4_pure.h').read_text()
assert 'id="modeHRev4"' not in d and 'REV.4' not in d
assert 'id="modeHRev1Plus"' not in d
assert 'id="humanV4CarrierMinNm"' in d and 'id="humanV4CarrierMaxNm"' in d
assert 'const humanPeakEdit={dirty:false,saving:false,epoch:0}' in d
assert 'function markHumanPeakDirty()' in d
assert 'const syncTuning=forceTuning||(!humanPeakEdit.dirty&&!humanPeakEdit.saving)' in d
assert 'peakEpoch!==humanPeakEdit.epoch||stopEpoch!==humanStopEdit.epoch' in d
assert 'humanPeakEdit.epoch++' in d
assert 'H_VARIANT_REV4' in c and 'nagHumanV4RuntimeSetLabTuning' in c
assert 'nagHumanV4RuntimeConfigSnapshot' in a and 'OPPOSITE_STOCK_PER_RX' in a
assert 'NAG_HUMAN_V4_STOCK_DEADBAND_RAW = 5u' in v4
assert 'NAG_HUMAN_V4_NEGATIVE_BIAS_PCT = 80u' in v4
for token in ['c.carrierMinRaw = 10u','c.carrierMaxRaw = 60u','c.base.peakMinRaw = 180u','c.base.peakMaxRaw = 260u','c.base.waitMinMs = 900u','c.base.waitMaxMs = 3000u','c.base.refractoryMinMs = 500u','c.base.refractoryMaxMs = 1500u','c.hoPolicy = H3_HO_TIERED_1_2','c.ho1ThresholdRaw = 40u','c.ho2ThresholdRaw = 200u','c.visualRescueEnabled = true']:
    assert token in v4, token
print('PASS v3.6f1 Mode H Rev.4/dashboard edit-state contract')
