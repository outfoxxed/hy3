# Changelog

# hl0.41.1 and before

- Fixed glitchy tab bar effects when switching workspaces with fade effect.
- Fixed wrongly sized layout when moving resized nodes between workspaces.

# hl0.41.0 and before

- Fixed IPC and wlr-foreign-toplevel not getting fullscreen and maximize events.
- Fixed glitches when moving nodes between monitors with hy3 dispatchers.
- Fixed glitchy tab bar effects when switching workspaces

## hl0.40.0 and before

- Added `hy3:warpcursor` dispatcher to warp cursor to current node.
- Added cursor warping options for `hy3:movefocus`.

## hl0.37.1 and before

- Added `no_gaps_when_only = 2`
- Fixed fullscreen not working on workspaces with only floating windows

## hl0.36.0 and before

- Implement `resizeactivewindow` for floating windows
- Fully implement `resizeactivewindow` for tiled windows

## hl0.35.0 and before

- Fixed `hy3:killactive` and `hy3:movetoworkspace` not working in fullscreen.
- `hy3:movetoworkspace` added to move a whole node to a workspace.
- Newly tiled windows (usually from moving a window to a new workspace) are now
placed relative to the last selected node.

## hl0.34.0 and before
*check commit history*
