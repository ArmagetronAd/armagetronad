# Conan

Usually, you should build this game against the libraries
your system provides. However, some situations call for custom
build dependencies, like building a lean standalone version
without external dependencies. Then you want to have the
dependencies in a configuration where they are stripped of
all the stuff we do not need.

Conan is a system that can provide these builds, if properly
configured.

## Configure it Properly

It's best to set up conan in a new directory somewhere (here is fine):

```
export CONAN_SYSREQUIRES_MODE=verify
export CONAN_HOME=$PWD/.conan-home
mkdir -p "${CONAN_HOME}"
```

Then let your profile autodetect and add the configuration from `conanoptions.ini`:

```
conan profile detect --force
cat conanoptions.ini >> ${CONAN_HOME}/profiles/default
```

You can try with any other profile, of course; the one created above is
quite fat free and should work, vanilla profiles might not. We have to
compile some stuff that is not really prepared for conan.

## Build Prerequisites

We need to build and install our own version of SDL and SDL_Image, they're
too old to be included in the standard library. Just invoke

`./prerequisites/build.sh`

to build them and

`./prerequisites/clean.sh`

to erase them again.

## Activate Conan

in this directory, invoke

```
conan build ./conanfile.py --build=missing
```

This will now also build `libcurl` and `libxml2`.

before calling the configure script. With conan configured, it will pick up
the dependencies from here.

