#include <assert.h>
#include <string.h>
#include "../country_override_pure.h"

static void modes_and_authorization_fail_closed() {
  for (unsigned mode = 0; mode <= 255; ++mode) {
    assert(countryOverrideModeValidPure(mode) == (mode <= 3));
    for (unsigned gates = 0; gates < 4; ++gates) {
      const bool expected = gates == 3 && (mode >= 1 && mode <= 3);
      assert(countryOverrideGateOpenPure(mode, gates & 1, gates & 2) == expected);
    }
  }
}

static void only_supported_route_local_buses_are_allowed() {
  struct Route { uint8_t profile, topology; bool a238, b238, a7ff, b7ff; };
  const Route routes[] = {
    {1, 1, false, true, true, true},
    {2, 2, true, true, true, true}, {2, 3, false, true, true, true},
    {3, 2, true, true, true, true}, {3, 3, false, true, true, true},
    {4, 2, true, true, true, true}, {4, 3, false, true, true, true},
    {5, 2, true, true, true, true}, {5, 3, false, true, true, true}
  };
  for (const Route &r : routes) {
    assert(countryOverrideRouteAllowedPure(r.profile, r.topology, 0, 0x238) == r.a238);
    assert(countryOverrideRouteAllowedPure(r.profile, r.topology, 1, 0x238) == r.b238);
    assert(countryOverrideRouteAllowedPure(r.profile, r.topology, 0, 0x7ff) == r.a7ff);
    assert(countryOverrideRouteAllowedPure(r.profile, r.topology, 1, 0x7ff) == r.b7ff);
    assert(!countryOverrideRouteAllowedPure(r.profile, r.topology, 2, 0x238));
    assert(!countryOverrideRouteAllowedPure(r.profile, r.topology, 255, 0x7ff));
    assert(!countryOverrideRouteAllowedPure(r.profile, r.topology, 0, 0x334));
  }
  for (uint8_t profile = 0; profile <= 6; ++profile) {
    for (uint8_t topology = 0; topology <= 4; ++topology) {
      const bool valid = profile == 1 ? topology == 1 :
          profile >= 2 && profile <= 5 && (topology == 2 || topology == 3);
      if (valid) continue;
      for (uint8_t bus = 0; bus < 2; ++bus) {
        assert(!countryOverrideRouteAllowedPure(profile, topology, bus, 0x238));
        assert(!countryOverrideRouteAllowedPure(profile, topology, bus, 0x7ff));
      }
    }
  }
}

static void country_238_changes_only_country_counter_checksum() {
  struct Fixture { uint8_t source[8], mode, expected[8]; };
  const Fixture fixtures[] = {
    {{0xB0,0x23,0x9A,0xDD,0x1F,0x00,0xF6,0x99},1,
     {0xB0,0x23,0x48,0xDF,0x1F,0x00,0x06,0x59}},
    {{0xB0,0x23,0x48,0xDF,0x1F,0x00,0xF6,0x49},2,
     {0xB0,0x23,0x9A,0xDD,0x1F,0x00,0x06,0xA9}},
    // France is valid for either destination; these are not KR-only rewrites.
    {{0xB0,0x23,0xFA,0xDC,0x1F,0x00,0xF6,0xF8},1,
     {0xB0,0x23,0x48,0xDF,0x1F,0x00,0x06,0x59}},
    {{0xB0,0x23,0xFA,0xDC,0x1F,0x00,0xF6,0xF8},2,
     {0xB0,0x23,0x9A,0xDD,0x1F,0x00,0x06,0xA9}},
    // Non-wrapping counter, preserving the lower nibble.
    {{0xB0,0x23,0xFA,0xDC,0x1F,0x00,0x26,0x28},2,
     {0xB0,0x23,0x9A,0xDD,0x1F,0x00,0x36,0xD9}},
    {{0xB0,0x23,0xFA,0xDC,0x1F,0x00,0xF6,0xF8},3,
     {0xB0,0x23,0x2A,0xDE,0x1F,0x00,0x06,0x3A}}
  };
  for (const Fixture &f : fixtures) {
    uint8_t actual[8]; memcpy(actual, f.source, 8);
    assert(countryOverrideApply238Pure(actual, 8, f.mode));
    assert(memcmp(actual, f.expected, 8) == 0);
    assert(countryOverrideRead238CountryPure(actual, 8) == (f.mode == 1 ? 840 : f.mode == 2 ? 410 : 554));
    assert(!countryOverrideApply238Pure(actual, 8, f.mode));
    assert(memcmp(actual, f.expected, 8) == 0);
  }
  assert(countryOverrideRead238CountryPure(nullptr, 8) == 0);
  const uint8_t shortFrame[3] = {0,0,0};
  assert(countryOverrideRead238CountryPure(shortFrame, 3) == 0);
  assert(countryOverride238ChecksumPure(nullptr) == 0);
}

static void malformed_238_never_changes_bytes() {
  const uint8_t invalid[][8] = {
    {0xB0,0x23,0xFA,0xDC,0x1F,0x00,0xF6,0x00}, // Bad checksum.
    {0xB0,0x23,0x00,0xDC,0x1F,0x00,0xF6,0xFE}, // UNKNOWN.
    {0xB0,0x23,0xFF,0xDF,0x1F,0x00,0xF6,0xFE}, // SNA.
    {0xB0,0x23,0xE8,0xDF,0x1F,0x00,0xF6,0xE9}  // Outside three-digit domain.
  };
  for (const auto &f : invalid) {
    for (uint8_t mode = 1; mode <= 3; ++mode) {
      uint8_t actual[8]; memcpy(actual, f, 8);
      assert(!countryOverrideApply238Pure(actual, 8, mode));
      assert(memcmp(actual, f, 8) == 0);
    }
  }
  const uint8_t france[8] = {0xB0,0x23,0xFA,0xDC,0x1F,0x00,0xF6,0xF8};
  for (unsigned mode = 0; mode <= 255; ++mode) {
    uint8_t actual[8]; memcpy(actual, france, 8);
    if (mode != 1 && mode != 2 && mode != 3) {
      assert(!countryOverrideApply238Pure(actual, 8, mode));
      assert(memcmp(actual, france, 8) == 0);
    }
    assert(!countryOverrideApply238Pure(nullptr, 8, mode));
  }
  for (uint8_t len = 0; len <= 9; ++len) {
    if (len == 8) continue;
    uint8_t actual[8]; memcpy(actual, france, 8);
    assert(!countryOverrideApply238Pure(actual, len, 1));
    assert(memcmp(actual, france, 8) == 0);
  }
}

static void country_7ff_preserves_other_page_fields() {
  const uint8_t sources[][2] = {{0x52,0x46}, {0x52,0x4B}, {0x53,0x55}, {0x5A,0x4E}};
  for (const auto &source : sources) {
    for (uint8_t mode = 1; mode <= 3; ++mode) {
      uint8_t actual[8] = {1,0xA5,source[0],source[1],0x62,0xA5,0xE8,1};
      const uint8_t expectedUs[8] = {1,0xA5,0x53,0x55,0x62,0xA5,0xE8,1};
      const uint8_t expectedKr[8] = {1,0xA5,0x52,0x4B,0x62,0xA5,0xE8,1};
      const uint8_t expectedNz[8] = {1,0xA5,0x5A,0x4E,0x62,0xA5,0xE8,1};
      const uint8_t *expected = mode == 1 ? expectedUs : mode == 2 ? expectedKr : expectedNz;
      const bool changed = memcmp(actual, expected, 8) != 0;
      assert(countryOverrideApply7ffPure(actual, 8, mode) == changed);
      assert(memcmp(actual, expected, 8) == 0);
      assert(!countryOverrideApply7ffPure(actual, 8, mode));
    }
  }
  for (uint8_t region = 0; region < 16; ++region) {
    for (uint8_t mode = 1; mode <= 3; ++mode) {
      uint8_t actual[8] = {3, static_cast<uint8_t>(0xA0 | region),0xDC,0xAD,0xE1,0x20,0x30,0x64};
      uint8_t before[8]; memcpy(before, actual, 8);
      const uint8_t us[8] = {3,0xA0,0xDC,0xAD,0xE1,0x20,0x30,0x64};
      const uint8_t kr[8] = {3,0xA7,0xDC,0xAD,0xE1,0x20,0x30,0x64};
      const bool changed = mode != 3 && region <= 10 && region != (mode == 1 ? 0 : 7);
      assert(countryOverrideApply7ffPure(actual, 8, mode) == changed);
      assert(memcmp(actual, changed ? (mode == 1 ? us : kr) : before, 8) == 0);
    }
  }
}

static void malformed_or_unrelated_7ff_never_changes_bytes() {
  const uint8_t invalid[][2] = {{0,0},{0xFF,0xFF},{'r','F'},{'R','f'},{'@','F'},{'[','F'},{'R','0'}};
  for (const auto &source : invalid) {
    uint8_t actual[8] = {1,0xA5,source[0],source[1],0x62,0xA5,0xE8,1};
    uint8_t before[8]; memcpy(before, actual, 8);
    assert(!countryOverrideApply7ffPure(actual, 8, 1));
    assert(!countryOverrideApply7ffPure(actual, 8, 2));
    assert(!countryOverrideApply7ffPure(actual, 8, 3));
    assert(memcmp(actual, before, 8) == 0);
  }
  for (unsigned page = 0; page <= 255; ++page) {
    if (page == 1 || page == 3) continue;
    uint8_t actual[8] = {static_cast<uint8_t>(page),0xA1,'R','F',0x62,0xA5,0xE8,1};
    uint8_t before[8]; memcpy(before, actual, 8);
    assert(!countryOverrideApply7ffPure(actual, 8, 1));
    assert(!countryOverrideApply7ffPure(actual, 8, 2));
    assert(!countryOverrideApply7ffPure(actual, 8, 3));
    assert(memcmp(actual, before, 8) == 0);
  }
  const uint8_t france[8] = {1,0xA5,'R','F',0x62,0xA5,0xE8,1};
  for (unsigned mode = 0; mode <= 255; ++mode) {
    uint8_t actual[8]; memcpy(actual, france, 8);
    if (mode != 1 && mode != 2 && mode != 3) {
      assert(!countryOverrideApply7ffPure(actual, 8, mode));
      assert(memcmp(actual, france, 8) == 0);
    }
    assert(!countryOverrideApply7ffPure(nullptr, 8, mode));
  }
  for (uint8_t len = 0; len <= 9; ++len) {
    if (len == 8) continue;
    uint8_t actual[8]; memcpy(actual, france, 8);
    assert(!countryOverrideApply7ffPure(actual, len, 2));
    assert(memcmp(actual, france, 8) == 0);
  }
}

static void nz_preserves_handedness_and_all_mux1_flags() {
  for (unsigned flags=0; flags<256; ++flags) {
    uint8_t actual[8]={1,static_cast<uint8_t>(flags),'P','J',0x62,0xA5,0xE8,1};
    uint8_t expected[8]; memcpy(expected, actual,8);
    expected[2]='Z'; expected[3]='N';
    assert(countryOverrideApply7ffPure(actual,8,3));
    assert(memcmp(actual,expected,8)==0);
  }
}

int main() {
  nz_preserves_handedness_and_all_mux1_flags();
  modes_and_authorization_fail_closed();
  only_supported_route_local_buses_are_allowed();
  country_238_changes_only_country_counter_checksum();
  malformed_238_never_changes_bytes();
  country_7ff_preserves_other_page_fields();
  malformed_or_unrelated_7ff_never_changes_bytes();
  return 0;
}
