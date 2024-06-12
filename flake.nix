{
  inputs = {
    hyprland.url = "git+https://github.com/hyprwm/Hyprland?submodules=1&rev=38132ffaf53c843a3ed03be5b305702fde8f5a49";
  };

  outputs = {
    self,
    hyprland,
    ...
  }: let
    inherit (hyprland.inputs) nixpkgs;

    hyprlandSystems = fn:
      nixpkgs.lib.genAttrs
      (builtins.attrNames hyprland.packages)
      (system: fn system nixpkgs.legacyPackages.${system});

    props = builtins.fromJSON (builtins.readFile "${hyprland}/props.json");
  in {
    packages = hyprlandSystems (system: pkgs: rec {
      hy3 = pkgs.callPackage ./default.nix {
        hyprland = hyprland.packages.${system}.hyprland;
        hlversion = props.version;
      };
      default = hy3;
    });

    devShells = hyprlandSystems (system: pkgs: {
      default = import ./shell.nix {
        inherit pkgs;
        hlversion = props.version;
        hyprland = hyprland.packages.${system}.hyprland-debug;
      };

      impure = import ./shell.nix {
        pkgs = import <nixpkgs> {};
        hlversion = props.version;
        hyprland = (pkgs.appendOverlays [hyprland.overlays.hyprland-packages]).hyprland-debug.overrideAttrs {
          dontStrip = true;
        };
      };
    });
  };
}
