# hy3
i3 / sway like layout for [hyprland](https://github.com/hyprwm/hyprland).

### Features
- [x] i3 like tiling
- [x] Window splits
- [x] Window movement
- [x] Window resizing
- [ ] Selecting a group of windows at once (and related movement)
- [ ] Tabbed groups
- [ ] Some convenience dispatchers not found in i3 or sway

### Stability
As of now hy3 is stable enough to use normally as long as you don't change layout after loading the plugin (currently unimplemented).
If you encounter any crashes or bugs please report them in the issue tracker.

When reporting bugs, please include:
- Commit hash of the version you are running.
- Steps to reproduce (if you can figure them out)
- backtrace of the crash

If you don't know how to reproduce it or can't, or you can't take a backtrace please still report the issue.

## Configuration
Set your `general:layout` to `hy3` in hyprland.conf.

hy3 requires using a few custom dispatchers for normal operation.
In your hyprland config replace the following dispatchers:
 - `movefocus` -> `hy3:movefocus`
 - `movewindow` -> `hy3:movewindow`

You can use `hy3:makegroup` to create a new split.

### Dispatcher list
 - `hy3:makegroup, <h | v>` - make a vertical or horizontal split
 - `hy3:movefocus, <l | u | d | r>` - move the focus left, up, down, or right
 - `hy3:movewindow, <l | u | d | r>` - move a window left, up, down, or right
 - `hy3:debugnodes` - print the node tree into the hyprland log

## Installing

### Nix
Under nix, use the provided devShell, then go to [Manual Installation](#manual)

### [Hyprload](https://github.com/Duckonaut/hyprload) (currently untested)
Add an entry to your hyprload.toml like so:

```toml
plugins = [
  # ...
  "outfoxxed/hy3",
  # ...
]
```

### Manual
First export `HYPRLAND_HEADERS`, then run the following commands:

```sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

> **Note**: Please use a debug build as debugging a backtrace from a release build is much more difficult if you need to report an error.

The plugin will be located at `build/libhy3.so`.
