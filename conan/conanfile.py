from conan import ConanFile
from conan.tools.files import copy
from conan.tools.env import VirtualRunEnv
import os

# activate with
# conan install . --build=missing 

class Pkg(ConanFile):
    generators = \
            "AutotoolsToolchain", \
            "PkgConfigDeps"

    requires = \
            "libcurl/[>=7]", \
            "libxml2/[>=2.9.10]"

    default_options = {
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

