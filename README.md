All Hands On Deck! (AHOD)
================
[![Current Release](https://img.shields.io/badge/release-v1.0.1-orange.svg)](https://github.com/allejo/AllHandsOnDeck/releases/tag/v1.0.1)
![Minimum BZFlag Version](https://img.shields.io/badge/BZFlag-v2.4.3+-blue.svg)

Attention all tankers! All hands on deck!

Originally introduced as a new game mode by [trepan](https://forums.bzflag.org/viewtopic.php?f=64&t=6449), this game mode requires an insane amount of team work, more than BZFlag leagues. In this game mode, all of the members of a team must be in the AHOD zone with the enemy flag in order to capture the flag. If just one team member is missing, there is no capture!

## Author

Vladimir "allejo" Jimenez

## Requirements

- BZFlag 2.4.3+ (*latest version available on GitHub is recommended*)
- C++11

## Usage

This plug-in introduces an `AHOD` map object. Only one `AHOD` object may be defined in a map due to the gameplay; creating more than one object may lead to unintended side effects.

The plug-in takes one command line parameter and that's the path to a text file. The text file is read and shown to players on join explaining how AHOD works.

### Loading the plug-in

```
-loadplugin /path/to/AllHandsOnDeck.so,welcome.txt
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
