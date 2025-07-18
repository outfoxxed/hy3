{
  inputs = {
    hyprland.url = "github:hyprwm/hyprland/c4a4c341568944bd4fb9cd503558b2de602c0213";
  };

  outputs = { self, hyprland, ... }: let
    inherit (hyprland.inputs) nixpkgs;

    hyprlandSystems = fn: nixpkgs.lib.genAttrs
      (builtins.attrNames hyprland.packages)
      (system: fn system nixpkgs.legacyPackages.${system});

    hyprlandVersion = nixpkgs.lib.removeSuffix "\n" (builtins.readFile "${hyprland}/VERSION");
  in {
    packages = hyprlandSystems (system: pkgs: rec {
      hy3 = pkgs.callPackage ./default.nix {
        hyprland = hyprland.packages.${system}.hyprland;
        hlversion = hyprlandVersion;
      };
      default = hy3;
    });

    devShells = hyprlandSystems (system: pkgs: {
      default = import ./shell.nix {
        inherit pkgs;
        hlversion = hyprlandVersion;
        hyprland = hyprland.packages.${system}.hyprland;
      };

      impure = import ./shell.nix {
        pkgs = import <nixpkgs> {};
        hlversion = hyprlandVersion;
        hyprland = (pkgs.appendOverlays [ hyprland.overlays.hyprland-packages ]).hyprland.overrideAttrs {
          dontStrip = true;
        };
      };
    });
  };
}
