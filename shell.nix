# Copyright (C) 2005 - 2022 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

{ pkgs ? import <nixpkgs> {} }:

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
