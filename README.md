# Stellar Blade CPS — Headless Vitcoin

Raise Vitcoin to a requested **total** without opening an in-game menu. Designed for cases where CPS loads but its graphics overlay fails under CrossOver/D3DMetal.

The starting count does **not** have to be 1. The plugin reads the current inventory and calculates `amount = target - current`:

| Current | Requested total | Added |
| ---: | ---: | ---: |
| 0 | 999 | 999 |
| 1 | 999 | 998 |
| 80 | 999 | 919 |
| 500 | 999 | 499 |
| 999 or more | 999 | 0 |

Gold does not need to match a fixed balance. The plugin records it before a request and verifies that it remains unchanged afterwards.

## Compatibility

- Stellar Blade **1.5.0**; native signatures and RVAs are deliberately checked against this build.
- CPS **1.2.1** tested; C API 2 / CPS 1.2.0 minimum.
- Native request path tested with ordinary CrossOver on macOS: **0 → 1** and **1 → 999**, with gold unchanged.
- The new parser and arbitrary-start planning are covered by host tests. The refactored public DLL has been cross-compiled, but has not been retested across other saves or operating systems.
- The live item table sets `MaxAmount = 999999`. Amounts larger than the tested +998 request have not been tested in-game.

Uses CPS game-thread callbacks, not an overlay or custom graphics hook. The Windows x64 DLL imports only `KERNEL32.dll`, without a CRT or C++ constructors. Cheat Menu is not required at runtime.

## Install and request a total

1. Back up your save. Install [CPS by n0ted](https://www.nexusmods.com/stellarblade/mods/4054) following its instructions. CrossOver may require the `winmm` override **Native then Builtin**.
2. Download `headless_vitcoin.cps` from this repository's Releases, or build it below. Copy it into `SB/Binaries/Win64/CPS/Plugins/`.
3. Start the game and enter gameplay. CPS creates `SB/Binaries/Win64/CPS/Configs/HeadlessVitcoin/`. Without a request file, the plugin only reads data.
4. While idle at a camp, create **UTF-8** `request.ini` in that config directory:

   ```ini
   id=first-999
   target=999
   ```

   `target` means the desired total, not an amount to add. `id` must be a new identifier of 1–32 letters, digits, `_`, or `-`.

   Alternatively, on a host with Python:

   ```sh
   python3 make_request.py '/path/to/CPS/Configs/HeadlessVitcoin' 999
   ```

5. Check `status.txt`. `VERIFIED` means the exact requested total was observed and gold was unchanged. Rest at a camp and wait for autosave to finish to persist the result.

Each ID gets a persistent `attempt-<id>.txt` **before** the one native request. That ID cannot execute again, even after a restart. A result that cannot be verified is **not retried**. Inspect the actual inventory before deliberately creating another ID; keep old receipt files.

For a later, separate top-up after spending coins, explicitly create a new request ID. The plugin does not automatically refill currency after spending. If already at or above a target, it consumes that request ID without changing inventory. It never removes coins to lower a balance.

Disable by moving `headless_vitcoin.cps` out of `Plugins` and restarting the game. Config and receipt files can remain.

## Build

Requires Python 3 and official [Zig 0.15.2](https://ziglang.org/download/). Builds a Windows DLL from macOS, Linux, or Windows:

```sh
python3 build.py --zig /path/to/zig
```

Outputs: `dist/headless_vitcoin.cps`, the DLL, and `dist/SHA256SUMS`. Packaging uses Python's standard ZIP library; `cps_pack.exe` is not needed. The CPS framework is downloaded separately and is not bundled.

Host tests:

```sh
cc -std=c11 -Wall -Wextra -Werror tests/test_request.c -o /tmp/test_request
/tmp/test_request
```

Tests cover parsing, duplicate fields, malformed/overflowing targets, path-like request IDs, zero/current/above-target cases, and the reflected maximum. See [implementation notes](docs/IMPLEMENTATION.md).

## Credits and license

Plugin code: MIT. The CPS C API header and SDK-derived UE layouts retain **Copyright (c) 2026 n0ted** under MIT; see [LICENSE.CPS-SDK](LICENSE.CPS-SDK). Native call arguments were checked against Cheat Menu 1.1.0 and the local 1.5.0 executable. CPS/SDK: [n0ted](https://www.nexusmods.com/stellarblade/mods/4054). Cheat Menu: [n0ted](https://www.nexusmods.com/stellarblade/mods/4055).

No CPS framework DLL, Cheat Menu binary, game executable, save file, account identifier, or personal gameplay log is included.
