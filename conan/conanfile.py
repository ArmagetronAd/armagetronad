from conan import ConanFile
from conan.tools.files import copy
from conan.tools.gnu import PkgConfigDeps
from conan.tools.env import VirtualRunEnv, VirtualBuildEnv
import os

# activate with
# ./conan_install.sh

class Pkg(ConanFile):
    generators = \
            "AutotoolsToolchain"


    requires = \
            "none_aa/0.0.1", \
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
        venv = VirtualBuildEnv(self)
        venv.generate()

        pc = PkgConfigDeps(self)
        pc.generate()

        # Copy none.pc from the dummy package
        none_pkg = self.dependencies["none_aa"]
        none_pc = os.path.join(none_pkg.package_folder, "lib", "pkgconfig", "none.pc")
        copy(self, "none.pc", os.path.dirname(none_pc), self.build_folder)

        # copy libraries
        libs_path = os.path.join(self.build_folder, "lib")
        for dep_name, dep in self.dependencies.items():
            dirs = dep.cpp_info.libdirs + dep.cpp_info.bindirs
            for dir in dirs:
                for extension in [ "*.so.*", "*.dylib*", "*.dll" ]:
                    copy(self, extension, dir, libs_path)

        # modiy LD_LIBRARY_PATH
        run_env = VirtualRunEnv(self)
        run_env.environment().append_path("LD_LIBRARY_PATH", libs_path)
        run_env.generate()

