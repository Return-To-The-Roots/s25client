{ pkgs ? import <nixpkgs> {} }:

with pkgs;
stdenv.mkDerivation {
  name = "s25client";
  buildInputs = [
    boost
    bzip2
    cmake
    curl
    gettext
    libiconv
    miniupnpc
    SDL
    SDL_mixer
  ];
}
