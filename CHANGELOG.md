# Changelog

# Upcoming

- Added `active_alt_monitor` tab bar color options.

# hl0.51.0 and before

- Only compatibility fixes.

# hl0.50.0 and before

- Only compatibility fixes.

# hl0.49.0 and before

- Only compatibility fixes.

# hl0.48.0 and before
- Added `toggle` to `hy3:makegroup`
- Added `hy3:locktab`

# hl0.47.0-1 and before
- Added focused tab color, to mark selected tabs inside an unfocused container.
- Fixed tab rendering on rotated displays.
- Fixed tabs being clickable throguh fullscreen windows.
- Fixed tabs boing clickable through their own windows.
- Fixed incorrect window focus when switching tabs.

# hl0.47.0 and before

- Added configurable borders and blurred backgrounds to tabs.
- Added warp option to `hy3:movetoworkspace`.
- Renamed `hy3:tabs:rounding` to `hy3:tabs:radius`.
- Replaced `hy3:focustab mouse` with an unconditional hook that works better.
- Renamed tab color options.
- Changed default tab style.
- Fixed floating windows losing their size after maximizing and then going into fullscreen.
- Fixed tab bars being clickable while covered by layers or with active pointer grabs.
- Fixed bugs when moving nodes across workspaces.
- Fixed broken tab damage tracking (artifacts when tabs animate).

# hl0.46.0 and before

- Only compatibility fixes.

# hl0.45.0 and before

- Added hyprsplit compatibility for hy3:movetoworkspace
- Added support for special workspaces
- Added monitor switching support to hy3:movefocus
- Added support for gapsin and gapsout workspace rules
- Fixed tab bars with no_gaps_when_only

# hl0.44.0 and before

- Fixed tab bars on root node not respecting outer gaps
- Fixed tab bars clipping when switching workspaces with slidevert
- Improved performance of tab bars
- Tab bars now use windowsMove for all animations
- Fixed tab bars rendering in the wrong workspace
- Floating windows will now be focused when all tiled windows are closed
- Added `hy3:togglefocuslayer`

# hl0.43.0 and before

- Fixed blurry tab bar text

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
