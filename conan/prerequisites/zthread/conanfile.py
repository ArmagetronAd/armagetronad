from conan import ConanFile
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.files import download, unzip, get, copy, patch
from conan.tools.env import Environment, VirtualRunEnv, VirtualBuildEnv
from conan.tools.scm import Git
import os
import platform

##################
# ZThread        #
##################


# created after a suggestion by perplexity.ai

class ZThreadConan(ConanFile):
    name = "zthread_aa"
    version = "1.2.12"
    description = "ZThread library"
    settings = "os", "arch", "compiler", "build_type"
    generators = "PkgConfigDeps"

    default_options = {
    }

    def _get_gnu_triple(self):
        """Construct GNU triple (cpu-vendor-os) for use with autotools configure."""
        
        os_map = {
            "Linux": "linux-gnu",
            "Macos": "apple-darwin",
            "Windows": "w64-mingw32",
            "FreeBSD": "freebsd",
        }
        
        cpu = self.settings.arch
        os_name = os_map.get(str(self.settings.os), str(self.settings.os))
        
        return f"{cpu}-{os_name}"

    def source(self):
        get(self,"https://sourceforge.net/projects/zthread/files/ZThread/2.3.2/ZThread-2.3.2.tar.gz",strip_root=True)
        #download(self, "https://forums3.armagetronad.net/download/file.php?id=9628", "patches/zthread-2.3.2-fix-cmake.patch")
        #patch(self, "patches/zthread-2.3.2-fix-cmake.patch")
    def generate(self):
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

        # Provide explicit build/host system types to work around config.guess issues in nix
        gnu_triple = self._get_gnu_triple()
        tc.configure_args.extend([
            f"--build={gnu_triple}",
            f"--host={gnu_triple}",
        ])

        env.save_script("zthread_env")

        tc.generate()
        run_env.generate()
        build_env.generate()

    def build(self):
        autotools = Autotools(self)
        autotools.configure()
        autotools.make()

    def package_info(self):
        self.cpp_info.libs = ["zthread"]
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.set_property("pkg_config_name", "zthread")

    def package(self):
        autotools = Autotools(self)
        autotools.install()

    def export_sources(self):
        copy(self, "patches/*", self.recipe_folder, self.export_sources_folder)

