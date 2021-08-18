This is from the 0.2.8.3 branch of development. 
It is the most recent LTS (Long Term Support) branch.
While it is the most recent LTS branch, 
it will receive security fixes, adaptions to new
compilers and Linux versions and some less important bugfixes.

Target audience for this series are mostly Linux distributors who do not want 
to upgrade to 0.2.9 just yet.
If you are in that camp and need help integrating the changes into your build, 
contact us.

0.2.8.3.6 is the first release using the build system that powers the 0.2.9 series. 
It should bring more consistency between this and future releases, 
but as newer versions of autotools are now used, differences to 0.2.8.3.5 in the tarball
are to be expected.

Security related: One fix for a remotely exploitable use after free was backported from 0.2.9 (#34) and one
client crash exploitable by making you play back a specially prepared recording (#54) was fixed.

0.2.8.3.6.1 fixes silly build system mistakes and adds ppa support for Ubuntu 21.04 and 21.10.
