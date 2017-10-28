# All Hands On Deck! (AHOD)

[![GitHub release](https://img.shields.io/github/release/allejo/AllHandsOnDeck.svg?maxAge=2592000)](https://github.com/allejo/AllHandsOnDeck/releases/latest)
![Minimum BZFlag Version](https://img.shields.io/badge/BZFlag-v2.4.12+-blue.svg)
[![License](https://img.shields.io/github/license/allejo/AllHandsOnDeck.svg)](https://github.com/allejo/AllHandsOnDeck/blob/master/LICENSE.md)

Attention all tankers! All hands on deck!

Originally introduced as a new game mode by [trepan](https://forums.bzflag.org/viewtopic.php?f=64&t=6449), this game mode requires an insane amount of team work, more than BZFlag leagues. In this game mode, all of the members of a team must be "on deck" with the enemy flag in order to capture the flag. If just one team member is missing, there is no capture!

## Requirements

- BZFS 2.4.12
- C++11

This plug-in follows [my standard instructions for compiling plug-ins](https://github.com/allejo/docs.allejo.io/wiki/BZFlag-Plug-in-Distribution).

## Usage

### Loading the plug-in

This plug-in takes a comma separated list of arguments.

1. Set the game mode for this map.
    - *SingleDeck* - the traditional AHOD game mode where two or more teams fight to capture the enemy flag on a single deck; think king of the hill.
    - *MultipleDecks* - having multiple decks, each deck is tied to a specific team; think traditional CTF but requiring your entire team on the deck to cap.
1. (optional) Set the welcome message.
    - *default* - enable the default welcome message explaining the game mode
    - *disabled* - disable the welcome message entirely
    - *filepath* - path to a text file. The contents of the text file will be used as the welcome message

```text
-loadplugin AllHandsOnDeck,<SingleDeck|MultipleDecks>
-loadplugin AllHandsOnDeck,<SingleDeck|MultipleDecks>,<default|disabled|filepath>
```

### Custom BZDB Variables

These custom BZDB variables must be used with -setforced, which sets BZDB variable `<name>` to `<value>`, even if the variable is not built-in. These variables may be changed at any time in-game by using the /set command.

```text
-setforced <name> <value>
```

| Name | Type | Default | Description |
| ---- | :--: | :-----: | ----------- |
| `_ahodPercentage` | float | 1 | The percentage (in decimal form) of a team that must be on the deck in order to cap. |

### Custom Map Objects

This plug-in introduces the `DECK` map object which defines the location of where a team must be in order to cap. This object supports the traditional `position`, `size`, and `rotation` attributes.

> **Warning:** The `AHOD` map object has been deprecated and will be removed in the future. The `DECK` object has taken its place and is backwards compatible.

Additionally, for the *MultipleDecks* game mode, the `color` attribute is required, which defines which team this deck is for; only one deck per team is supported. The accepted values are the same as the `base` object.

```text
deck
  position 0 0 0
  size 5 5 5
  rotation 0
  color 1
end
```

## License

[MIT](https://github.com/allejo/AllHandsOnDeck/blob/master/LICENSE.md)

### TriHix License

I have included TriHix in this repository since this map and the plug-in go together; however, the map itself is not licensed to be used or hosted without permission.
