{ nixpkgs ? import ./fetchNixpkgs.nix { }
, system ? builtins.currentSystem
}:

let
  pkgs = import nixpkgs { inherit system; };

in rec {
  my_weather_acl = pkgs.callPackage ./my_weather_acl { };
}
