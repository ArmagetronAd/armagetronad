This release welcomes our new ultrawide monitor using overlords!

And, to be frank, also the old, regular widescreen overlords. 
All this time, the game had been optimized for 4:3 or 5:4 screens, 
with menu text and HUD elements getting stretched to the side for widescreen users.
No more of that! The changes also benefit splitscreen users; 
for a horizontal split, the HUD will now no longer cover half the (split) screen.

And while we were at fixing fonts, 
the default console rendering now tries to display the bitmap font precisely as it is designed, 
pixel by pixel; 
that should make the console more readable and sharper looking for everyone.

Playing back debug recordings has been made simpler and more robust; 
the --playback command line switch is no longer required and the network code should no longer
give up when the recorded server response does not match what the playback code expects. 
Especially, this version should have no problems playing back tournament recordings made with
version 0.2.8.

What Steam users had for a while is now available for everyone: 
The onboarding process has been tweaked a little. The intial game is less frustrating, 
and the tutorial tooltips are spammed less.

Furthermore, compatibility of the generic Linux binaries has been improved, 
with more systems supported out of the box.
