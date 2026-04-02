from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.env import Environment, VirtualRunEnv, VirtualBuildEnv
from conan.tools.files import download, unzip, get, copy
import os

##################
# SDL            #
##################

# created after a suggestion by perplexity.ai

class SdlConan(ConanFile):
    name = "sdl_aa"
    version = "1.2.15"
    description = "SDL 1.2 library (legacy, uses SDL2)"
    settings = "os", "arch", "compiler", "build_type"
    generators = "PkgConfigDeps"
    exports_sources = "CMakeLists.txt", "src/*", "include/*"

    requires = \
            "sdl/[>=2.0.0 <3.0.0]"

    default_options = {
        "sdl/*:shared": True, # sdl_compat uses dynamic loading, so we absolutely need this
    }

    def source(self):
            get(self,
                "https://github.com/libsdl-org/sdl12-compat/releases/download/release-1.2.74/sdl12-compat-1.2.74.tar.gz",
                strip_root=True)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        # Copy dependency libs into local build lib path for configure-time runtime tests
        libs_path = os.path.join(self.build_folder, "lib")
        os.makedirs(libs_path, exist_ok=True)
        for dep_name, dep in self.dependencies.items():
            dirs = dep.cpp_info.libdirs + dep.cpp_info.bindirs
            for dir in dirs:
                for extension in [ "*.so", "*.so.*", "*.dylib", "*.dylib*", "*.dll" ]:
                    copy(self, extension, dir, libs_path)

        # Build-time environment to allow configure checks that execute linked binaries.
        build_env = VirtualBuildEnv(self)
        build_env.environment().prepend_path("LD_LIBRARY_PATH", libs_path)
        build_env.environment().prepend_path("DYLD_LIBRARY_PATH", libs_path)
        build_env.environment().prepend_path("DYLD_FALLBACK_LIBRARY_PATH", libs_path)
        build_env.generate()

        # Runtime package environment as well
        run_env = VirtualRunEnv(self)
        run_env.environment().prepend_path("LD_LIBRARY_PATH", libs_path)
        run_env.environment().prepend_path("DYLD_LIBRARY_PATH", libs_path)
        run_env.environment().prepend_path("DYLD_FALLBACK_LIBRARY_PATH", libs_path)
        run_env.generate()

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
        self.cpp_info.requires = ["sdl::sdl"]
        self.cpp_info.set_property("pkg_config_name", "sdl_aa")
