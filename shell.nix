{ pkgs ? import (fetchTarball
  "https://github.com/NixOS/nixpkgs/archive/3590f02e7d5760e52072c1a729ee2250b5560746.tar.gz")
  { }
  # Use this version of the LLVM toolchain
, llvm ? pkgs.llvmPackages_11 }:

# TODO JOEY Figure out how to use the libc++ bundled with LLVM.
# The code commented out does this, except clang can't find stddef.h
# (see below for attempts to fix this, I am able to get stddef.h in the include
# path but for some reason clang can't find it?! Even with using what's not
# commented out now, the include path contains a symlink to the directory that
# containd stddef.h. This makes no sense):<
# Create a shell environment that has these packages available
# let
#   # Do not use libc(++) provided by GCC, use the one provided by LLVM
#   # (see https://nixos.wiki/wiki/C#Get_a_compiler_without_default_libc)
#   clang = pkgs.wrapCCWith {
#     cc = llvm.clang-unwrapped;
#     bintools = pkgs.wrapBintoolsWith {
#       bintools = pkgs.bintools-unwrapped;
#     # libc = null;
#     };
#     libcxx = llvm.libcxx;
#   };
# in (pkgs.overrideCC llvm.stdenv clang).mkDerivation {
llvm.stdenv.mkDerivation {
  name = "llvm-interpreter";
  buildInputs = with pkgs; [
    llvm.llvm # To build the interpreter
    llvm.lldb # To debug the interpreter
    shfmt # For formatting shell scripts in the 'fmt' make target
    nixfmt # For formatting this file in the 'fmt' make target
    graphviz # For viewing control flow graphs
  ];

  # TODO JOEY Is there a better way to get the clang/<version>/include directory
  # in the include path?
  # NIX_CFLAGS_COMPILE = "-I${clang.cc.lib}/lib/clang/${clang.cc.version}/include";
  # shellHook = ''
  #   NIX_CFLAGS_COMPILE="$NIX_CFLAGS_COMPILE -I${clang.cc.lib}/lib/clang/${clang.cc.version}/include"
  #   #export LIBCLANG_PATH="${llvm.libclang}/lib"
  # '';
}
