{
  hyprland,

  lib,
  nix-gitignore,
  keepDebugInfo,
  stdenv ? (keepDebugInfo hyprland.stdenv),

  cmake,
  ninja,
  pkg-config,
  pango,
  cairo,

  debug ? false,
  hlversion ? "git",
  versionCheck ? true,
}: stdenv.mkDerivation {
  pname = "hy3";
  version = "hl${hlversion}${lib.optionalString debug "-debug"}";
  src = nix-gitignore.gitignoreSource [] ./.;

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
  ];

  buildInputs = [
    hyprland.dev
    pango
    cairo
  ] ++ hyprland.buildInputs;

  cmakeFlags = lib.optional (!versionCheck) "-DHY3_NO_VERSION_CHECK=ON";

  cmakeBuildType = if debug
                   then "Debug"
                   else "RelWithDebInfo";

  buildPhase = "ninjaBuildPhase";
  enableParallelBuilding = true;
  dontStrip = true;

  meta = with lib; {
    homepage = "https://github.com/outfoxxed/hy3";
    description = "Hyprland plugin for an i3 like manual tiling layout";
    license = licenses.gpl3;
    platforms = platforms.linux;
  };
}
