# 1.2.1 changes from 1.2.0

- ConnectionManager: onUnavailable performs one immediate fresh automatic Wi-Fi request after teardown. No retiring-network probes on retry. Other errors do not retry.
- ConnectionRetryPolicy: one retry budget per launch/manual Connect sequence; disconnect and terminal failure cancel it.
- Waiting message now says Android Wi-Fi connection, rather than implying a user approval dialog is necessarily visible.
- Version 1.2.1 / code 5; signing identity unchanged.
- Added retry policy regression tests; updated launch emulator APK target.
- Original release directories remain unchanged.
