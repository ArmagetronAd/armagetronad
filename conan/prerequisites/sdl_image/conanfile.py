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
            "libjpeg/[9f]", \
            "libpng/[>=1.0.0 <2.0.0]"

    default_options = {
    }

    def source(self):
        git = Git(self)
        git.clone(url="https://github.com/libsdl-org/SDL_image.git", target=".")
        git.checkout("7c6ea40bb75262740cd07f7658bc543f13c65b3c")

        #get(self,"https://www.libsdl.org/projects/old/SDL_image/release/SDL_image-1.2.12.tar.gz",strip_root=True)

    def generate(self):
        tc = AutotoolsToolchain(self)

        build_env = VirtualBuildEnv(self)
        run_env = VirtualRunEnv(self)

        # hack in dependency library paths
        lib_paths = [lib for _, dep in self.dependencies.items() for lib in dep.cpp_info.libdirs]

        rpath_statements = ["-Wl,-rpath," + lib for lib in lib_paths]
        build_env.environment().define("LDFLAGS", os.environ.get("LDFLAGS", "") + " " + " ".join(rpath_statements))

        # set SDL prefix explicitly to avoid finding system SDL
        sdl = self.dependencies["sdl_aa"]
        tc.configure_args.extend([
            "--with-sdl-prefix=" + sdl.package_folder,
            "--disable-imageio", # this is macOS specific and does not compile
        ])

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

