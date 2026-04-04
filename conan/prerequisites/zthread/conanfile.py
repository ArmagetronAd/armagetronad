from conan import ConanFile
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.files import get, patch, replace_in_file
from conan.tools.env import VirtualBuildEnv
import os
import platform

##################
# ZThread        #
##################


# created after a suggestion by perplexity.ai

class ZThreadConan(ConanFile):
    name = "zthread_aa"
    version = "2.3.2"
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

        # steal patches from debian
        get(self, "http://archive.ubuntu.com/ubuntu/pool/universe/z/zthreads/zthreads_2.3.2-11.1build1.debian.tar.xz", strip_root=False)
        files = os.listdir(os.path.join(self.source_folder, "debian/patches"))
        files.sort()
        for file in files:
            if file.endswith(".patch"):
                print(f"Applying patch: {file}")
                patch(self, patch_file=os.path.join("debian/patches", file), strip=1)

        # correct sed syntax
        replace_in_file(self, os.path.join(self.source_folder, "configure"), "[:space:]", "[[:space:]]")

    def generate(self):
        tc = AutotoolsToolchain(self)

        build_env = VirtualBuildEnv(self)

        # this lib uses c++ methods deprecated in c++11 and removed in c++17
        build_env.environment().define("CXXFLAGS", "--std=c++11 -fpermissive -DPTHREAD_MUTEX_RECURSIVE_NP=PTHREAD_MUTEX_RECURSIVE " + os.environ.get("CXXFLAGS", ""))

        # hack to avoid complaints
        build_env.environment().define("MISSING", "true")

        # Provide explicit build/host system types to work around config.guess issues in nix
        gnu_triple = self._get_gnu_triple()
        tc.configure_args.extend([
            f"--build={gnu_triple}",
            f"--host={gnu_triple}",
            f"--prefix={self.package_folder}",
            "--enable-shared=false",
        ])

        tc.generate()
        build_env.generate()

    def build(self):
        autotools = Autotools(self)
        autotools.configure()
        autotools.make()

    def _make_install(self):
        """Manually run make install without DESTDIR."""
        import os
        # Run make install directly, inheriting environment
        cmd = f"cd {self.build_folder} && make install"
        result = os.system(cmd)
        if result != 0:
            raise RuntimeError(f"make install failed with code {result}")

    def package_info(self):
        self.cpp_info.libs = ["zthread"]
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.set_property("pkg_config_name", "zthread")

    def package(self):
        autotools = Autotools(self)
        # Don't use DESTDIR since zthread applies it inconsistently
        # Prefix is already set at configure time to self.package_folder
        self._make_install()

