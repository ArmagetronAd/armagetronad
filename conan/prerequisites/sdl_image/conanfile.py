from conan import ConanFile
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.files import download, unzip, get, copy
from conan.tools.env import Environment, VirtualRunEnv, VirtualBuildEnv
from conan.tools.scm import Git
import os

##################
# SDL_image      #
##################


# created after a suggestion by perplexity.ai

class SdlImageConan(ConanFile):
    name = "sdl_image_aa"
    version = "1.2.12"
    description = "SDL_image 1.2 library"
    settings = "os", "arch", "compiler", "build_type"
    generators = "PkgConfigDeps"

    requires = \
            "sdl_aa/[>=1.2.10 <2.0.0]", \
            "libpng/[>=1.0.0 <2.0.0]"

    default_options = {
    }

    def source(self):
        git = Git(self)
        git.clone(url="https://github.com/libsdl-org/SDL_image.git", target=".")
        git.checkout("7c6ea40bb75262740cd07f7658bc543f13c65b3c")

        # get(self,"https://www.libsdl.org/projects/old/SDL_image/release/SDL_image-1.2.12.tar.gz",strip_root=True)

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

        # set SDL prefix explicitly to avoid finding system SDL
        sdl = self.dependencies["sdl_aa"]
        #print(sdl.cpp_info.includedirs)
        #exit(1)

        #png = self.dependencies["libpng"]
        tc.configure_args.extend([
            "--with-sdl-prefix=" + sdl.package_folder,
            "--disable-imageio",
        ])

        env.save_script("sdl_env")

        tc.generate()
        run_env.generate()
        build_env.generate()

    def build(self):
        autotools = Autotools(self)
        autotools.configure()
        autotools.make()

    def package_info(self):
        self.cpp_info.libs = ["SDL_image"]
        self.cpp_info.includedirs = ["include/SDL"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.set_property("pkg_config_name", "SDL_image")

    def package(self):
        autotools = Autotools(self)
        autotools.install()

    def export_sources(self):
        copy(self, "patches/*", self.recipe_folder, self.export_sources_folder)

