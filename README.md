# Q1 VR Player
Play Quake 1 in VR, no price tags attached.

Requires Oculus Quest glasses and, obviously, Quake 1.

![The tool in its full glory](https://github.com/kbiElude/Q1-VR-Player/blob/main/docs/screenshot.png?raw=true)

This tool could not have been created without existence of the following projects:

- APIInterceptor by Dominik Witczak (hey that's me)
- GLFW by Marcus Geelnard and Camilla Löwy
- ImgUI by Omar Cornut
- LibOVR by Facebook Technologies

# How do I run Quake 1 in VR mode?
1. Install Quake 1. Steam distribution is recommended since it comes with GLQuake attached.
2. Grab GLQuake 0.98 Alpha build. You can find it here: http://www.quaketerminus.com/nqexes/glquake098alpha.zip. Put that binary in Q1 directory.
3. Make sure your Oculus Quest is running in Oculus Link mode.
4. Unpack the latest Q1 VR Player build available (https://github.com/kbiElude/Q1-VR-Player/raw/main/builds/v1.0/Q1VRPlayer_v1_0.zip), any target directory's fine. Run Launcher.exe. Point the tool to the directory where GLQuake.exe lives.
5. Enjoy! If necessary, please use the user interface to calibrate the output.
6. To mitigate Quake 1's aggresive geometry culling, open Q1 console and use "fov 90" command. Sadly, there appears to be no way to do this by passing a command line argument.

# Anything I should know before I start?
While playing, Q1 VR Player hides the original Q1 window and instead opens its own one. However, in the background, it will regularly set focus to the hidden window, in order to work around an issue where libovr mutes out the audio if the audio comes from a window that's not set as foreground. This does not affect gameplay in any way, but could interfere with your work if you try using other windows while Q1 VR Player continues to run in the background.

If you know a non-hacky way to work around this, PRs are more than welcome!

# How do I build this?
The easy way is to grab build.zip, extract it to any directory you wish and just run it.

Another easy way is to just build locally. For Visual Studio 2022, you're going to need to:

1. Create a build directory in the root of the project.
2. Go inside it, fire cmake with --G"Visual Studio 17" -AWin32 ..
3. Open the result .sln file. Build the executable.

You need the executable to be 32-bit because, well, that's what was the only x86 arch around when Q1 was released, hence the funny -AWin32 bit. Don't forget it or you'll be sorry.

# Does this work for modded Quake builds?
No. Any changes to rendering flow would disrupt the heuristics used by the tool.

# Any plans for the future?
Perhaps an OpenXR backend, so that you could play Quake 1 using any compatible VR system, not just Oculus Quests? We'll see.

Linux version is not planned. I have no idea whether Q1 VR Player works with Wine and got no bandwidth available to maintain a dedicated Linux build, sorry.

# Oy, implement feature X for me plox.
See that pull request button? Feel free to propose new features if you fancy :) Same goes for bug-fixes.

# This is awesome. Can I support you in any way?
Donations are always welcome, as I spend way too much on Bandcamp, CDs and vinyls. See https://paypal.me/dominikwitczak1