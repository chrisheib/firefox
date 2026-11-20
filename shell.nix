{
  pkgs ? import <nixpkgs> { },
}:

let
  # Mozilla bevorzugt LLVM/Clang für den Core-Build
  stdenv = pkgs.clangStdenv;
  runtimeLibraryPath = pkgs.lib.makeLibraryPath (
    (map (input: input.lib or input.out) pkgs.firefox-unwrapped.buildInputs) ++ [ pkgs.gdk-pixbuf ]
  );
in
pkgs.mkShell.override { inherit stdenv; } {
  # Holt alle Basis-Bibliotheken (GTK, PulseAudio, Pango etc.)
  inputsFrom = [ pkgs.firefox-unwrapped ];

  nativeBuildInputs = with pkgs; [
    # Compiler & Build-Tools
    clang
    llvm
    rustc
    cargo
    rust-cbindgen
    nodejs # Wird für einige Build-Skripte im Core benötigt
    watchman # Beschleunigt die Erkennung von Dateiänderungen

    # System-Tools
    pkg-config
    gnumake
    python3
    m4
    git

    # Debugging & IDE-Support
    gdb
    lldb
    rust-analyzer # Unverzichtbar für Rust-Code-Completion in VS Code/Neovim
  ];

  # Wichtige Umgebungsvariablen für den Compiler-Linker
  shellHook = ''
    export MOZ_NIX_SHELL=1
    export HOST_CC=clang
    export HOST_CXX=clang++
    export CC=clang
    export CXX=clang++
    export PATH="${pkgs.rustc}/bin:${pkgs.cargo}/bin:$PATH"
    export AS=clang
    export CXXFLAGS="-I$HOME/.mozbuild/sysroot-x86_64-linux-gnu/usr/include/freetype2 $CXXFLAGS"

    obj_library_path=
    for obj_dist_bin in "$PWD"/obj-*/dist/bin; do
      [ -d "$obj_dist_bin" ] && obj_library_path="$obj_library_path:$obj_dist_bin"
    done
    export LD_LIBRARY_PATH="''${obj_library_path#:}:${runtimeLibraryPath}''${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

    echo "=========================================================="
    echo "Firefox core development shell loaded (Clang/Rust)"
    echo " Debuggers: gdb, lldb"
    echo " Language server: rust-analyzer"
    echo "=========================================================="
  '';
}
