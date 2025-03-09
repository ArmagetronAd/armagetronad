from conan import ConanFile

# activate with
# conan install . --build=missing 

class Pkg(ConanFile):
    generators = \
            "AutotoolsToolchain", \
            "PkgConfigDeps", \
            "VirtualRunEnv"

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
        "libcurl/*:shared": False,
        "libcurl/*:static": True
    }

