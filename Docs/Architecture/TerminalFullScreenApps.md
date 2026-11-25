# Full-screen applications (Midnight Commander) in the Agentino terminal

Goal: `mc` started in the terminal of an agent works completely in the web and desktop clients, and its screen is drawn with
RichText (`Text.RichText`), as the output of the terminal is now. This document is the design; nothing of it is implemented yet.

## Current state

The terminal is `Include/agentinoqml/Qml/TerminalSessionPane.qml`; the pty is `Include/agentinodata/CTerminalSessionManagerComp.cpp`
(ConPTY on Windows, `openpty` with `TERM=xterm` elsewhere; the output is decoded to a string, the keys are written as UTF-8).

- The pty output goes through a small VT emulator written in the QML file: a grid of cells `{ch, color, bold}`, a cursor, scroll
  regions, insert and delete of lines and characters, erase, deferred wrap, `CSI ?1049h/?47h` for the alternate screen. The primary
  screen runs through the same grid (ConPTY repaints it with cursor moves); the rows that scroll off are committed to the scrollback.
- A grid row is a `Text` with `textFormat: Text.RichText` (`gridRowHtml`), one span per run of equal color/weight, the cursor is a
  span with a background. The rows are `Repeater` delegates at `y = index * charHeight`.
- The keys go through `vtInputForKey`: Ctrl+A..Z, arrows, Home/End, PgUp/PgDn, Ins/Del, F1..F10, Alt+key as ESC+key.
- The size of the pty is `floor(width / charWidth)` x `floor(height / charHeight)`, `charWidth` is measured from a probe text.

## What `mc` needs and the terminal does not have

| Area | Gap (place) | Why `mc` needs it |
|---|---|---|
| Cell attributes | Cell has no background, no reverse, no underline/italic/dim; SGR knows 0, 1, 22, 30-37, 39, 90-97 only (`applySgrToState`) | `mc` is blue panels, cyan menu bar, reverse video for the current file, and its skins use 256 colors/truecolor |
| Font | The output font is `Style.fontFamily`, proportional (see the comment at `sizeMetricsProbe`) | A frame of `mc` is aligned by columns: the cells of different rows must be the same width |
| Escape sequences | `ESC ( 0`, `ESC ) B` and the like are skipped as 2 bytes, so a third byte is printed; `CSI X` (erase characters), `CSI b` (repeat), `CSI s/u`, `CSI E/F`, `CSI Z`, tab stops, `IRM`, origin mode, autowrap mode are ignored | ConPTY repaints with `ECH` and `REP`; curses programs use the rest |
| Modes | Only `?1049/?47` and `?25` are read | Application cursor keys `?1`, bracketed paste `?2004`, mouse `?1000/?1002/?1003/?1006`, focus `?1004` |
| Replies | The terminal never answers a device query | `CSI c`, `CSI 5n`, `CSI 6n` (cursor position) are sent by ncurses/slang and ConPTY |
| Characters | One cell per UTF-16 unit | Wide characters (2 cells), combining marks (0 cells), characters outside the BMP |
| Keys | No modifiers on arrows/F-keys (`CSI 1;5A`), no F11/F12, no Ctrl+Space and Ctrl+`[ \ ] ^ _`, no application cursor mode, the browser takes some keys | `mc` uses Ctrl+arrows, Shift+F-keys, Alt+letters, Ctrl+O, F10 |
| Mouse | None | `mc` clicks the menu bar, the F-key buttons and the panels |
| Paste | Sent as typed input | Bracketed paste when the application asks for it |

## Design

### 1. The emulator becomes a module with no UI

Move the VT code out of the QML file into a JS library (`TerminalEmulator.js`, `.pragma library`, no QML types inside) that is
loaded by the pane and by the node tests. It owns the grid, the cursor, the modes and the parser; it has three entry points:

- `feed(text)`: the bytes of the pty; it returns what changed (the set of dirty rows, the replies to write back to the pty, the title).
- `resize(cols, rows)`.
- `rowHtml(index, options)`: the RichText of one row (see below).

The QML pane keeps only the view: the rows, the scrollbar, the keys and the mouse. This also takes the emulator (several hundred lines)
out of `TerminalSessionPane.qml` and makes the emulator testable without a browser.

### 2. Cell model

A cell is `{ch, width, fg, bg, attrs}`: `fg`/`bg` are `null` (default), a palette index 0..255 or a 24-bit value; `attrs` is a bit
mask (bold, dim, italic, underline, blink, reverse, strike, conceal). A wide character is a cell of width 2 followed by a cell
that is skipped. The rows are arrays of such cells; a row has a version number, so only the rows that changed are rendered again.

### 3. Parser

A VT500-style state machine (ground, escape, CSI with parameters, intermediates and `:` sub-parameters, OSC, DCS/APC/PM/SOS that are
skipped) that keeps the state between chunks (a sequence split by a chunk is not a special case any more, which replaces the
`routerPendingPartial`/`altPendingPartial` code). It handles:

- the cursor: CUU CUD CUF CUB CNL CPL CHA VPA HPA CUP HVP, DECSC/DECRC (with the attributes and the charset), `CSI s/u`, IND NEL RI, tab stops (HTS TBC CHT CBT);
- the screen: ED EL ECH ICH DCH IL DL SU SD REP, DECSTBM, DECALN, RIS and DECSTR, the alternate screen (`?47 ?1047 ?1048 ?1049`);
- SGR completely: 1-9, 21-29, 30-37, 38/48 with `5;n` and `2;r;g;b` (and the `:` form), 39/49, 90-97, 100-107;
- charsets: `ESC ( ) * +` with `0` (line drawing), `B`, `SO`/`SI`;
- modes: DECCKM, DECOM, DECAWM, IRM, LNM, `?25`, `?2004`, `?1000/1002/1003/1006`, `?1004`; the unknown ones are remembered and ignored;
- replies: DA1/DA2 (`CSI c`), DSR 5 and 6, `CSI 18t`/`CSI 14t`; the replies are returned by `feed` and sent with `sendRawInput`;
- characters: iteration by code points, a width table (East Asian wide = 2, combining = 0), the replacement for what has no glyph.

### 4. Drawing with RichText

Each row is still a `Text` with `Text.RichText`. A run of cells with the same attributes is one span:

```
<span style="white-space:pre"><span style="color:#cccccc;background-color:#0000aa;font-weight:bold;text-decoration:underline">...</span>...</span>
```

- Reverse is resolved by the emulator (the colors are swapped), dim by mixing the color with the background, bold on the colors 0..7 as the bright one (a setting), the default colors come from the theme; the palette 16..255 is the xterm table.
- The cursor is a reverse cell (the engine now paints the background of a space and keeps one space at the end of a RichText block, which is what a cursor at the end of the line needs), a blinking cursor is a timer that bumps the version of one row.
- The background of a row must be seamless: the row `Text` has the height of one line and the rows are at integer steps (`charHeight` rounded up); if the engine leaves a gap the row gets a `Rectangle` behind it.
- One row is re-rendered only when its version changed. Measured in the engine (layout only, no glyph rasterization, a row of 80 cells with 11 spans): about 1 ms for 1 changed row and 2.7 ms for all 24 rows of the frame, so a full screen of `mc` per frame is not a problem.

### 5. A monospace grid

The grid uses its own font family: the generic `monospace` (the engine rasterizes it with the font of the browser; `Style` gets a
`terminalFontFamily`). `charWidth` is measured on a monospace probe (`"M".repeat(20)`), the size of the pty is `floor(width / charWidth)`.
A glyph whose width is not the width of a cell (a fallback font) would shift the rest of the row: such a character is cut out of
the row text into its own short `Text` placed at `x = col * charWidth` (done only when the measurement shows a mismatch; the
common case stays one `Text` per row).

### 6. Keys, paste, mouse

- A key table in the module: the xterm encoding with the modifier parameter (`CSI 1;<m>A`, `CSI 15;<m>~`), F1..F12, application cursor keys (`ESC O A`) when `?1` is on, Alt+key as `ESC key`, Ctrl+Space/`[ \ ] ^ _`, the keypad. Tested as a table.
- The keys the browser takes (F1, F3, F5, F10, Ctrl+O...) get `preventDefault` through the engine when the grid has the focus; the ones that cannot be taken (Ctrl+W, Ctrl+T, F11 in a browser) stay with the browser.
- Paste: `ESC[200~ ... ESC[201~` when `?2004` is on.
- Mouse: when `?1000/1002/1003` is on, press, release, drag and wheel are sent in the `?1006` form (`CSI < b;x;y M/m`) and the selection is off; Shift gives the selection back. Without a mouse mode the mouse selects text of the grid and Ctrl+C/Insert copy it.

### 7. Agent side

- `TERM=xterm-256color` and `COLORTERM=truecolor` for the shells of the pty that is not ConPTY (`CTerminalSessionManagerComp.cpp`, now `xterm`); a locale with UTF-8 if the environment has none.
- ConPTY: no change expected; the replies of section 3 answer the queries of ConPTY and of the program, if it sends them (to be confirmed by the recording).
- The size is already sent on resize (`ResizePseudoConsole`, `TIOCSWINSZ`); it must also be sent when the grid font changes.

## How to test

1. A recorder of the raw pty stream of a real `mc` (`Tests/TerminalEmulator/tools/record.mjs`: ConPTY on Windows with `mc.exe`; a pty on Linux is to be added) with a script of keys, to store the streams as fixtures (done for the first one).
2. A reference: the same streams fed to an independent emulator (`@xterm/headless`, `tools/reference.mjs`, not part of the build) give the expected screens (text and attributes of every cell), stored next to the fixtures (done for the first one). The tests of the repository compare the screens of our emulator with them: no network and no extra package when the tests run.
3. Unit tests of the parser by sequence (every item of section 3), of the key table and of the width table.
4. An engine test: rows of a recorded `mc` screen laid out and painted, the quads of the backgrounds and the glyph positions checked (no gap between rows, the columns line up).
5. A check by eye in the browser against desktop: `mc` at 80x24 and after a resize, the menu (F9), the panels, the viewer (F3), the editor (F4), the dialogs, the mouse.

## Stages

1. **Readable `mc`**: the module with the cell attributes (background, reverse, underline, 256 and truecolor), charsets, ECH/REP/CSI s,u, the replies, the monospace font. The existing behaviour is kept (tests for it first).
2. **Usable `mc`**: the complete parser, wide characters, the modes, the key table with modifiers and F11/F12, application cursor keys.
3. **Complete**: mouse, bracketed paste, focus, selection and copy from the grid, the cursor blink, the agent environment.
4. **Polish**: per-row versions and the measurement of performance with glyphs, the placed `Text` for the cells that do not fit the grid.

## What `mc.exe` really writes (recorded)

Recorded with `Tests/TerminalEmulator/tools/record.mjs` (ConPTY, 80x24, `mc.exe` 4.8.31, a script of keys: arrows, F9 menu, Ctrl+Up, F10),
8.5 KB, in `Tests/TerminalEmulator/fixtures/mc-basic.json`:

- **No alternate screen**: there is no `?1049`/`?47`. `mc.exe` draws in the primary buffer, so the whole `mc` runs through the primary
  grid of the terminal, where the rows that scroll off are committed to the scrollback. A full-screen program must never fill the scrollback.
- The screen is drawn with `ESC[2J`, `ESC[H`, `ESC[row;colH` and, most of all, `CSI n X` (erase characters, 55 times) followed by
  `CSI n C` (cursor forward), `CSI K`, `
`, BEL. **`CSI X` is not handled by the terminal now.**
- SGR: 16 colors only. Foreground 30, 39, 93, 97; background 44 (blue), 46 (cyan), 47 (white), 49. No reverse, no 256 colors, no
  truecolor, no mouse mode in this recording. The selected line, the menu bar and the key bar are colors of the background.
- `?25h` is repeated, `OSC 0` sets the title, the text has Cyrillic (the locale of the machine) and the box drawing characters
  `┌ ┐ └ ┘ ─ │ ├ ┤` as Unicode (no `ESC ( 0`).
- The first lines of the recording: `Running under a virtual console, enabling 256/true-color support` - `mc` itself says that it will use 256 colors and
  truecolor when the terminal allows it, so the skins that use them can show up.

The expected screens of this recording (text and attributes of every cell after every chunk) are in
`Tests/TerminalEmulator/fixtures/mc-basic.expected.json`, made by `@xterm/headless` (`tools/reference.mjs`). The screen of `mc` as the reference
shows it: a cyan menu bar, blue panels with white and yellow text, a status line, and a bar of F-keys with black text on white.

Both tools are dev-only (`Tests/TerminalEmulator/tools/package.json`: `node-pty`, `@xterm/headless`; run `npm install` there). The tests of the
emulator read the fixtures only, so they need no package.

This gives the order of the stage 1: **background colors and bright foreground (SGR 40-47, 100-107, 90-97), `CSI X`, `CSI b`, the primary
screen that is not scrolled into the scrollback by a full-screen program, a monospace font** - after that the recorded `mc` is drawn as it is.

## Stage 1: done

- `Include/agentinoqml/Qml/TerminalEmulator.js` (a `.pragma library`, no UI; it is in `agentinoqml.qrc`): the cell model (`fg`, `bg`, attribute mask), the
  parser as a state machine that goes on across chunks, SGR completely (16, 256, truecolor, the `:` form, bold, dim, italic, underline, blink, inverse,
  conceal, strike), erase with the background that is set, `CSI X`, `CSI b`, `CSI s/u`, CNL/CPL, tab stops, the sets of characters (`ESC ( 0`, SO/SI), the
  modes (autowrap, origin, insert, cursor, alternate screen are used; application keys, bracketed paste, mouse and focus are kept for the next stages), the
  replies (DA1/DA2, DSR 5/6, `CSI 18 t`), wide and combining characters, surrogate pairs, the title. `rowHtml` makes the RichText of a row.
- `TerminalSessionPane.qml` uses it: the emulator code (about 630 lines) is gone from the pane, the primary screen and the alternate screen are the same
  grid, a row is `<span style="white-space:pre">` with the spans of `rowHtml`, the terminal font is monospace (`Consolas` on Windows desktop, the generic
  `monospace` elsewhere and in the web build), and the replies of the terminal are written to the pty.
- The default foreground on a dark background of the program is light gray: `mc` sets a blue background and "default" for the text, which the dark
  text color of the light theme would hide (the frames and the files that are not selected).
- `Tests/TerminalEmulator/emulator.test.mjs` (`node --test`): the recorded `mc.exe` gives the screens of `@xterm/headless` after every chunk (text, colors,
  attributes, cursor), the same stream character by character gives the same screen, and 22 tests of single sequences and of `rowHtml`.
- Checked in the web build (the bundle compiled with the JQML 4 compiler from this worktree, in the page of a running server): `mc.exe` is drawn
  with the colors of its skin, the frames and the columns line up, the selection moves with the arrow keys, the grid follows a change of the size of
  the page (120x38, then 50x12) and `mc` redraws itself, and after F10 the prompt comes back.

Not done in this stage (the next ones): the key table with modifiers, F11/F12 and application cursor keys; the mouse and bracketed paste; the
selection and copy of the grid; `TERM=xterm-256color` on the agent; a full-screen program does not get the whole pane yet (its screen is the last
rows of the scrollback view).

Found while checking: `mc.exe` run by the agent of this machine (the service account) fails at the first start with `No such file or directory`
for `C:/windows/TEMP/mc-<user>` and prints its messages with a wrong code page; it starts when `TEMP` and `TMP` are set to a folder that exists
(`set TEMP=C:\Users\Public& set TMP=C:\Users\Public`). That is the environment of the agent, not the terminal. F9 shows no pull-down menu, with the
reference emulator too: it is what `mc.exe` writes through ConPTY.

## Stage 2: done

- **The key table** is `encodeKey` in `TerminalEmulator.js`. It takes the Qt key code (the same numbers in desktop Qt and in the JQML engine), the text and
  the modifiers, so the pane needs no mapping of its own (`vtInputForKey` is one call). It is the encoding of xterm:
  - cursor keys: `ESC [ A`, `ESC O A` while the program asks for the application mode (`CSI ?1h`), and `ESC [ 1 ; m A` with a modifier (also in the
    application mode); Home and End the same;
  - F1..F4: `ESC O P..S`, `ESC [ 1 ; m P..S` with a modifier; F5..F20: `ESC [ n ~` (15, 17, 18, 19, 20, 21, 23, 24, 25, 26, 28, 29, 31, 32, 33, 34),
    `ESC [ n ; m ~` with a modifier, so F11 and F12 now go to the program; Insert, Delete, PageUp, PageDown the same;
  - the modifier is `m = 1 + shift + 2 alt + 4 ctrl`;
  - Shift+Tab is `ESC [ Z`, Backspace `DEL` (Ctrl+Backspace `BS`), Alt in front of a key is `ESC`;
  - Ctrl with the letters, Space, `@ [ \ ] ^ _ ? /` and the digits 2..8 gives the control characters, so Ctrl+O, Ctrl+\ and the like reach the program.
- The browser: the engine calls `preventDefault` when the QML key handler accepts the event, so F1, F3, F5, F10 and the like stay with the terminal. The
  keys that a page cannot take from a browser (Ctrl+W, Ctrl+T, Ctrl+N, F11 for the full screen) stay with it.
- A second recording, `Tests/TerminalEmulator/fixtures/mc-dialogs.json`, with the screens of the reference: the viewer (F3), the editor `mcedit` with typing,
  the "Save?" dialog, a search dialog, the help, the pull-down menus, Ctrl+arrows, Tab between the panels. 64 chunks, and the emulator gives the screens of
  `@xterm/headless` after every one of them (text, colors, attributes, cursor). The test goes over every recording of `fixtures/`; a scenario is a
  script of keys in `tools/record.mjs` (`node record.mjs dialogs mc-dialogs.json`), which works in a folder with files of its own.
- 34 tests in `emulator.test.mjs` (6 for the keys, 3 for every recording), `node --test Tests/TerminalEmulator/emulator.test.mjs`.
- Checked in the web build with real `KeyboardEvent`s (with the modifiers): the keys are accepted (`defaultPrevented`), the cursor keys, Esc, F1, F3 and
  F10 reach `mc` and do what they do (F3 opens the file, so `mc` shows its dialog, F10 leaves).

Not done in this stage: the keypad in its application mode (`ESC =`), which `mc` does not need; the mouse, bracketed paste and focus events (the modes are
kept and not used yet); the whole pane for a full-screen program.

Found while checking: the agent of this machine runs `mc.exe` as the service account, which makes it fragile: it needs `USERNAME`, `USER` and `TMPDIR` set
(`set USERNAME=mcuser& set USER=mcuser& set TMPDIR=C:\Users\Public`), it cannot make its lock file `.#file` in a folder of another user and it stopped on its own
`assert` in `w32_slang.c` (`c < vio.cols`, the screen buffer of its Windows console layer) after the size of the pane had changed. These are not things of
the terminal; for the checks of the next stages `mc` is better run as a normal user.

## Stage 3: done

- **The mouse**: `encodeMouse` in `TerminalEmulator.js` makes the bytes of a press, a release, a move and the wheel for the mode that the program asked
  for (`?1000` buttons, `?1002` and the drag, `?1003` all the motion) in the form that it chose (`?1006`: `ESC [ < b ; x ; y M/m`; otherwise `ESC [ M` and
  three characters, which has a limit of 223 columns). The modifiers add 4, 8 and 16, the wheel is 64 and 65, a move adds 32.
- **The pane** has a `MouseArea` over the screen that is on only while a program owns the screen (the alternate screen) or asked for the mouse, so the
  scrollback view keeps its own selection and wheel. It turns a point into a cell: the grid view for the alternate screen, the last lines of the
  scrollback view for a program that draws the main screen (`terminalCell`). The wheel is `TerminalWheelHandler.qml` (a `WheelHandler`, which is a type
  of QtQuick 2.14, so it has a file of its own and the pane keeps `import QtQuick 2.12`).
- **What the engine does not give** to a `MouseArea`: the modifiers (always 0) and the wheel (the signal is declared and never emitted). So the pane keeps
  the modifiers of the last key event (`keyModifiers`) for the mouse, and takes the wheel from a `WheelHandler`.
- **Selection**: when the program did not ask for the mouse, or Shift is held, the pointer selects cells of the grid view along the lines (`selectionColumns`,
  highlighted by `rowHtml`), Ctrl+C with a selection (and Ctrl+Shift+C) copies its text (`selectionText`, the blanks at the end of each row dropped), without a
  selection Ctrl+C goes to the program as before. Ctrl+V and Shift+Insert paste in the grid view too.
- **Paste**: `encodePaste` makes the new lines Enter and, when the program asked for the bracketed paste (`?2004`), wraps the text in `ESC [ 200 ~ ... ESC [ 201 ~`
  (the escape characters of the text are dropped).
- **Focus**: `encodeFocus` - `ESC [ I` and `ESC [ O` when the view gets or loses the focus and the program asked for it (`?1004`).
- **The agent** (`CTerminalSessionManagerComp.cpp`, the Unix pty): `TERM=xterm-256color` and `COLORTERM=truecolor`, as the terminal draws both now. Not compiled in this work.
- 12 more tests (43 in all): the SGR and the old form of the mouse, the motion by mode, the wheel, the modifiers, the paste, the focus, the selection (also reversed, with a
  wide character, with the blanks at the end).
- Checked in the web build with a small program on the pty (the alternate screen, the mouse, the focus and the paste on) and with the events of the page:
  the focus reports `ESC[O` and `ESC[I` went out, a click at row 1, column 20 sent `ESC[<0;21;2M` and `ESC[<0;21;2m`, a drag sent the press, the motion
  `ESC[<32;x;yM` and the release, the wheel `ESC[<64;..M` and `ESC[<65;..M`; with the mouse off a drag selected the cells (highlight, no report), Ctrl+C copied
  ("Selection copied", nothing sent), a paste went out as `ESC[200~first<CR>secondESC[201~`, and with Shift held a drag selected while the mouse mode was on.

**What ConPTY does**: on this Windows 10 (19045) the pseudoconsole passes `?2004h` and `?1004h` to the terminal but not `?1049h` and not the mouse modes
(`?1000/1002/1006`): a program that asked for them through the console shows none of them in the output. So a console program on a Windows agent (mc.exe)
cannot get the mouse through ConPTY, with any terminal; on a Linux agent the pty passes everything. For the check above the modes were given to the
emulator by `feedTerminal`, as a Linux pty would give them. The cell of a click on the main screen (`terminalCell` for a program like mc.exe) is not checked with a
program: it follows from the line height of the scrollback view.

Not done: the cursor blink; the selection and copy of the scrollback view in the web build (the engine does not select in rich text); the whole pane for a
program that draws the main screen; the keypad in the application mode.

## Stage 4: done

Measured first, in the web build, with the 192 chunks of the recording `mc-dialogs` (72 KB) fed to the grid view of a running pane (the work of the pane
for a chunk: the parser, the mirrors, the rich text of every row):

| | before | after |
|---|---|---|
| a chunk (mean / median / 95th percentile / worst) | 6.96 / 6.9 / 8.2 / 10.3 ms | 1.18 / 0.5 / 4.0 / 8.0 ms |
| the rich text of the 26 rows of a screen | 6.67 ms | 0.11 ms (all kept) |

Nearly all of the 7 ms was the rich text of all the rows made again for every chunk, when a keystroke of `mc` changes two or three rows. So:

- **The rich text is kept on the row** (`cacheKey`, `cacheHtml` on the array of the cells, made by `rowHtml` for the options, the cursor, the selection and `trim` it was
  asked for). The rows are arrays that move with scroll, insert and delete of lines, so the text goes with the row. A change of the cells calls `touch(row)`: a character,
  a combining character, an erase, an insert or delete of characters, DECALN, and the wide character that does not fit at the end of a line (7 places). The rows that are
  replaced by new arrays (erase of the display, resize, the screens of the alternate buffer) have no kept text.
- **The style of a cell is made once per combination** of foreground, background and attributes in a row (`buildRowHtml`), not once per cell.
- **The test of it** (`emulator.test.mjs`): 1500 steps of random operations (text, cursor moves, SGR, erase, insert and delete of characters and lines, scroll, scroll
  regions, the alternate screen, reset, resize, combining characters) on a small screen; before each step the rows are asked for one kind of text, after it the kept
  text of every row is compared with the one made fresh. Checked by taking each of the 7 `touch` calls out in turn: the test fails for every one (the first version of the
  test did not catch one of them, because it asked the rows for several kinds in a row, and the kept text is of one kind at a time; the test asks for the same kind
  before and after now).
- **The cursor blinks** in the grid view (a timer of 530 ms while the view has the focus), and stays on for an interval after any output and any key, as in xterm.
  The scrollback view does not blink: its text is laid out as a whole, and a blink every half second would lay out the whole log again.

**Glyph widths** in the monospace font of the browser (measured with the canvas that the engine measures with, in units of the width of "M"): ASCII, the box drawing
characters, the block elements, Cyrillic, the arrows and the common symbols are exactly 1; wide characters (CJK) are 1.82 and braille is 1.37. A row with such a
character is shifted to the left by 0.18 cell for each CJK character (0.37 for each braille one) from there on. What `mc` and the usual full-screen programs
draw is in the first group, so nothing was changed for it. The remedy is to cut such characters out of the row (two blanks keep the columns and the background)
and to draw them as `Text` items placed at `x = column * width of a cell` over the grid view; it is not done, and it works for the grid view only (the main screen
is laid out as text).

**The whole pane for a program that draws the main screen** (mc.exe): not done in this stage, done in the stage 5 (a sign in the stream is found there).

## Stage 5: the whole pane for a program on the main screen, and the mouse

**The mouse on Windows: measured, and it cannot be done through ConPTY here.** On this Windows 10 (19045) three programs were run in a ConPTY and given the mouse as SGR
sequences (`ESC [ < b ; x ; y M/m`):

| program | what it asked for | what it got |
|---|---|---|
| `mc.exe` (4.8.31) | the mouse of the console | nothing: the selected row did not move, the menu did not open, the wheel did nothing |
| a PowerShell program with `ENABLE_MOUSE_INPUT` (`ReadConsoleInput`) | the mouse events of the console API | no mouse event, only the characters of the X10 form as keys; the SGR sequences were swallowed |
| a node program that asks for `?1000h ?1002h ?1006h` in its output and reads raw input | the mouse in the VT way | only the key that was typed afterwards: the SGR sequences were swallowed, and the modes it wrote were not passed to the terminal |

So conhost of this Windows eats mouse input and passes no mouse mode, whatever the program does; the client cannot give a console program of a Windows agent the mouse (a newer
conhost, Windows 11, passes it: checked afterwards on Windows 11 with `mc.exe`, the mouse works there). Nothing was changed for it: the mouse is given to a program that asks for it with the modes of the terminal (a Linux agent, WSL, a newer ConPTY),
as in the stage 3. What was done for the program that does not get the mouse:

- **The wheel is the arrow keys** for a program that owns the screen and has no mouse mode: three Up or Down for a notch (`ESC [ A`, or `ESC O A` in the application mode), as in the
  other terminals on the alternate screen. Checked: a wheel notch moved the selected row of a program by three.
- **Selecting with the pointer works on the screen of the program**, as on the alternate screen (the text of the cells is copied with Ctrl+C).

**The pane is the program's while it owns the screen** (`updateOwnership` in `TerminalEmulator.js`, `gridOwnsPane` in the pane). A program of the alternate screen owns it as far as the
terminal is told. Under ConPTY nothing is told: the program writes the main screen with cursor addressing, and nothing in the stream says it is not a shell that wrote a screenful. The rule
is about how the screen is written. A shell writes from the top down: its cursor is at the end of what it wrote (nothing under it), and the lines go up into the history when the screen
is full. A program that owns the screen writes all of its screen at places, with its status line under the cursor, and it does not scroll the main screen while it draws. So the screen is
the program's when

- no line has scrolled off the top since the cursor was last put in the top row or the screen was last cleared (a program starts its screen at the top row; **ConPTY scrolls the old
  lines up when a program clears the screen** - 15 lines in the live check, after a screenful of output of the shell - and that must not count against the program), and
- at least 60% of the rows (and 5) are written, and
- a written row is under the cursor;

it keeps the screen while a quarter of the rows is written, and the first line that scrolls off ends it. The pane gives the screen back 400 ms after the rule stops to hold (a program may
clear its screen and draw it again in the next chunk), at once when the real alternate buffer is left. While the program owns the pane the scrollback view is hidden, the cell under a
point is plain arithmetic of the grid (so the mouse, when a program does have it, is mapped exactly), and the screen is not committed to the history.

- **Tests** (`emulator.test.mjs`, 53 in all): the recordings of `mc.exe` (it owns the screen from its first full frame to its last; after it has quit the screen of the shell is not owned;
  no gap while it runs, in the viewer, the editor, the dialogs, the help); shells that are never taken for a program (a few commands, a listing that nearly fills the screen written from
  the top down, a long listing that scrolls, a progress bar that goes up and writes the lines again, a screenful after a clear, a shell that edits its line with absolute positions below
  the top row and writes rows under the cursor); an editor that draws the main screen (taken, and given back when the shell is back; cleared and drawn again; ended by a line that
  scrolls off; a program that starts after sixty lines of output, which ConPTY scrolls up).
- **Live check** (web build, a test program that draws the main screen with cursor addressing only, with no alternate screen and no mouse modes, started after a screenful of output):
  the pane was the program's from the first sample, the wheel moved its selection, a drag selected cells and Ctrl+C copied them, and after `q` the pane went back to the scrollback
  400 ms later with the history and the prompt. `mc.exe` itself could not be started in the live session (the service account of the agent, see the stage 2); its recordings are in the tests.
- **What the first live check found**: the first version ended the ownership at any line that scrolled off, and ConPTY scrolls 15 lines when the program clears; the rule is
  narrowed to the scrolls since the cursor was in the top row (a test for it).

**Found, and not done: the history of the web terminal is cut at 32767 characters.** The history is the rich text of thousands of lines in one `TextEdit`, and the engine gives every
`TextEdit` the default `maximumLength` of a `TextInput`, 32767 (Qt has no such limit for a `TextEdit`). When the history is longer than that (about 250 lines) the end of it, with the
prompt and the output that is being written, is not shown. Taking the limit away is not enough (it was tried: the page froze, because the engine lays out the whole rich text again for every
chunk of the output). The history needs a window of its own (the last lines in the `TextEdit`, or a view of rows), which is a change of the pane and not of this stage; the check above was done
with a history that fits.

## Stage 6: the colors of the console, and the mouse only for clicks

**The 16 colors are those of the console of Windows (the scheme Campbell).** The real `mc.exe` in Windows Terminal and the web terminal were put side by side (a screenshot of each) and the
pixels were measured. The screens were the same in structure (the menu bar and the key bar are the color 6, the panels the color 4), and the colors were not: the real blue was `#0036d9`
and the cyan `#378fdc`, ours `#001cc7` and `#12aaaa`. The real ones are the scheme Campbell (`#0037da` and `#3a96dd`), which is what a program of a Windows console is made to look right
in, so `ANSI16` in `TerminalEmulator.js` is that scheme now:

| | 0 black | 1 red | 2 green | 3 yellow | 4 blue | 5 magenta | 6 cyan | 7 white |
|---|---|---|---|---|---|---|---|---|
| normal | `#0c0c0c` | `#c50f1f` | `#13a10e` | `#c19c00` | `#0037da` | `#881798` | `#3a96dd` | `#cccccc` |
| bright (8..15) | `#767676` | `#e74856` | `#16c60c` | `#f9f1a5` | `#3b78ff` | `#b4009e` | `#61d6d6` | `#f2f2f2` |

The colors 16..255 are those of xterm as before. A test holds the values. Measured again in the web build after the change: the blue of the panels comes out at about `#0444d4` on a
compressed screenshot (`#0036d9` in the real one, `#001cc7` before). The skin of `mc` and its menu are the same as in the real one; what the real screenshot shows in a different place (the
dropdown of the menu) is `mc`'s own drawing, which the agent's `mc` does not make the same way (F9 draws no pull-down under ConPTY of Windows 10, see the stage 2).

**The mouse is reported for clicks and drags, not for the pointer that only passes.** On Windows 11 the mouse reaches `mc.exe`, and `mc` asks for "any motion" (`?1003`), so the
pane sent a report to the agent for every cell the pointer crossed, each of them a request. Now the pane sends the motion only while a button is down (a drag: one report for each
cell that the pointer enters), never the motion without a button. Checked in the web build with the mode `?1003` on: three moves of the pointer over the screen sent nothing, a
click sent the press and the release (`ESC[<0;13;8M`, `ESC[<0;13;8m`), a drag sent the press, the motion with the button down (`ESC[<32;17;15M`, `ESC[<32;25;16M`) and the release.
`encodeMouse` of the module still makes the reports of the modes `?1002` and `?1003` (the module is the protocol, the pane has the policy).

**The terminal has its own scheme, not the theme of the application.** The screen is dark as the window of a console whatever the theme is, so the default colors are the ones of Campbell: the text
`#cccccc` on `#0c0c0c`, the selection `#264f78`, the cursor a block of the text color, the errors of the shell `#e74856` and its system messages `#767676`. They are the properties `terminalFg`,
`terminalBg`, `terminalSelection`, `terminalError` and `terminalSystem` of the pane, used by the grid view, the scrollback view, the translator of the colors of the plain shell (`ansiToHtml`, its
16 colors are those of the emulator) and the overlays on the screen. The theme colors are left to the parts around the screen (the status bar, the popups). Before, the text of a plain shell was
the color of the theme (dark gray for the light theme) on the black screen, which looked dim.

## The desktop (Qt) client

The terminal pane is the same QML in the desktop client and in the web build, so the emulator (`TerminalEmulator.js`) and the pane run in both. What was checked on the
desktop side, with Qt 6.11.2 (MSVC, Debug):

- **The module in the JavaScript engine of Qt**: `qml.exe` ran the recording of `mc.exe` through `TerminalEmulator.js` (`import "TerminalEmulator.js" as Emulator`) and the
  result is the same as in node (V8): the text and the rich text of the screen (checksums), the number of cells with a color and with a background, the cursor, the codes of keys.
  A QML `color` reaches the library as `#rrggbb`, which `hexColor` takes (`#aarrggbb` too).
- **The rich text in the text engine of Qt**: the rows of the recorded screens (the panels, the dialog of mcedit, the help) were drawn as the pane draws them (a `Text` per row,
  `textFormat: Text.RichText`, `<span style="white-space:pre">` and the spans of `rowHtml`, Consolas 14 px: a cell of 8 x 17 px) and saved as images. The colors of the skin of
  `mc`, the backgrounds, the bold and the frames come out right, the rows touch without gaps, and the vertical lines of the frames and the columns line up.
- **The client builds**: `AgentinoClient` (Ninja, MSVC 19.51) with the new `agentinoqml.qrc` (the module and `TerminalWheelHandler.qml` in the resources) and the pane compiled
  from this worktree; `qmllint` finds no error in the pane and no warning in the wheel handler.
- **Not done**: the client was not driven by hand or by a script: the terminal tab was not opened in the running desktop window, so the keys, the mouse, `WheelHandler`
  and the timer of the blink are checked in the web build only. The client was not started because the platform plugin `offscreen` is not in its folder and a real
  window would connect to the server with the saved settings.

**Building the desktop client from a worktree** (it is what was done; the build goes to a folder of its own and touches neither `Bin` nor the build tree of the main checkout):
configure `Build/CMake` of the worktree with `-G Ninja -DCMAKE_BUILD_TYPE=Debug -DAGENTINODIR=<worktree> -DAGENTINODIR_BUILD=<own folder> -DWEB_COMPILE=OFF -DBUILD_TESTING=OFF` in the
environment of MSVC with `IACFDIR`, `ACFDIR`, `ACFSLNDIR`, `IMTCOREDIR` and `IMTCOREDIR_BUILD` set (paths with `/`, a `\` in `AGENTINODIR_BUILD` breaks the SDL generators), then build the
target `AgentinoClient`. Three things of a worktree have to be given by hand: `Partitura/AgentinoVoce.arp/VersionInfo.acc` (generated, ignored by git: copy it from the main checkout),
the folder `ImtCore` next to the worktree (the SDL schemas import `../../../../ImtCore/Sdl/...` relative to the checkout; a junction made for the build) and `AgentinoClient.res` (the
resources of the exe: copy it from the main build). The first build needs minutes (the SDL code is generated again).

## Open questions

- The recording does not have the mouse, the viewer (F3), the editor (F4) and the dialogs of copy: they are to be recorded for the stages 2 and 3 (the script of keys is in `record.mjs`). `mc` of Linux (with `?1049`, `ESC ( 0` in some skins) is to be recorded on a Linux agent.
- A monospace font that has the box-drawing and Cyrillic glyphs on every client (the browser decides for the generic `monospace`); if it is not good enough, a font file is bundled and loaded by the page.
- The scrollback of a full-screen application is not kept (as in other terminals); the output of the primary screen is as it is now.
- Wide characters and braille in the web build (see the glyph widths of the stage 4): the placed `Text` items for the grid view.
- The history of the web terminal is cut at 32767 characters (see the stage 5): it needs a window of the last lines.
- A full-screen program that has nothing under its cursor (`less`: the cursor is in the last row) is not recognized on the main screen; it is drawn in the scrollback view as before.
