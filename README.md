All Hands On Deck (AHOD) [![Current Release](https://img.shields.io/badge/release-v1.0.0-orange.svg)](https://github.com/allejo/AllHandsOnDeck/releases/tag/v1.0.0) ![Minimum BZFlag Version](https://img.shields.io/badge/BZFlag-v2.4.3+-blue.svg)
================

Attention all tankers! All hands on deck!

Originally introduced as a new game mode by [trepan](http://forums.bzflag.org/viewtopic.php?f=64&t=6449&p=64686&hilit=ahod#p64686), this game mode requires an insane amount of team work, more than BZFlag leagues. In this game mode, all of the members of a team must be in the AHOD zone with the enemy flag in order to capture the flag. If just one team member is missing, there is no capture!

## Author

Vladimir "allejo" Jimenez

## Compiling

### Requirements

- BZFlag 2.4.3+ (*latest version available on GitHub is recommended*)
- C++11

### How to Compile

1. Check out the 2.4.x BZFlag source code from GitHub, if you do not already have it on your server.

        git clone -b 2.4 https://github.com/BZFlag-Dev/bzflag.git

2. Go into the newly checked out source code and then the plugins directory.
                
        cd bzflag/plugins

3. Run a git clone of this repository from within the plugins directory. This will create a new `AllHandsOnDeck` directory within the plugins directory. Notice, you will be checking out the 'release' branch which will always contain the latest stable release of the plugin to allow for easy updates. If you are running a test port and would like the latest development build, use the 'master' branch instead of 'release.'

        git clone -b release https://github.com/allejo/AllHandsOnDeck.git

4. Use the 'addToBuild.sh' script to add the plugin to the build system.

        sh addToBuild.sh AllHandsOnDeck

5. Instruct the build system to generate a Makefile, then compile, and install the plugin.

        cd ..; ./autogen.sh; ./configure; make; make install;

## Usage

This plug-in has no configuration options. It only contains an `AHOD` map object.

### Loading the plug-in

```
-loadplugin /path/to/AllHandsOnDeck.so
```

### BZW Example

The AHOD zone uses the [bz_CustomZoneObject](http://forums.bzflag.org/viewtopic.php?f=40&t=19034) from the BZFS API so it supports the same syntax as normal map objects. It also supports the bbox and cylinder syntax, although this syntax will be deprecated in the API with BZFlag 2.6+.

```
ahod
  position 0 0 0
  size 40 40 30
  rotation 45
end
```

## License

[GNU General Public License Version 3.0](https://github.com/allejo/AllHandsOnDeck/blob/master/LICENSE.md)

### TriHix License

I have included TriHix in this repository since this map and the plug-in go together; however, the map itself is not licensed to be used or hosted without permission.
