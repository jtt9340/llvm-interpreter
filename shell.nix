{ pkgs ? import (fetchTarball
  "https://github.com/NixOS/nixpkgs/archive/3590f02e7d5760e52072c1a729ee2250b5560746.tar.gz")
  { } }:

# Create a shell environment that has these packages available
let
  # Use this version of the LLVM toolchain
  llvmPackages = pkgs.llvmPackages_11;
in pkgs.mkShell {
  name = "llvm-interpreter";
  nativeBuildInputs = with pkgs;
    with llvmPackages; [
      bash
      clang
      llvm
      lldb
      shfmt
      nixfmt
    ];
}
