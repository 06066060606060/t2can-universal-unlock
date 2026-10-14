"""Inspect semicolon CAN logs without interpreting vehicle signals.

Timestamp units are explicit. Corrupt rows are reported, never reconstructed.
Optional timestamp bounds exclude damaged timestamps after inspecting the log.
"""
import argparse
import collections
import csv
import hashlib
import json
import re
from pathlib import Path


def analyze(path, ticks_per_second, start=None, end=None):
    frames, rejected = [], []
    reasons = collections.Counter()
    lines = 0
    with path.open(encoding="utf-8-sig", newline="") as source:
        for lines, row in enumerate(csv.reader(source, delimiter=";"), 1):
            reason = None
            try:
                if len(row) != 6:
                    raise ValueError("column_count")
                if not row[0].isdigit() or not re.fullmatch(r"0x[0-9a-fA-F]+", row[1]):
                    raise ValueError("timestamp_or_identifier")
                timestamp, identifier, dlc = int(row[0]), int(row[1], 16), int(row[4])
                if row[2] not in ("0", "1") or row[3] not in ("0", "1"):
                    raise ValueError("flags")
                if identifier > (0x1FFFFFFF if row[2] == "1" else 0x7FF):
                    raise ValueError("identifier_range")
                data = row[5].split()
                if not 0 <= dlc <= 8 or len(data) != dlc or any(
                        not re.fullmatch(r"[0-9a-fA-F]{2}", byte) for byte in data):
                    raise ValueError("dlc_or_payload")
                if (start is not None and timestamp < start) or (end is not None and timestamp > end):
                    raise ValueError("timestamp_outside_selected_range")
                frames.append((timestamp, identifier, dlc, lines))
            except ValueError as error:
                reason = str(error)
            if reason:
                reasons[reason] += 1
                rejected.append({"line": lines, "reason": reason, "text": ";".join(row)})
    if not frames:
        raise ValueError("No usable CAN frames")
    inversions = sum(b[0] < a[0] for a, b in zip(frames, frames[1:]))
    frames.sort(key=lambda frame: frame[0])
    timestamps = [frame[0] for frame in frames]
    duration = (timestamps[-1] - timestamps[0]) / ticks_per_second
    counts = collections.Counter(frame[1] for frame in frames)
    windows = {}
    for milliseconds in (1, 2, 5, 10, 100, 1000):
        width = milliseconds * ticks_per_second / 1000
        left, maximum, first = 0, 0, timestamps[0]
        for right, timestamp in enumerate(timestamps):
            while timestamps[left] <= timestamp - width:
                left += 1
            count = right - left + 1
            if count > maximum:
                maximum, first = count, timestamps[left]
        windows[str(milliseconds)] = {"frames": maximum, "firstTimestamp": first}
    return {
        "source": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        "timestampTicksPerSecond": ticks_per_second,
        "selectedRange": {"start": start, "end": end},
        "csvRecords": lines, "usableFrames": len(frames), "rejectedRecords": len(rejected),
        "rejectionReasons": dict(reasons), "timestampInversions": inversions,
        "durationSeconds": duration, "usableFramesPerSecond": len(frames) / duration if duration else None,
        "identifierCount": len(counts),
        "dlcCounts": dict(collections.Counter(frame[2] for frame in frames)),
        "windowPeaksMs": windows,
        "identifiers": [{"id": f"0x{identifier:03X}", "frames": count,
                         "usableFramesPerSecond": count / duration if duration else None}
                        for identifier, count in counts.most_common()],
        "rejected": rejected,
        "limitations": ["Timestamp units are supplied by the caller, not proven by this headerless CSV.",
                         "Counts exclude damaged rows and cannot measure exact CAN loss.",
                         "A peak uses timestamps as recorded, not sub-tick physical arrival timing.",
                         "One bus log does not measure three-bus scheduling or transmit completion."]
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=Path)
    parser.add_argument("--time-unit", choices=("ms", "us"), required=True)
    parser.add_argument("--timestamp-start", type=int)
    parser.add_argument("--timestamp-end", type=int)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.timestamp_start is not None and args.timestamp_end is not None and args.timestamp_start > args.timestamp_end:
        parser.error("timestamp-start must not exceed timestamp-end")
    result = analyze(args.csv, 1000 if args.time_unit == "ms" else 1000000,
                     args.timestamp_start, args.timestamp_end)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: value for key, value in result.items() if key not in ("rejected", "identifiers")}, indent=2))


if __name__ == "__main__":
    main()
