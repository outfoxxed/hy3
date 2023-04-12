{
  inputs = {
    hyprland.url = "github:hyprwm/Hyprland";
    nixpkgs.follows = "hyprland/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { nixpkgs, hyprland, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs { inherit system; };
      hyprpkgs = hyprland.packages.${system};
    in {
      packages.default = pkgs.gcc12Stdenv.mkDerivation {
        pname = "hy3";
        version = "0.1";

        src = ./.;

        nativeBuildInputs = with pkgs; [
          cmake
          pkg-config
        ];

        #HYPRLAND_HEADERS = hyprpkgs.hyprland.src; - TODO
      };

      devShells.default = pkgs.mkShell.override { stdenv = pkgs.gcc12Stdenv; } {
        name = "hy3-shell";
        nativeBuildInputs = with pkgs; [
          cmake
          pkg-config

          clang-tools_15
          bear
        ];

        buildInputs = with pkgs; [
          hyprpkgs.wlroots-hyprland
          libdrm
          pixman
        ];

        inputsFrom = [
          hyprpkgs.hyprland
          hyprpkgs.wlroots-hyprland
        ];

        #HYPRLAND_HEADERS = hyprpkgs.hyprland.src; - TODO
      };
    });
}
