{
  description = "Tiger VNC";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/master";
  inputs.flake-utils.url = "github:numtide/flake-utils";
  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system: let

      pname = "tigervnc";
      version = let
        cmakeListsContent = builtins.readFile ./CMakeLists.txt;
        versionMatch =
          builtins.match ".*set\\(VERSION ([0-9.]+)\\).*" cmakeListsContent;
      in assert versionMatch != null;
        builtins.elemAt versionMatch 0;
      src = ./.;
      pkgs = import nixpkgs { inherit system; };

      native-build = pkgs.stdenv.mkDerivation {
        inherit pname version src;
        nativeBuildInputs = with pkgs; [
          cmake
          gettext
        ];
        buildInputs = with pkgs; [
          fltk
          gmp
          libjpeg_turbo
          nettle
          pam
          pixman
          xorg.libXi
          zlib
        ];
      };

      windows-build = is64: let
        ming = pkgs.pkgsCross.${if is64 then "mingwW64" else "mingw32"};
        fltk = (ming.fltk.override {
          zlib = zlib;
          libjpeg = libjpeg_turbo;
          withGL = false;
          withCairo = false;
          withDocs = false;
          withShared = false;
        }).overrideAttrs (old: {
          meta = old.meta // {
            # fltk can be built for Windows despite nixpkgs claiming otherwise
            platforms = ming.lib.platforms.all;
          };
        });
        gmp = ming.gmp.override { withStatic = true; };
        libiconv = ming.libiconv.override
          { enableStatic = true; enableShared = false; };
        libintl = confStatic (ming.libintl.override { inherit libiconv; });
        confStatic = pkg: pkg.overrideAttrs (old: {
          configureFlags = (old.configureFlags or [])
                           ++ [ "--disable-shared" "--enable-static" ]; });
        mcfg = confStatic ming.windows.mcfgthreads;
        libjpeg_turbo = (ming.libjpeg_turbo.override {
          enableStatic = true;
          enableShared = false;
          enableJpeg8 = true;
        }).overrideAttrs (old: {
          CFLAGS = [ "-L${mcfg}/lib" ]; });
        nettle = confStatic (ming.nettle.override { gmp = gmp; });
        pixman = ming.pixman.overrideAttrs (old: {
          mesonFlags = (old.mesonFlags or [])
                       ++ [ "-Ddefault_library=static" ]; });
        zlib = ming.zlib.override { shared = false; };
      in ming.stdenv.mkDerivation {
        inherit pname version;
        nativeBuildInputs = with pkgs; [
          cmake
          gettext
        ];
        buildInputs = [
          fltk
          gmp
          libiconv
          libintl
          libjpeg_turbo
          mcfg
          nettle
          pixman
          zlib
        ];
        src = ./.;
        CXXFLAGS = [
          "-static"
        ];
        # Enforce self-contained build. Shared libraries will only be allowed if
        # included in the output. An exception is made for gettext (todo fix).
        allowedReferences = [ libintl ];
      };

      java-build = pkgs.stdenv.mkDerivation {
        pname = "${pname}-java";
        inherit version;
        src = ./java;
        nativeBuildInputs = with pkgs; [
          cmake
          zulu
        ];
        installPhase = ''
          mkdir -p $out/bin
          cp *.jar $out/bin
        '';
      };

    in { packages = {
           default = native-build;
           windows = windows-build true;
           windows32 = windows-build false;
           java = java-build;
         }; }
    );
}
