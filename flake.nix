{
  inputs = {
    hyprland.url = "github:hyprwm/Hyprland?rev=72cb5d24b6894334da021157c626556a8e99b6ff";
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
        hyprland = hyprland.packages.${system}.hyprland-debug;
      };

      impure = import ./shell.nix {
        pkgs = import <nixpkgs> {};
        hlversion = hyprlandVersion;
        hyprland = (pkgs.appendOverlays [ hyprland.overlays.hyprland-packages ]).hyprland-debug.overrideAttrs {
          dontStrip = true;
        };
      };
    });
  };
}
