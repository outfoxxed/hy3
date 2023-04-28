# hy3
i3 / sway like layout for [hyprland](https://github.com/hyprwm/hyprland).

### Features
- [x] i3 like tiling
- [x] Window splits
- [x] Window movement
- [x] Window resizing
- [x] Selecting a group of windows at once (and related movement)
- [ ] Tabbed groups
- [ ] Some convenience dispatchers not found in i3 or sway

### Stability
As of now hy3 is stable enough to use normally.
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
 - `hy3:raisefocus` - raise the active focus one level
 - `hy3:debugnodes` - print the node tree into the hyprland log

## Installing

### Nix
#### Using the home-manager module
Assuming you use hyprland's home manager module, you can easily integrate hy3, as hy3 provides a home manager module that exposes the `wayland.windowManager.hyprland.plugins.hy3.enable` option.

```nix
# flake.nix

{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    home-manager = {
      url = "github:nix-community/home-manager";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    hyprland.url = "github:hyprwm/Hyprland";

    hy3 = {
      url = "github:outfoxxed/hy3";
      inputs.hyprland.follows = "hyprland";
    };
  };

  outputs = { nixpkgs, home-manager, hyprland, hy3, ... }: {
    homeConfigurations."user@hostname" = home-manager.lib.homeManagerConfiguration {
      pkgs = nixpkgs.legacyPackages.x86_64-linux;

      modules = [
        hyprland.homeManagerModules.default
        hy3.homeManagerModules.default

        {
          wayland.windowManager.hyprland = {
            enable = true;
            plugins.hy3.enable = true;
          };
        }
      ];
    };
  };
}
```

#### Manually (Nix)
hy3's binary is availible as `${hy3.packages.<system>.hy3}/lib/libhy3.so`, so you can also
directly use it in your hyprland config like so:

```nix
# ...
wayland.windowManager.hyprland = {
  # ...
  extraConfig = ''
    exec-once = hyprctl plugin load ${hy3.packages.x86_64-linux.hy3}/lib/libhy3.so
  '';
};
```

### [Hyprload](https://github.com/Duckonaut/hyprload)
Add an entry to your hyprload.toml like so:

```toml
plugins = [
  # ...
  "outfoxxed/hy3",
  # ...
]
```

### Manual
Install hyprland, then run the following commands:

```sh
cmake -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build
```

The plugin will be located at `build/libhy3.so`, and you can load it normally
(See [the hyprland wiki](https://wiki.hyprland.org/Plugins/Using-Plugins/#installing--using-plugins) for details.)
