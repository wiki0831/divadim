# Supporting a new `DivaMegaMix.exe` version

DivaDim calls into the game at fixed addresses. A game update moves those functions, and a wrong address crashes the game (illegal instruction, or a bad hook). This note explains how to find the new addresses, and a prompt you can give an AI assistant to do it.

## What the mod needs

| Name | Role |
|---|---|
| `TaskPvGameInit` | Runs when a PV starts. DivaDim hooks it to apply `default_dim` / `pv_settings`. |
| `TaskPvGameDisp` | Function run every frame while a PV is displayed. DivaDim hooks it. |
| `DrawRectangle` | Draws the black rectangle. Called with `(Rect*, res, layer, color, prio)`. |
| `CurrentPvID` | `int32` at `0x14CBFFE2C`. Identical in every known version, but confirm it for a new one. |

The first three are listed per game version in the `Versions` table in `src/mod.cpp`. At startup the mod checks that the bytes at `drawRectangle` match `DrawRectangleSignature`. If no version matches, the mod does nothing instead of crashing.

Known versions:

| Version | `TaskPvGameInit` | `TaskPvGameDisp` | `DrawRectangle` |
|---|---|---|---|
| Current Steam | `0x1405DA040` | `0x1405DA090` | `0x1405B4D40` |
| Early 2023 (Steam Deck) | `0x1405DA000` | `0x1405DA050` | `0x1405B4D00` |

`TaskPvGameInit` and `TaskPvGameDisp` are `E9` jump thunks in the file. In both versions they sit `0x40` below the corresponding thunk in the newer build, so a new version may shift them by a different amount. Confirm by following the thunk (`target = thunk + 5 + int32 at thunk+1`) in both exes and comparing the target function bytes. The targets should have the same instructions.

## How to find the addresses in a new exe

Image base is `0x140000000`. File offsets come from the PE section table.

1. **Find `DrawRectangle`.** Search the new exe for its prologue bytes, which have been identical in every version so far:
   `48 89 5C 24 08 44 89 4C 24 20 55 56 57 48 81 EC A0 01 00 00`
   Convert the file offset of the match to a virtual address using the section table. The result must be unique.
2. **Find `TaskPvGameInit` and `TaskPvGameDisp`.** Both are jump thunks. Use the previous version's thunk addresses plus the shift seen on `DrawRectangle` as a first guess, then confirm the thunk target function matches the known version's (same instructions, only relative displacements differ). Search for the target's first bytes if the guess fails. Pointing at the middle of a function crashes the game, which is exactly what happened with `0x1405DA040` on the 2023 build.
3. **Check against the running game, not just the file.** The exe is Denuvo-protected. Some addresses hold jump thunks (`E9 ...`) in the file but real code at runtime, or the other way round. The most reliable check is the bytes read from memory in the running game, so log them (`VirtualQuery` plus a read) from a debug build of the mod and compare them with what you expect.
4. Add a row to `Versions` and keep the table order newest first.

## Pitfalls we hit

- A file that is "the same game version" can still differ between devices. The Steam Deck had an older build from early 2023 while the PC had the current one, so always compare the exe hash, not the version text.
- Never read a hardcoded address without checking it first. The mod uses `Matches()` with `__try/__except` for this.
- The mod's DLL must be built **Release|x64**. A Debug build links the debug C++ runtime, which Proton/Wine does not have, and the loader reports "DLL not found" even though the file is there.
- DIVA Mod Loader sets the working directory to the mod folder before calling `Init()`, so `config.toml` is found by its relative name.

## Prompt for an AI assistant

Copy this, fill in the paths, and attach or point to both exe files:

```text
I maintain DivaDim, a DIVA Mod Loader DLL mod for Hatsune Miku: Project DIVA Mega Mix+
(repo root: <path>). It hooks the PV-start function TaskPvGameInit and the per-frame
function TaskPvGameDisp, and calls DrawRectangle, at hardcoded addresses listed in the
Versions table in src/mod.cpp. It also reads the current PV ID from 0x14CBFFE2C.

I have a new game executable at <path to new DivaMegaMix.exe> that the mod does not
support yet. A known-good executable is at <path to a supported DivaMegaMix.exe>, with
known addresses in the Versions table.

Please:
1. Parse the PE headers of both files (image base 0x140000000) and write a small tool to
   convert between virtual addresses and file offsets.
2. In the known-good exe, read the bytes at the known DrawRectangle address and use the
   first 20 bytes as a signature. Search the new exe for it and report every match.
3. TaskPvGameInit and TaskPvGameDisp are E9 jump thunks. In both exes, follow each thunk
   (target = thunk + 5 + int32 at thunk+1) and compare the target function bytes. They
   should contain the same instructions, with only relative displacements different.
   Look for the new thunk addresses by searching for those target bytes. If the
   targets are in obfuscated regions, also compare the code around each thunk.
4. Check that the PV ID address 0x14CBFFE2C is still the right one: find code that
   references it (rip-relative lea / mov) in both exes and compare.
5. The exe is Denuvo protected: file bytes and in-memory bytes can differ. If I send you a
   log of memory bytes from a debug build of the mod, trust it over the file.
6. Add a row to the Versions table in src/mod.cpp and update the tables in
   DECODING_NEW_EXE.md. Do not change anything else.
7. Build Release|x64 and confirm the DLL imports only KERNEL32.dll and USER32.dll.

Do not guess addresses. If a result is ambiguous, tell me what extra data you need.
```
