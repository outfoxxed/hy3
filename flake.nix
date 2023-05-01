{
  inputs = {
    hyprland.url = "github:hyprwm/Hyprland";
  };

  outputs = { self, hyprland, ... }: let
    inherit (hyprland.inputs) nixpkgs;
    hyprlandSystems = fn: nixpkgs.lib.genAttrs (builtins.attrNames hyprland.packages) (system: fn system nixpkgs.legacyPackages.${system});
  in {
    packages = hyprlandSystems (system: pkgs: rec {
      hy3 = pkgs.gcc12Stdenv.mkDerivation {
        pname = "hy3";
        version = "0.1";
        src = ./.;

        nativeBuildInputs = with pkgs; [ cmake pkg-config ];

        buildInputs = [
          hyprland.packages.${system}.hyprland.dev
        ] ++ hyprland.packages.${system}.hyprland.buildInputs;

        # no noticeable impact on performance and greatly assists debugging
        cmakeBuildType = "Debug";
        dontStrip = true;

        meta = with pkgs.lib; {
          homepage = "https://github.com/outfoxxed/hy3";
          description = "Hyprland plugin for an i3 / sway like manual tiling layout";
          license = licenses.gpl3;
          platforms = platforms.linux;
        };
      };

      default = hy3;
    });

    devShells = hyprlandSystems (system: pkgs: {
      default = pkgs.mkShell.override { stdenv = pkgs.gcc12Stdenv; } {
        name = "hy3";

        nativeBuildInputs = with pkgs; [
          clang-tools_15
          bear
        ];

        inputsFrom = [ self.packages.${system}.hy3 ];
      };
    });

    homeManagerModules.default = { config, lib, pkgs, ... }: let
      cfg = config.wayland.windowManager.hyprland.plugins.hy3;
      hy3Package = self.packages.${pkgs.hostPlatform.system}.default;
    in {
      options.wayland.windowManager.hyprland.plugins.hy3 = {
        enable = lib.mkEnableOption "hy3 plugin";

        package = lib.mkOption {
          type = lib.types.package;
          default = hy3Package;
        };
      };

      config = lib.mkIf cfg.enable {
        wayland.windowManager.hyprland.extraConfig = ''
          plugin = ${cfg.package}/lib/libhy3.so
        '';
      };
    };
  };
}
