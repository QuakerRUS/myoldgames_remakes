PITON 1.0
=========

A snake game: eat 50 rabbits on each of the 10 levels without hitting a wall or yourself.
A remake and completion of Piton (QBasic, 2000).

Run piton.exe. The map editor is piton_editor.exe, the graphics editor is painter.exe
(see PAINTER.txt).

GAME CONTROLS
  Arrows / W A S D  change the direction (no 180-degree turn)
  P                 pause
  Esc               quit to the menu (asks first)
  F11 / Alt+Enter   fullscreen

MENU
  NEW GAME (start on any level you have reached), DEMO (the computer plays),
  OPTIONS (sound, music, fullscreen), EXIT.
  Up / Down: item, Enter / Space: choose, Left / Right: level, Esc: back.

MAP EDITOR (piton_editor.exe)
  Tab: field <-> palette, arrows: cursor, Space / Enter: place, Backspace: erase,
  1-6: palette (brick, empty, snake start up / right / down / left), R: rotate the start,
  H / V: symmetry painting, G: grid, Home: fit,
  mouse: left places, right erases, wheel zooms, middle button pans,
  Ctrl+Z / Ctrl+Y: undo / redo, Ctrl+N / O / S: new / open / save,
  F5: test-play the level in piton.exe, Esc: quit.

SAVES
  Level reached, high score and settings: %APPDATA%\V Software\Piton
  (piton.exe --data-dir <folder> keeps them in that folder instead).
  The level reached and the high score are signed: if piton.json is edited by hand,
  the progress is reset to level 1 and high score 0 (the settings are kept) and the
  menu says "SAVE FILE WAS MODIFIED - PROGRESS RESET".

V Software Entertainment — Program by Fedoseyenko Victor — Russia, Moscow, 2026
