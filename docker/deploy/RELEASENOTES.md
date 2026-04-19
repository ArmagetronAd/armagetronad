This is from the 0.2.9 branch of development. Our current releases are derived from here.

## It's all libxml's fault

The main reason for this release is that the default installation of libxml
removed http fetch functions we were using, and that starts appearing in 
mainstream Linux distributions now, such as Ubuntu 26.10. On such systems,
we now use libcurl as a replacement. Binary builds are unchanged, they still
use the old libxml.

But hey, you also get a better framerate indicator. And it seems the bugs 
Alex Bagnall fixed made the bots a little more competent.

There is a new framerate limiter option in 
System Setup/Display Settings/Screen Mode. We decided to default it to 360 FPS, 
which should be inoffensive. [This article](/blog/2026/04/18/framerate-limit) 
describes how you can set it up to benefit you even more. And of course, you 
can completely disable it.
