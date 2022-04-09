# Copyright (C) 2005 - 2022 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

{
  pkgs ? import <nixpkgs> { overlays = [
    # This is a workaround for https://github.com/NixOS/nixpkgs/issues/146759
    # NixOS does currently ship an incomple SDL2 installation when withStatic=false.
    # See https://nixpk.gs/pr-tracker.html?pr=149601 as state for a PR on NixOS side.
    (final: prev: {
      SDL2 = prev.SDL2.override {
        withStatic = true;
      };
    })];
  }
}:

with pkgs;
mkShell {
  name = "s25client";
  buildInputs = [
    boost
    bzip2
    cmake
    curl
    gettext
    libiconv
    miniupnpc
    SDL2
    SDL2_mixer
    libsamplerate
  ];
}
