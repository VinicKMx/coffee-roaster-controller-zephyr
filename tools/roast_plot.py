#!/usr/bin/env python3
"""Plot roaster CSV telemetry emitted by the firmware console."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path

import matplotlib.pyplot as plt


def read_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def mdeg_to_c(values: list[str]) -> list[float]:
    return [int(value) / 1000.0 for value in values]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", type=Path)
    args = parser.parse_args()

    rows = read_rows(args.csv)
    if not rows:
        raise SystemExit("empty telemetry file")

    elapsed_min = [int(row["elapsed_ms"]) / 60000.0 for row in rows]
    actual_c = mdeg_to_c([row["temperature_mdeg_c"] for row in rows])
    target_c = mdeg_to_c([row["target_mdeg_c"] for row in rows])
    ror_c_per_min = mdeg_to_c([row["ror_mdeg_c_per_min"] for row in rows])
    heater_pct = [int(row["heater_request_permille"]) / 10.0 for row in rows]

    fig, temp_axis = plt.subplots()
    power_axis = temp_axis.twinx()

    temp_axis.plot(elapsed_min, actual_c, label="actual C")
    temp_axis.plot(elapsed_min, target_c, label="target C")
    temp_axis.plot(elapsed_min, ror_c_per_min, label="RoR C/min")
    power_axis.plot(elapsed_min, heater_pct, color="tab:red", alpha=0.5, label="heater %")

    temp_axis.set_xlabel("elapsed min")
    temp_axis.set_ylabel("temperature / RoR")
    power_axis.set_ylabel("heater %")
    temp_axis.grid(True)

    lines, labels = temp_axis.get_legend_handles_labels()
    power_lines, power_labels = power_axis.get_legend_handles_labels()
    temp_axis.legend(lines + power_lines, labels + power_labels, loc="best")

    fig.tight_layout()
    plt.show()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
