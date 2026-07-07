# Conan

If you wish to use conan versions of some of our dependences
(libxml2 and curl at the moment), invoke

```
./conan/prerequisites/build.sh # may only be needed once
conan build ./conan/conanfile.py --build=missing
```

before calling the configure script.

