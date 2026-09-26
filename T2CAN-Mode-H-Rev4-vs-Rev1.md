### 🛞 Mode H Rev.4 — Reworked from Rev.1

The original Rev.1 event flow is retained, but the behavior around each interaction has been significantly redesigned.

- **Opposite Carrier added**
  - Rev.1 follows stock torque during WAIT / REFRACTORY.
  - Rev.4 continuously applies a small torque opposite to the live stock torque between main interactions.

- **More natural transition**
  - Main interaction now ramps back into the carrier instead of simply returning to stock torque.

- **New direction behavior**
  - Rev.1 used previous-event direction persistence.
  - Rev.4 uses an asymmetric human-like direction bias independent of the current stock torque direction.

- **Hands-On handling redesigned**
  - Rev.1 primarily generated a fixed HO response.
  - Rev.4 uses torque-dependent tiered Hands-On levels while preserving stock HO when an override is unnecessary.

- **Visual Warning Rescue added**
  - A steering visual warning can trigger an interaction immediately instead of waiting for the normal event cycle.

- **Independent carrier randomness**
  - Carrier variation is generated separately from the main interaction engine, so it does not alter event timing or direction behavior.

**In short:** Rev.1 generated isolated steering interaction events, while Rev.4 adds continuous stock-relative interaction between those events and reacts to actual steering warnings.
