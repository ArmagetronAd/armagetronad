# dummy package to satisfy sdl2's bogus 'none' dependency

from conan import ConanFile
from conan.tools.files import save
import os

class NoneConan(ConanFile):
    name = "none_aa"
    version = "0.0.1"
    generators = "PkgConfigDeps"

    def package(self):
        pkgconfig_dir = os.path.join(self.package_folder, "lib", "pkgconfig")
        os.makedirs(pkgconfig_dir, exist_ok=True)

        pc_content = """\
prefix={prefix}
libdir=${{prefix}}/lib
includedir=${{prefix}}/include

Name: none
Description: Dummy none backend for SDL2
Version: 0.0.1
Libs: 
Cflags: 
        """.format(prefix=self.package_folder)

        save(self, os.path.join(pkgconfig_dir, "none.pc"), pc_content)

    def package_info(self):
        # Tell Conan to add this pkg-config path so `pkg-config none` is found
        pkgconfig_path = os.path.join(self.package_folder, "lib", "pkgconfig")
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.set_property("pkg_config_name", "none")


