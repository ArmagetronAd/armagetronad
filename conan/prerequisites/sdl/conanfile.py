from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import download, unzip, get, copy
import os

# created after a suggestion by perplexity.ai

class SdlConan(ConanFile):
    name = "sdl_aa"
    version = "1.2.15"
    description = "SDL 1.2 library (legacy, uses SDL2)"
    settings = "os", "arch", "compiler", "build_type"
    generators = "PkgConfigDeps", "VirtualBuildEnv"

    requires = \
            "sdl/[>=2.0.0 <3.0.0]"

    default_options = {
        "sdl/*:shared": True, # sdl_compat uses dynamic loading
        "libxml2/*:html": False,
        "libxml2/*:http": False,
        "libxml2/*:ftp": False,
        "libxml2/*:zlib": False,
        "libxml2/*:iconv": False,
        "libxml2/*:shared": True
    }

    def source(self):
            get(self,
                "https://github.com/libsdl-org/sdl12-compat/releases/download/release-1.2.74/sdl12-compat-1.2.74.tar.gz",
                strip_root=True)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["SDL"]
        self.cpp_info.includedirs = ["include/SDL"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.bindirs = ["bin"]
        self.cpp_info.set_property("pkg_config_name", "sdl")
