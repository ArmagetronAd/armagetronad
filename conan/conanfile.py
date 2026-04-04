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
            "zthread_aa/[>=2.0.0 <3.0.0]", \
            "sdl_aa/[>=1.2.15 <2.0.0]", \
            "sdl_image_aa/[>=1.2.12 <2.0.0]", \
            "libcurl/[>=7]", \
            "libxml2/[>=2.9.10]"

    # we need the -config binaries from these dependencies at build time
    build_requires = \
            "zthread_aa/[>=2.0.0 <3.0.0]", \
            "sdl_aa/[>=1.2.15 <2.0.0]"

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

        # modify LD_LIBRARY_PATH for build time to find dependency libraries (this should not be required if we would properly separate tests in configure.ac, like only compose found libraries right at the end)
        build_env.environment().prepend_path("LD_LIBRARY_PATH", libs_path)
        build_env.environment().prepend_path("DYLD_FALLBACK_LIBRARY_PATH", libs_path)

        # modify LD_LIBRARY_PATH
        run_env = VirtualRunEnv(self)
        run_env.environment().prepend_path("LD_LIBRARY_PATH", libs_path) # linux
        run_env.environment().prepend_path("DYLD_FALLBACK_LIBRARY_PATH", libs_path) # macOS

        tc.generate()
        run_env.generate()
        build_env.generate()

