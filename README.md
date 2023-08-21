<img align="right" style="width: 256px" src="assets/logo.svg">

# hy3
<a href="https://matrix.to/#/#hy3:outfoxxed.me"><img src="https://img.shields.io/badge/Join%20the%20matrix%20room-%23hy3:outfoxxed.me-0dbd8b?logo=matrix&style=flat-square"></a>

i3 / sway like layout for [hyprland](https://github.com/hyprwm/hyprland).

[Installation](#installation), [Configuration](#configuration)

### Features
- [x] i3 like tiling
- [x] Node based window manipulation (you can interact with multiple windows at once)
- [x] Greatly improved tabbed node groups over base hyprland
- [x] Optional autotiling

Additional features may be suggested in the repo issues or the [matrix room](https://matrix.to/#/#hy3:outfoxxed.me).

### Demo
<video width="640" height="360" controls="controls" src="https://user-images.githubusercontent.com/83010835/255322916-85ae8196-8b12-4e15-b060-9872db10839f.mp4"></video>

### Stability
hy3 always tracks the latest (git) version of hyprland. It likely will not work when built against a given stable
hyprland release, however it will probably work within a few commits of the latest hyprland version. Please ping
`@outfoxxed:outfoxxed.me` in the [matrix room](https://matrix.to/#/#hy3-general:outfoxxed.me) or make an issue
if hy3 does not build or function against the latest hyprland commit.

Commits are tested for obvious crashes before pushing and usually fine, however you may occasionally find a bug.

If you encounter any bugs, please report them in the issue tracker.

When reporting bugs, please include:
- Commit hash of the version you are running.
- Steps to reproduce the bug (if you can figure them out)
- backtrace of the crash (if applicable)

If you are too lazy to use the issue tracker, please at least ping `@outfoxxed:outfoxxed.me`
in the [matrix room](https://matrix.to/#/#hy3-support:outfoxxed.me) with your bug information.

## Installation
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
There is an unofficial [hy3-git](https://aur.archlinux.org/packages/hy3-git) package,
usable with the `hyprland-git` package.

### Manual
Install hyprland, including its headers and pkg-config file, then run the following commands:

```sh
cmake -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build
```

The plugin will be located at `build/libhy3.so`, and you can load it normally
(See [the hyprland wiki](https://wiki.hyprland.org/Plugins/Using-Plugins/#installing--using-plugins) for details.)

Note that the hyprland headers and pkg-config file **MUST be installed correctly, for the target version of hyprland**.

## Configuration
Set your `general:layout` to `hy3` in hyprland.conf.

hy3 requires using a few custom dispatchers for normal operation.
In your hyprland config replace the following dispatchers:
 - `movefocus` -> `hy3:movefocus`
 - `movewindow` -> `hy3:movewindow`

You can use `hy3:makegroup` to create a new split.

The [dispatcher list](#dispatcher-list) and [config fields](#config-fields) sections have all the
configuration options, and some explanation as to what they do.
[The hyprland config in my dots](https://git.outfoxxed.me/outfoxxed/nixnew/src/branch/master/modules/hyprland/hyprland.conf) can also be used as a reference.

### Config fields
```conf
plugin {
  hy3 {
    # disable gaps when only one window is onscreen
    no_gaps_when_only = <bool> # default: false

    # policy controlling what happens when a node is removed from a group,
    # leaving only a group
    # 0 = remove the nested group
    # 1 = keep the nested group
    # 2 = keep the nested group only if its parent is a tab group
    node_collapse_policy = <int> # default: 2

    # offset from group split direction when only one window is in a group
    group_inset = <int> # default: 10

    # scale factor of windows on the special workspace
    special_scale_factor = <float> # default: 0.8

    # tab group settings
    tabs {
      # height of the tab bar
      height = <int> # default: 15

      # padding between the tab bar and its focused node
      padding = <int> # default: 5

      # the tab bar should animate in/out from the top instead of below the window
      from_top = <bool> # default: false

      # rounding of tab bar corners
      rounding = <int> # default: 3

      # render the window title on the bar
      render_text = <bool> # default: true

      # font to render the window title with
      text_font = <string> # default: Sans

      # height of the window title
      text_height = <int> # default: 8

      # left padding of the window title
      text_padding = <int> # default: 3

      # active tab bar segment color
      col.active = <color> # default: 0xff32b4ff

      # urgent tab bar segment color
      col.urgent = <color> # default: 0xffff4f4f

      # inactive tab bar segment color
      col.inactive = <color> # default: 0x80808080

      # active tab bar text color
      col.text.active = <color> # default: 0xff000000

      # urgent tab bar text color
      col.text.urgent = <color> # default: 0xff000000

      # inactive tab bar text color
      col.text.inactive = <color> # default: 0xff000000
    }

    # autotiling settings
    autotile {
      # enable autotile
      enable = <bool> # default: false

      # make autotile-created groups ephemeral
      ephemeral_groups = <bool> # default: true

      # if a window would be squished smaller than this width, a vertical split will be created
      # -1 = never automatically split vertically
      # 0 = always automatically split vertically
      # <number> = pixel height to split at
      trigger_width = <int> # default: 0

      # if a window would be squished smaller than this height, a horizontal split will be created
      # -1 = never automatically split horizontally
      # 0 = always automatically split horizontally
      # <number> = pixel height to split at
      trigger_height = <int> # default: 0
    }
  }
}
```

### Dispatcher list
 - `hy3:makegroup, <h | v | opposite | tab>, [ephemeral | force_ephemeral]` - make a vertical / horizontal split or tab group
   - `ephemeral` - the group will be removed once it contains only one node. does not affect existing groups.
   - `force_ephemeral` - same as ephemeral, but converts existing single windows groups.
 - `hy3:changegroup, <h | v | tab | untab | toggletab | opposite>` - change the group the node belongs to, to a different layout
   - `untab` will untab the group if it was previously tabbed
   - `toggletab` will untab if group is tabbed, and tab if group is untabbed
   - `opposite` will toggle between horizontal and vertical layouts if the group is not tabbed.
 - `hy3:setephemeral, <true | false>` - change the ephemerality of the group the node belongs to
 - `hy3:movefocus, <l | u | d | r | left | down | up | right>, [visible]` - move the focus left, up, down, or right
   - `visible` - only move between visible nodes, not hidden tabs
 - `hy3:movewindow, <l | u | d | r | left | down | up | right>, [once], [visible]` - move a window left, up, down, or right
   - `once` - only move directly to the neighboring group, without moving into any of its subgroups
   - `visible` - only move between visible nodes, not hidden tabs
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
 - :warning: **ALPHA QUALITY** `hy3:setswallow, <true | false | toggle>` - set the containing node's window swallow state
 - :warning: **ALPHA QUALITY** `hy3:expand, <expand | shrink | base>` - expand the current node to cover other nodes
   - `expand` - expand by one node
   - `shrink` - shrink by one node
   - `base` - undo all expansions
