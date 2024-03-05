{
  hyprland,
  pkgs,
  hlversion ? "git",
  hy3 ? pkgs.callPackage ./default.nix {
    inherit hyprland hlversion;
    versionCheck = false;
  },
}: pkgs.mkShell {
  inputsFrom = [ hy3 ];

  nativeBuildInputs = with pkgs; [
    clang-tools_17
    bear
  ];
}
