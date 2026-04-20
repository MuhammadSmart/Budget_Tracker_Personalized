# Budget Tracker (personal, console)

A C++17 terminal budget assistant. Learns your spending categories and items
on first run, shows an ASCII dashboard of how much budget is left per
category, tracks burn rate across cycles, and tells you in plain language
when you're spending faster than usual or about to run out.

## Build

```sh
cmake -B build -S .
cmake --build build
./build/budget
```

Data lives in `./data/` (plain text — `profile.cfg`, `ledger.log`, `history.tsv`).

## Talking to it

Free-form, no menus:

```
bought eggs 45
spent 32 on uber
paid the electricity bill 410
rice 90
show
show groceries
set groceries 900
set currency USD
add category health 400
add item coffee to groceries
history
options
help
quit
```

## What it tells you

- One bar per category: `[#######.......] 470 / 800  59%  !!8d left`
- Projected end-of-cycle spend per category (real burn-rate math)
- "X is running 34% hotter than your usual day-12 spend" once you have
  ≥2 closed cycles (last 3 weighted heaviest)
- Rebalance plan when ≥2 categories are projected over total:
  computed trim % per category to land back on budget
