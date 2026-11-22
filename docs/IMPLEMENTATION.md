# Implementation and validation

The item is identified from the **running game's data table**, rather than by a copied session address:

- row: `ETC_Item_VendingMachineCoin`
- `ID`: 10
- `ScanDisplayGroup`: `DitCoin`
- reflected `MaxAmount` field, checked before planning a request

FName indices are session-specific and are rediscovered. The Christmas row aliases the ordinary coin and is not used.

For build 1.5.0, the native add-request function must match a unique byte signature, a verified prefix, and RVA `0x1afa030`. The presentation getter and its RIP-relative singleton must match RVAs `0x1a98b50` and `0x70373d0`. Inventory access validates the actor record, owner/bucket maps, sparse allocation flags, item GUID key/value equality, names, and quantity ranges.

The native Windows x64 argument values follow the validated call:

```text
(null, 1, currentActorID, &liveItemFName, amount, 1,
 &emptyDelegate, &emptyDelegate, false, false, false)
```

Each empty delegate is 16 zero bytes; FName is 8 bytes. The semantic enum names of arguments 2 and 6 are not asserted. `amount` is a positive integer derived from the current quantity, not a fixed 998. No game field is written directly.

The earlier local experiment used a current-count-of-1 guard and an exact gold value to validate the offsets. Those personal test conditions are removed here. The gold alias `BetaCrystal` is read from live inventory, and gold must remain equal to its request-time value during result verification. The HP tolerance matches the inspected implementation: positive maximum ≤10,000,000, HP between 0 and `2*maximum+1`.

Request IDs are restricted to path-safe characters. A `CREATE_NEW` receipt file is flushed before a native call, so IDs are reserved persistently even if the game exits immediately afterwards. Duplicate IDs never repeat. Request files do not retry on timeout. A fresh ID is an explicit new request, so users must inspect uncertain/partial outcomes before choosing one.

## Evidence boundaries

- In-game tests of the underlying engine path succeeded for 0→1 and 1→999; gold was unchanged. The initial test's display was manually confirmed.
- The public parser/planner was tested on the host with address/undefined-behavior sanitizers, including starts other than 1.
- The public DLL was cross-compiled for x86_64 Windows and its import table checked for CRT dependencies.
- This refactor has not been live-tested against every starting value, another profile, another game version, or the configured upper limit.
- Changing requests to adjust SP or another item is not supported by this plugin.
