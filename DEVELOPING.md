

Using this as the base for your scripts
=======================================

If you are interested in using this package as the base from which you can
develop your own scripts, you will need to do rather more work than a normal
Dromed user. You will need to have at least the following installed:

1. a working compiler setup (and editor to code in)
2. git (the distributed version control system) to fetch the library code.
2. the supporting libraries
3. the twscript code.

what follows is a walktrhough of how to get the above set up, and how to get
to the point where you can actually compile scripts.

1. Setting up the compiler
--------------------------

Before you can do any compiling, you need a compiler. This is actually not as
straightfoward as you might think: as even if you already have a compiler
available, there's a good chance it may not actually either work with this code,
or produce an .osm that Thief will actually load (it may work fine, in which
case all the better, but don't blame me if you end up surrounded by betentacled
horrors from beyond time and space while testing it). Hence, you are strongly
advised to follow these steps, even if you already have a working compiler
setup.

0. Download the TDM-GCC compiler suite for Windows from the [project site](http://tdm-gcc.tdragon.net/).
   You want to get the `tdm-gcc-x.x.x` bundle installer (MinGW/sjlj), currently
   at version 4.6.1 available from [this link](http://sourceforge.net/projects/tdm-gcc/files/TDM-GCC%20Installer/tdm-gcc-4.6.1.exe/download)
   (note that I have not tried using the tdm64-gcc installer, and I doubt the
   generated .osm would work. Feel free to experiment!)
1. Run the `tdm-gcc-x.x.x` installer:
    - Click "Create"
    - Select "MinGW/TDM (32-bit), click "Next"
    - Set the installation directory, click "Next"
    - Unless you have some preferred sourceforge mirror, click "Next" again.
    - Review the selected components, you shouldn't need to change any. Click "Install".
    - Go look at lolcats or something for a bit.
2. From the Start Menu, select MinGW32 -> MinGW Command Prompt
3. When the command prompt appears, type the following: `gcc -v`. If all goes well, you
   should see several lines of information output, ending in `gcc version X.X.X (tdm-1)`.

This is enough to get you a working compiler, but the toolset included is very limited,
I generally suggest installing MSYS as well:

0. Download the latest version of the `mingw-get-inst` installer from
   [sourceforge](http://sourceforge.net/projects/mingw/files/Installer/mingw-get-inst/)
1. Run the installer
2. Click next until the installer reaches the "Select Destination Location" step, set the
   location to the path you installed tdm-gcc to (for example, `C:\MinGW-TDM`
3. On the Select Components step **YOU MUST UNTICK "C Compiler"**. *Do not* attempt to
   install the standard MinGW compiler here, or you will encounter all sorts of problems
   (remember those betentacled horrors? Yes, those sorts of problems). Scroll down the
   list of components, find "MSYS Basic System" and tick it. You must ensure that
   "MSYS Basic System" **is the only enabled component**, then click "Next".
4. Go look at more lolcats while it installs.
5. From the start menu, select MinGW -> MinGW Shell (yes, this is different from above).
6. At the command prompt, type `gcc -v` again. You should get the same output as before.

At this point, you should have a working compiler, and a decent set of tools installed.

2. Setting up an editor
-----------------------

I'm not going to tell you to use one editor or another here - editor choice has been the
subject of Holy Wars for decades, and almost everyone has their favourite. If you don't
have a decent programmer's editor installed, you can go far worse than to start off with
[Notepad++](http://notepad-plus-plus.org/): it's fast, light, does syntax highlighting
and block folding. Apparently TDM-GCC plays nice with [Code::Blocks](http://www.codeblocks.org/),
so you might want to look at that. I tend to use Emacs via [Cygwin](http://www.cygwin.com/),
but I'm also reliably informed that I'm insane, so YMMV and all that.

3. Setting up git
-----------------

To actually get the code, you need to install git. If you already have git (or github's
native tool) installed, you can skip this step.

0. Close any MinGW windows you may have opened when setting up the compiler.
1. Download the latest installer from http://git-scm.com/downloads
2. Run the installer, clicking "Next" through it until you come to the "Adjusting your
   PATH environment" page. Select the "Run Git from the Windows Command Prompt" option,
   and click "Next"
3. Leave the "Choosing the SSH executable" option on "Use OpenSSH". Do not use PuTTY!
4. Keep clicking "Next" until it starts installing.
5. lolcats time again.
7. Once the install has finished, run Git -> Git GUI from the Start Menu. If all goes
   well, the Git Gui window should open.

4. Getting the code
-------------------

Now you should be in the position where you can actually start to fetch code to work on.
All the code is on github, so you need to clone copies of it with git (you may wish to
fork the twscript project before doing so, if you want to push your changes back to
github, but that's up to you).

0. Create a directory somewhere on your system, something like D:\thiefscripts - it doesn't
   matter what, provided that you can remember it, and it **does not** contain spaces.
   Spaces in filenames are another way to summon those betentacled horrors, avoid them.
1. From the start menu, select Git -> Git Bash
2. Change into the directory you created in step 0, using unix-like syntax, eg:
   `cd /d/thiefscripts`
3. Enter the following commands to clone Telliamed's script support libraries:
      git clone https://github.com/whoopdedo/lg.git
      git clone https://github.com/whoopdedo/dh2.git
      git clone https://github.com/whoopdedo/scriptlib.git
4. And this one to clone this project (if you have forked it, clone your fork instead!):
      git clone https://github.com/TheWatcher/twscript.git

You should now have four directories inside the one you created in step 0 above: lg,
dh2, scriptlib, and twscript. Now to compile them...

5. Compiling the libraries
--------------------------

Before you can actually create any scripts, you need to have compiled the support libraries
your scripts will need to use in order to communicate with the Dark engine. This step
should be pretty straightforward at this point. Henceforth I am going to assume that all
the cloned projects are inside the `D:\thiefscripts` directory as discussed in the previous
stage - if you cloned them elsewhere, you must update the paths accordingly.

First you need to fix up a few issues with the standard libraries as cloned:

0. Optional step: open `D:\thiefscripts\lg\lg\config.h` in your text editor, go to line 24
   and enter `#undef __thiscall`, eg:
      #else // !_MSC_VER
      #undef __thiscall
      #define __thiscall
   and save the changes. If you don't do this, you'll get a redefinition warning all over
   the place.
1. Open `D:\thiefscripts\lg\lg\types.h` in your text editor, go to line 195 and change
   `static const float epsilon = 0.00001f` to `static constexpr float epsilon = 0.00001f`
   Save the changes.
2. Open `D:\thiefscripts\lg\Makefile` in your text editor, go to line 24, and change
   `CXXFLAGS = -W -Wall -Wno-unused-parameter -masm=intel $(INCLUDEDIRS)` to be
   `CXXFLAGS = -std=gnu++0x -W -Wall -Wno-unused-parameter -masm=intel $(INCLUDEDIRS)`
   Save the changes.
3. Open `D:\thiefscripts\scriptlib\Makefile` in your text editor, go to line 45, and change
   `CXXFLAGS = -W -Wall -masm=intel` to `CXXFLAGS = -std=gnu++0x -W -Wall -masm=intel`
   Save the changes.

Now you can start compiling:

1. Start a MinGW Shell window
2. Change into the lg directory, eg: `cd /d/thiefscripts/lg`
3. Run `make`
4. Change into the dh2 directory, eg: `cd ../dh2`
5. Run `make`
6. Change into the scriptlib directory, eg: `cd ../scriptlib`
7. Run `make`

