{ pkgs, ... }:

{
  packages = [
    pkgs.boost
    pkgs.fmt
    pkgs.hyperfine
    pkgs.linuxKernel.packages.linux_zen.perf
    pkgs.mold
  ];
}
