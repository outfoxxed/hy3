{
  inputs = {
    hyprland.url = "github:hyprwm/hyprland/5bb8adbc3228901d199e8d22d6f712bd1d7d4e15";
  };

  outputs =
    {
      self,
      hyprland,
      ...
    }:
    let
      inherit (hyprland.inputs) nixpkgs;

      hyprlandSystems =
        fn:
        nixpkgs.lib.genAttrs (builtins.attrNames hyprland.packages) (
          system: fn system nixpkgs.legacyPackages.${system}
        );

      hyprlandVersion = nixpkgs.lib.removeSuffix "\n" (builtins.readFile "${hyprland}/VERSION");
    in
    {
      packages = hyprlandSystems (
        system: pkgs: rec {
          hy3 = pkgs.callPackage ./default.nix {
            hyprland = hyprland.packages.${system}.hyprland;
            hlversion = hyprlandVersion;
          };
          default = hy3;
        }
      );

      devShells = hyprlandSystems (
        system: pkgs: {
          default = import ./shell.nix {
            inherit pkgs;
            hlversion = hyprlandVersion;
            hyprland = hyprland.packages.${system}.hyprland;
          };

          impure = import ./shell.nix {
            pkgs = import <nixpkgs> { };
            hlversion = hyprlandVersion;
            hyprland = (pkgs.appendOverlays [ hyprland.overlays.hyprland-packages ]).hyprland.overrideAttrs {
              dontStrip = true;
            };
          };
        }
      );
    };
}
