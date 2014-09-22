Important Notice
================

This document will only be of interest to people who want to recompile twscript,
or use this package as the base from which they can develop their own scripts.

If all you want to do is *use* twscript, please see the TWScript.md document,
you can safely ignore this one!


Compiling or using this package as the base for your scripts
============================================================

If you are interested in using this package as the base from which you can
develop your own scripts, or just want to bugfix and recompile twscript,
you will need to do rather more work than a normal Dromed user. You will
need to have at least the following installed:

1. a working compiler setup (and editor to code in)
2. git (the distributed version control system) to fetch the library code.
2. the supporting libraries
3. the twscript code.

what follows is a walkthrough of how to get the above set up, and how to get
to the point where you can actually compile scripts.

1. Setting up the compiler
--------------------------

Before you can do any compiling, you need a compiler. This is actually not as
straightforward as you might think: as even if you already have a compiler
available, there's a good chance it may not actually work with this code, or it
may produce an .osm that Thief can't actually load (or it may work fine, in
which case, great! But don't blame me if you end up up to your armpits in
soul-devouring, tentacled horrors from beyond time and space when trying it).
Hence, you are strongly advised to follow these steps, even if you already
have a working compiler setup, unless you're feeling adventurous:

0. Download the TDM-GCC compiler suite for Windows from the [project site](http://tdm-gcc.tdragon.net/).
   You want to click on the "TDM32 bundle" (currently 3.8.1), this will take
   you to a sourceforge download page to download a `tdm-gcc-<version>.exe` file.
1. Run the `tdm-gcc-<version>.exe` installer:
    - Click "Create"
    - Select "MinGW/TDM (32-bit), click "Next"
    - Set the installation directory, click "Next"
    - Unless you have some preferred sourceforge mirror, click "Next" again.
    - Review the selected components, you shouldn't need to change any. Click
      "Install".
    - Go look at lolcats or something for a bit.
2. From the Start Menu, select MinGW32 -> MinGW Command Prompt
3. When the command prompt appears, type the following: `gcc -v`. If all goes
   well, you should see several lines of information output, ending in
   `gcc version X.X.X (tdm-2)`.

This is enough to get you a working compiler, but the toolset included is very
limited, I generally suggest installing MSYS as well:

0. Download the latest version of the `mingw-get-setup.exe` installer from
   [sourceforge](http://sourceforge.net/projects/mingw/files/Installer//)
1. Run the installer
2. Click 'Install'
3. Change the "Installation directory" to the directory you installed TDM-GCC
   to (usually `C:\TDM-GCC-32` with the default settings).
3. Click 'Continue' (you should be able to leave all the options on defaults)
4. Wait while stuff is downloaded
5. Eventually the downloads will complete, then click 'Continue'
6. The "MinGW Installation Manager" should now open
7. Click on the checkbox next to "msys-base' and select "Mark for installation"
11. Click on "All Packages" in the left list of the "MinGW Installation Manager"
12. Locate "msys-mintty" in the list on the right. It should be about three
    quarters of the way down the list.
13. Click on the checkbox next to "msys-mintty' and select "Mark for installation"
14. From the "Installation" menu, select "apply changes"
15. Click "Apply"
16. Wait while packages are downloaded and installed, then click "close"

It is possible to skip steps 12 and 13, but the default command prompt for msys
uses the standard Windows command line tool, which is dreadful. mintty is far
more usable and fully-featured.

For some reason best - perhaps only - known to the MinGW maintainers, the
MinGW Installation Manager does not appear to create useful start menu
shortcuts. To do this:

0. Using Windows Explorer, locate your msys directory. If you have followed the
   steps above, this should be `C:\TDM-GCC-32\msys\1.0`
1. The directory should contain a file `msys.bat`.
2. Select the `msys.bat` file, right click, and select "Create shortcut" from
   the popup menu.
3. Select the shortcut and open its properties.
4. In the "Target:" field, add ` -mintty` to the end, so it reads something
   like `C:\TDM-GCC-32\msys\1.0\msys.bat -mintty`
5. In the "Start in:" field, add `\bin` to the end, so it reads something
   like `C:\TDM-GCC-32\msys\1.0\bin`
6. Apply the changes, and okay the properties window to close it.
7. Drag the shortcut into the Windows start menu
8. Run the `msys.bat - shortcut` from the start menu, after a brief delay
   a terminal prompt should open
9. run `make`; the terminal should respond with "make: *** No targets specified and no makefile found.  Stop."
10. run `gcc -v`; you should get the same output as before.

At this point, you should have a working compiler, and a decent set of tools
installed.

2. Setting up an editor
-----------------------

I'm not going to tell you to use one editor or another here - editor choice has
been the subject of Holy Wars for decades, and almost everyone has their
favorite. If you don't have a decent programmer's editor installed, you can
go far worse than to start off with [Notepad++](http://notepad-plus-plus.org/):
it's fast, light, does syntax highlighting and block folding. Apparently
TDM-GCC plays nice with [Code::Blocks](http://www.codeblocks.org/),
so you might want to look at that. I tend to use Emacs via
[Cygwin](http://www.cygwin.com/), but I'm also reliably informed that I'm
insane, so YMMV with that. If you use an IDE like Code::Blocks, **do not**
expect to be able to use the IDE's build process to compile the scripts:
you are going to need to use make, and wrangle makefiles, to get them to
build.

3. Setting up git
-----------------

To actually get the code, you need to install git. If you already have git
(or github's native tool) installed, you can skip this step.

0. Close any MinGW windows you may have opened when setting up the compiler.
1. Download the latest installer from http://git-scm.com/downloads
2. Run the installer, clicking "Next" through it until you come to the
   "Adjusting your PATH environment" page. Select the "Run Git from the
   Windows Command Prompt" option, and click "Next"
3. Leave the "Choosing the SSH executable" option on "Use OpenSSH".
   **Do not select "Use (Tortoise)Plink"**, even if you have it installed
   and working. Enabling that option can, and usually will, introduce problems.
4. Keep clicking "Next" until it starts installing.
5. Twiddle your thumbs for a while.
7. Once the install has finished, run Git -> Git Bash from the Start Menu.
   If all goes well, the Git Bash window should open, and you can use
   `git --version` to confirm that git is working.

Before continuing, you may want to become familiar with the basics of using
Git. A good place to start is the online [Pro Git Book](http://git-scm.com/book).

4. Getting the code
-------------------

Now you should be in the position where you can actually start to fetch code
to work on. All the code is on github, so you need to clone copies of it
with git (you may wish to fork the twscript project before doing so, if you
want to push your changes back to github, but that's up to you).

0. Create a directory somewhere on your system, something like `D:\thiefscripts`
   It doesn't matter what you call it, provided that you can remember it, and it
   **does not** contain spaces. Spaces in filenames are another way to summon
   those soul-devouring, tentacled horrors, so avoid them: CamelCase or underscore_names
   are your friend.
1. From the start menu, select Git -> Git Bash
2. Change into the directory you created in step 0, using unix-like syntax, eg:
   `cd /d/thiefscripts`
3. Enter the following commands to clone Telliamed's script support libraries:

        git clone https://github.com/whoopdedo/lg.git
        git clone https://github.com/whoopdedo/scriptlib.git

4. And this one to clone this project (if you have forked it, clone your
   fork instead!):

        git clone https://github.com/TheWatcher/twscript.git

5. You'll probably also want to grab Telliamed's Public Scripts package itself,
   so that you can use the various script implementations it contains as a
   reference. You can either look at it [on the web](https://github.com/whoopdedo/publicscripts)
   at github, or clone a local copy:

        git clone https://github.com/whoopdedo/publicscripts.git

You should now have three or four directories inside the one you created in
step 0 above: lg, scriptlib, twscript, and possibly publicscripts. Now
to compile them...

5. Compiling the libraries
--------------------------

Before you can actually create any scripts, you need to have compiled the
support libraries your scripts will need to use in order to communicate
with the Dark engine. This step should be pretty straightforward at this
point. Henceforth, for simplicity, I am going to assume that all the projects
you cloned in section 4 are inside the `D:\thiefscripts` directory as
discussed there - if you cloned them elsewhere, you must update the paths
accordingly.

First you need to fix up a few issues with the standard libraries as cloned:

0. Optional step: open `D:/thiefscripts/lg/lg/config.h` in your text editor,
   go to line 24 and add `#undef __thiscall` to that line, eg:

        #else // !_MSC_VER
        #undef __thiscall
        #define __thiscall

   and save the changes. If you don't do this, you'll get a redefinition
   warning from almost every compile, and it gets irritating quickly.
1. Open `D:/thiefscripts/lg/lg/types.h` in your text editor, go to line
   195 and change `static const float epsilon = 0.00001f` to read
   `static constexpr float epsilon = 0.00001f`  Save the changes.
2. Open `D:/thiefscripts/lg/Makefile` in your text editor, go to line 24,
   and change

        `CXXFLAGS = -W -Wall -Wno-unused-parameter -masm=intel $(INCLUDEDIRS)`

   to be

        `CXXFLAGS = -std=gnu++0x -W -Wall -Wno-unused-parameter -masm=intel $(INCLUDEDIRS)`

   Save the changes.
3. Open `D:/thiefscripts/scriptlib/Makefile` in your text editor, go to
   line 45, and change

        `CXXFLAGS = -W -Wall -masm=intel`

   to read

        `CXXFLAGS = -std=gnu++0x -W -Wall -masm=intel`

   Save the changes.

Now you can start compiling:

0. Start a MinGW Shell window
1. Change into the lg directory, eg: `cd /d/thiefscripts/lg`
2. Run `make`
3. Change into the scriptlib directory, eg: `cd ../scriptlib`
4. Run `make`

6. Test compiling TWScript
--------------------------

At this point, all the libraries required to compile scripts should be in place
and compiled, so it is time to actually give it a go. Before you start working
on any scripts of your own, you should make sure your environment is sane by
compiling the TWScript package:

0. If a MinGW Shell window isn't current open, start one.
1. Change into the twscript directory: `cd /d/thiefscripts/twscript`
2. Run `make`
3. Once make has finished, copy twscript.osm into your Thief 2 installation.
4. Start Dromed, add an object, set it up to use TWTweqSmooth, go into game
   mode, and see if it works. If it does, you have successfully compiled the
   osm!

7. Writing your own scripts
---------------------------

If you get here without problems, you are ready to start making your own
scripts (and your problems are literally just beginning...).

Updated instructions here soon.

8. Important note about member variables
----------------------------------------

When writing scripts for the dark engine you should be very careful when using
'non persistent' member variables to maintain cross-message state (that is,
storing state that persists between messages in normal member variables rather
than using the persistent script data store via `set_script_data()`,
`get_script_data()`, etc). Every time a metaproperty is added to an object, the
list of scripts attached to the object is replaced: already attached script
instances are detached and destroyed, and new copies are added in their place.
This means that, every time a metaprop is added, you will lose any state held
in member variables.

Either avoid using member variables to maintain state and use the persistent
script data store to do it, or ensure that your scripts are able to restore
their state through other means.
