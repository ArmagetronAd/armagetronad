This is from the 0.2.9 branch of development. Our current releases are derived from here.

## It's all libxml's fault

The main reason for this release is that the default installation of libxml
removed http fetch functions we were using, and that starts appearing in 
mainstream Linux distributions now, such as Ubuntu 26.10. On such systems,
we now use libcurl as a replacement. Binary builds are unchanged, they still
use the old libxml.

But hey, you also get a better framerate indicator.

