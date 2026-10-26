# svalwatch

Watches a connected Svalboard and prints its current layer and pressed
switches as JSON lines, one line per change. It is the keyboard side of a
layer overlay: whatever draws the overlay reads svalwatch's stdout and
never talks to the board itself.

```sh
make                                # swift build -c release
make test                           # the SvalWatchCore tests
.build/release/svalwatch            # Ctrl-C (or close stdin) to stop
```

```json
{"type":"connected","serial":"sval:E464B4B2934E2C2A","protocol":3}
{"type":"keys","pressed":[]}
{"type":"layer","top":0,"active":1,"default":1,"mods":0,"caps_word":false,"caps_lock":false}
{"type":"keys","pressed":[[1,2]]}
{"type":"disconnected"}
```

`pressed` holds switch-matrix `[row, col]` pairs, as in the `matrix`
fields of Svalboard's `info.json` layout. `top` is the layer whose legends
are showing. `svalwatch --help` lists the options.

## How it gets the state

With this userspace's firmware, the board announces changes itself
(`users/townk/townk_announce.c`). svalwatch subscribes over Raw HID, and
while the subscription lasts the board prints one line to its QMK debug
console per change. svalwatch reads those lines and only renews the
subscription, every 2 s. The subscription is a 5 s lease, so when
svalwatch stops (quit, crash, sleep, screen lock), the board goes quiet
on its own. The console is a separate interface from the one Keybard
uses, so Keybard never sees the lines.

With any other firmware, the subscribe reply lacks townk_announce.c's
signature, and svalwatch polls instead, through the same read-only Raw
HID queries Keybard Host uses: VIA's switch-matrix read and Sval's
layer-state read, both wrapped in a svalboard/core client ID (which keeps
its replies apart from Keybard's when both are open). It polls fast
(60/s) while switches or the layer are changing and for 2 s after, then
4/s; the layer is read only after a switch changes or at least 15 times a
second. While idle it also listens to the board's ordinary key/mouse
reports, which the Mac receives anyway, and goes fast again on the first
one (that needs the Input Monitoring permission for whatever launched it;
without it the 4/s poll still notices, just later). `--poll` forces this
mode. Either way, svalwatch stops talking to the board while the screen is
locked, the displays sleep, or the Mac sleeps.

## Cost of polling (measured 2026-10-25, M-series Mac, rev A board)

One exchange with the board costs about 240 µs of CPU, ~85% of it in the
kernel's HID stack, whether the write is synchronous or not. That cost,
not svalwatch's own code, sets the price of polling:

| State | Polls/s | CPU (one core) | Memory |
|---|---|---|---|
| Idle (default) | ~3–6 | 0.1–0.3% | ~6 MB |
| Active (held at the fast rate) | ~48 | 1.3–2.1% | ~6 MB |

CPU readings vary up to 3× between runs: on Apple silicon the same work
reads as more "% of a core" on a slower core or clock. Run with
`--stats 10` to see the numbers on your machine.

## Layout

- `Sources/SvalWatchCore`: everything decidable without a board (packet
  layout, reply parsing, pacing, the session state machine, the output
  format). `swift test` covers it.
- `Sources/svalwatch`: the IOKit side (device matching, reports, timers,
  passive input, sleep/lock).
