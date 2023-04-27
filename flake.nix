{
  inputs = {
    hyprland.url = "github:hyprwm/Hyprland";
    nixpkgs.follows = "hyprland/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { nixpkgs, hyprland, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs { inherit system; };
      hyprland_pkg = hyprland.packages.${system}.hyprland;
    in rec {
      packages.default = pkgs.gcc12Stdenv.mkDerivation {
        pname = "hy3";
        version = "0.1";

        src = ./.;

        nativeBuildInputs = with pkgs; [
          hyprland_pkg.dev
          cmake
          pkg-config
        ] ++ hyprland_pkg.buildInputs;
      };

      devShells.default = pkgs.mkShell.override { stdenv = pkgs.gcc12Stdenv; } {
        name = "hy3-shell";

        nativeBuildInputs = with pkgs; [
          clang-tools_15
          bear
        ];

        inputsFrom = [ packages.default ];
      };
    });
}
