{
  hyprland,
  pkgs,
  hlversion ? "git",
  hy3 ? pkgs.callPackage ./default.nix {
    inherit hyprland hlversion;
    versionCheck = false;
  },
}: pkgs.mkShell.override {
  inherit (hy3) stdenv;
} {
  inputsFrom = [ hy3 ];

  nativeBuildInputs = with pkgs; [
    clang-tools
    bear
  ];
}
