# Copyright (C) 2005 - 2022 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

{
  pkgs ? import <nixpkgs> { },
}:

with pkgs;
mkShell {
  name = "s25client";

  # Set SOURCE_DATE_EPOCH to the newest file in the directory.
  # RTTR_BUILD_DATE is calculated by that epoch value.
  shellHook = ''
    updateSourceDateEpoch .
  '';

  buildInputs = [
    stdenv
    boost
    bzip2
    cmake
    curl
    gettext
    libiconv
    libsamplerate
    lua
    miniupnpc
    SDL2
    SDL2_mixer
  ];
}
