from conan import ConanFile
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.files import download, unzip, get, copy
import os

# created after a suggestion by perplexity.ai

class SdlConan(ConanFile):
    name = "aa_libsdl"
    version = "1.2.15"
    description = "SDL 1.2 library"
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "PkgConfigDeps", "VirtualBuildEnv"

    def source(self):
            get(self,
                "https://libsdl.org/release/SDL-1.2.15.tar.gz",
                strip_root=True)

    def generate(self):
        tc = AutotoolsToolchain(self)
        tc.generate()

    def build(self):
        autotools = Autotools(self)
        autotools.configure()
        autotools.make()

    def package_info(self):
        self.cpp_info.libs = ["SDL"]
        self.cpp_info.includedirs = ["include/SDL"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.bindirs = ["bin"]
        self.cpp_info.set_property("pkg_config_name", "sdl")

    def package(self):
        autotools = Autotools(self)
        autotools.install()

    def export_sources(self):
        copy(self, "patches/*", self.recipe_folder, self.export_sources_folder)

