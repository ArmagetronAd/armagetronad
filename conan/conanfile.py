from conan import ConanFile
from conan.tools.files import save, copy
from conan.tools.gnu import PkgConfigDeps, AutotoolsToolchain
from conan.tools.env import VirtualRunEnv, VirtualBuildEnv, Environment
import os

##################
# ArmagetronAd   #
##################

# activate with
# ./conan_install.sh

class Pkg(ConanFile):

    requires = \
            "sdl_aa/[>=1.2.15 <2.0.0]", \
            "sdl_image_aa/[>=1.2.12 <2.0.0]", \
            "libcurl/[>=7]", \
            "libxml2/[>=2.9.10]"
    
    default_options = {
        "sdl/*:shared": True, # sdl_compat uses dynamic loading
        "libcurl/*:with_ssl": False,
        "libcurl/*:with_https": False,
        "libcurl/*:with_ftp": False,
        "libcurl/*:with_file": False,
        "libcurl/*:with_rtsp": False,
        "libcurl/*:with_dict": False,
        "libcurl/*:with_telnet": False,
        "libcurl/*:with_tftp": False,
        "libcurl/*:with_pop3": False,
        "libcurl/*:with_imap": False,
        "libcurl/*:with_smtp": False,
        "libcurl/*:with_gopher": False,
        "libcurl/*:shared": True,
        "libcurl/*:static": False,
        "libxml2/*:html": False,
        "libxml2/*:http": False,
        "libxml2/*:ftp": False,
        "libxml2/*:zlib": False,
        "libxml2/*:iconv": False,
        "libxml2/*:shared": True
    }

    keep_imports = True

    def generate(self):
        pc = PkgConfigDeps(self)
        pc.generate()

        # Create none.pc to complete SDL2 config
        pc_content = """\
Name: none
Description: Dummy none backend for SDL2
Version: 0.0.1
Libs: 
Cflags: 
        """
        save(self, os.path.join(self.build_folder, "none.pc"), pc_content)

        #none_pkg = self.dependencies["none_aa"]
        #none_pc = os.path.join(none_pkg.package_folder, "lib", "pkgconfig", "none.pc")
        #copy(self, "none.pc", os.path.dirname(none_pc), self.build_folder)

        tc = AutotoolsToolchain(self)

        # copy libraries
        libs_path = os.path.join(self.build_folder, "lib")
        os.makedirs(libs_path, exist_ok=True)
        print(libs_path)
        for dep_name, dep in self.dependencies.items():
            dirs = dep.cpp_info.libdirs + dep.cpp_info.bindirs
            print(dirs)
            for dir in dirs:
                for extension in [ "*.so", "*.so.*", "*.dylib", "*.dylib*", "*.dll" ]:
                    copy(self, extension, dir, libs_path)

        build_env = VirtualBuildEnv(self)
        build_env.environment().prepend_path("LD_LIBRARY_PATH", libs_path)
        build_env.environment().prepend_path("DYLD_LIBRARY_PATH", libs_path)
        build_env.environment().prepend_path("DYLD_FALLBACK_LIBRARY_PATH", libs_path)

        # modiy LD_LIBRARY_PATH
        run_env = VirtualRunEnv(self)
        run_env.environment().prepend_path("LD_LIBRARY_PATH", libs_path)
        run_env.environment().prepend_path("DYLD_LIBRARY_PATH", libs_path)
        run_env.environment().prepend_path("DYLD_FALLBACK_LIBRARY_PATH", libs_path)

        # hack in dependency library paths
        bin_paths = [lib for _, dep in self.dependencies.items() for lib in dep.cpp_info.bindirs]
        lib_paths = [lib for _, dep in self.dependencies.items() for lib in dep.cpp_info.libdirs]
        include_paths = [lib for _, dep in self.dependencies.items() for lib in dep.cpp_info.includedirs]
        include_statements = [" -I" + inc for inc in include_paths]
        lib_statements = [" -L" + lib for lib in lib_paths]
        pkg_config_paths = [os.path.join(lib, "pkgconfig") for lib in lib_paths]

        env = Environment()
        env.define("CPPFLAGS", os.environ.get("CPPFLAGS", "") + " " + " ".join(include_statements))

        rpath_statements = ["-Wl,-rpath," + lib for lib in lib_paths]
        env.define("LDFLAGS", os.environ.get("LDFLAGS", "") + " " + " ".join(lib_statements + rpath_statements))

        env.append_path("LIBRARY_PATH", os.pathsep.join(lib_paths))
        env.append_path("LD_LIBRARY_PATH", os.pathsep.join(lib_paths))
        env.append_path("PKG_CONFIG_PATH", os.pathsep.join(pkg_config_paths))
        env = env.vars(self, scope="build")

        build_env.environment().define("CPPFLAGS", os.environ.get("CPPFLAGS", "") + " " + " ".join(include_statements))
        build_env.environment().define("LDFLAGS", os.environ.get("LDFLAGS", "") + " " + " ".join(lib_statements + rpath_statements))
        build_env.environment().append_path("PATH", os.pathsep.join(bin_paths))

        # set SDL prefix explicitly to avoid finding system SDL
        sdl = self.dependencies["sdl_aa"]
        #print(sdl.cpp_info.includedirs)
        #exit(1)

        #png = self.dependencies["libpng"]
        tc.configure_args.extend([
            "--with-sdl-prefix=" + sdl.package_folder,
            "--disable-imageio",
        ])

        env.save_script("aa_env")

        tc.generate()
        run_env.generate()
        build_env.generate()

