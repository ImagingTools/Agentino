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

## Open questions

- The recording does not have the mouse, the viewer (F3), the editor (F4) and the dialogs of copy: they are to be recorded for the stages 2 and 3 (the script of keys is in `record.mjs`). `mc` of Linux (with `?1049`, `ESC ( 0` in some skins) is to be recorded on a Linux agent.
- A monospace font that has the box-drawing and Cyrillic glyphs on every client (the browser decides for the generic `monospace`); if it is not good enough, a font file is bundled and loaded by the page.
- The scrollback of a full-screen application is not kept (as in other terminals); the output of the primary screen is as it is now.
