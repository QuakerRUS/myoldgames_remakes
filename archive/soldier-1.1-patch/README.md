# Soldier 1.1 patch

- `patch/` is the contents of `patch1_1.zip`.
- `src/` is the contents of `source1_1.zip`: the 1.1 BASIC sources (CP866), with UTF-8 copies
  in `src/src-utf8/`.

## Installing

Copy everything from `patch/` over a Soldier 1.0 folder (`../soldier-1.0/release/`),
replacing the files.

## What 1.1 changes

Verified by comparing the files with 1.0:

1. **Time limits removed.** In every `LEVELn.DAT` of the patch the header's time field is 0,
   so the timer shows "-". Levels 1, 24, 33 and 34 are not in the patch and keep their
   limits of 300, 120, 36 and 15.
2. **`GAME1.BAS`:** a time value of 0 means "no limit" (`time = -1`, the timer subroutine
   returns early).
3. **`MENU.BAS`:** entering level 52 shows "LEVEL NOT FOUND. YOU WIN!".
4. **`SOLDIER.BAS`:** the version label reads "Version 1.1".
5. **`SOLDIER.CFG`:** default speeds `15000, 2500` (1.0 shipped `15000, 25`).
