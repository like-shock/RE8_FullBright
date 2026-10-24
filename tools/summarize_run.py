#!/usr/bin/env python3
"""Deterministic summary of bounded diagnostic JSONL; never infer visual success."""
import argparse
from collections import Counter
import json
from pathlib import Path


def summarize(paths):
    statuses, reasons = Counter(), Counter()
    events = malformed = max_dropped = 0
    for path in paths:
        if path.stat().st_size > 4 * 1024 * 1024:
            raise ValueError(f"log exceeds 4 MiB: {path}")
        with path.open(encoding="utf-8") as stream:
            for line in stream:
                try:
                    event = json.loads(line)
                    status, reason = event["status"], event["reason"]
                    dropped = event["dropped"]
                    if not isinstance(status, str) or not isinstance(reason, str) or type(dropped) is not int or dropped < 0:
                        raise ValueError("invalid event fields")
                    statuses[status] += 1
                    reasons[reason] += 1
                    max_dropped = max(max_dropped, dropped)
                    events += 1
                except (ValueError, KeyError, TypeError):
                    malformed += 1
    return dict(events=events, malformed=malformed, max_dropped=max_dropped,
                statuses=dict(sorted(statuses.items())), reasons=dict(sorted(reasons.items())),
                visual_result="not_evaluated")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("logs", type=Path, nargs="+")
    args = parser.parse_args()
    try:
        print(json.dumps(summarize(args.logs), sort_keys=True, ensure_ascii=True))
    except (OSError, ValueError) as error:
        parser.exit(2, f"{error}\n")
