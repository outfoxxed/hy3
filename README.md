# hy3
i3 / sway like layout for [hyprland](https://github.com/hyprwm/hyprland).

### Features
- [x] i3 like tiling
- [x] Window splits
- [x] Window movement
- [x] Window resizing
- [x] Selecting a group of windows at once (and related movement)
- [x] Tabbed groups
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

### Config fields
```conf
plugin {
  hy3 {
    # disable gaps when only one window is onscreen
    no_gaps_when_only = <bool>

    # offset from group split direction when only one window is in a group
    group_inset = <int>

    # tab group settings
    tabs {
      # height of the tab bar
      height = <int>

      # padding between the tab bar and its focused node
      padding = <int>

      # the tab bar should animate in/out from the top instead of below the window
      from_top = <bool>

      # render the window title on the bar
      render_text = <bool>

      # rounding of tab bar corners
      rounding = <int>

      # font to render the window title with
      text_font = <string>

      # height of the window title
      text_height = <int>

      # left padding of the window title
      text_padding = <int>

      # active tab bar segment color
      col.active = <color>

      # urgent tab bar segment color
      col.urgent = <color>

      # inactive tab bar segment color
      col.inactive = <color>

      # active tab bar text color
      col.text.active = <color>

      # urgent tab bar text color
      col.text.urgent = <color>

      # inactive tab bar text color
      col.text.inactive = <color>
    }
  }
}
```

### Dispatcher list
 - `hy3:makegroup, <h | v | opposite | tab>` - make a vertical / horizontal split or tab group
 - `hy3:movefocus, <l | u | d | r | left | down | up | right>, [visible]` - move the focus left, up, down, or right
   - `visible` - only move between visible nodes, not hidden tabs
 - `hy3:movewindow, <l | u | d | r | left | down | up | right>, [once]` - move a window left, up, down, or right
   - `once` - only move directly to the neighboring group, without moving into any of its subgroups
 - `hy3:killactive` - close all windows in the focused node
 - `hy3:changefocus, <top | bottom | raise | lower | tab | tabnode>`
   - `top` - focus all nodes in the workspace
   - `bottom` - focus the single root selection window
   - `raise` - raise focus one level
   - `lower` - lower focus one level
   - `tab` - raise focus to the nearest tab
   - `tabnode` - raise focus to the nearest node under the tab
 - `hy3:focustab <mouse | [l | r | left | right | index, <index>], [prioritize_hovered | require_hovered], [wrap]>`
   - `mouse` - focus the tab under the mouse, works well with a non consuming bind, e.g.
     ```conf
     # binds hy3:focustab to lmb and still allows windows to receive clicks
     bindn = , mouse:272, hy3:focustab, mouse
     ```
   - `l | r | left | right` - direction to change focus towards
   - `index, <index>` - select the `index`th tab
   - `prioritize_hovered` - prioritize the tab group under the mouse when multiple are stacked. use the lowest group if none is under the mouse.
   - `require_hovered` - affect the tab group under the mouse. do nothing if none are hovered.
   - `wrap` - wrap to the opposite size of the tab bar if moving off the end
 - `hy3:debugnodes` - print the node tree into the hyprland log

## Installing

### Nix
#### Hyprland home manager module
Assuming you use hyprland's home manager module, you can easily integrate hy3 by adding it to the plugins array.

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

        {
          wayland.windowManager.hyprland = {
            enable = true;
            plugins = [ hy3.packages.x86_64-linux.hy3 ];
          };
        }
      ];
    };
  };
}
```

#### Manual (Nix)
hy3's binary is availible as `${hy3.packages.<system>.hy3}/lib/libhy3.so`, so you can also
directly use it in your hyprland config like so:

```nix
# ...
wayland.windowManager.hyprland = {
  # ...
  extraConfig = ''
    plugin = ${hy3.packages.x86_64-linux.hy3}/lib/libhy3.so
  '';
};
```

### Arch (AUR)
There is an unofficial [hy3-git](https://aur.archlinux.org/packages/hy3-git) package.

As of writing, the `hyprland-git` AUR package does not properly handle hyprland headers,
which are required for hy3 to build. I have been informed that there is a
[fixed version](https://gist.githubusercontent.com/eclairevoyant/0007c44a52c715f243452d17362104b3/raw/9b834290ac75bef2f3bb1364d1380832c65ed6bf/PKGBUILD)
that places the headers in the correct location.

### Manual
Install hyprland, including its headers and pkg-config file, then run the following commands:

```sh
cmake -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build
```

The plugin will be located at `build/libhy3.so`, and you can load it normally
(See [the hyprland wiki](https://wiki.hyprland.org/Plugins/Using-Plugins/#installing--using-plugins) for details.)

Note that the hyprland headers and pkg-config file **MUST be installed correctly, for the target version of hyprland**.
