# kryoto-online

A drop-in replacement for `steam_api.dll` / `steam_api64.dll` that reports the
running game as Spacewar (AppID 480) to a real Steam client.

Spacewar is free, so every account already owns it and the ownership check
passes. The rest of the Steamworks surface is forwarded to the real client
untouched, which is why Steam's own matchmaking, lobbies and P2P still work.

**This is not an emulator.** It needs the Steam client running and signed in. If
you want a build that runs with no Steam at all, you want gbe_fork instead - the
two solve different problems and are not interchangeable.

## Build

Visual Studio Build Tools 2022 or later, with "Desktop development with C++".

```
build.bat
```

Output:

```
build\x86\steam_api.dll
build\x64\steam_api64.dll
```

## Configure

Put a `kryoto-online.ini` **next to the game's executable** - not next to the
DLL. The two are the same folder for most games and are not for Unity ones,
where the DLL lives under `<Game>_Data\Plugins\x86_64\`.

```ini
[Settings]
AppId=480
ogAppId=220
PluginsFolder=plugins
GetStubbedLol=false
UnlockDLC=123,456,789
EmulateTicket=true
```

| Key | What it does |
|---|---|
| `AppId` | What the game is told it is. `480` (Spacewar) unless it refuses, in which case try `440`. |
| `ogAppId` | What it really is, so stats, achievements and DLC resolve against the right title. |
| `PluginsFolder` | Folder of extra DLLs to load, relative to the executable. |
| `GetStubbedLol` | Load a SteamStub-protected executable. |
| `UnlockDLC` | Comma-separated AppIDs to report as owned and installed. |
| `EmulateTicket` | Emulate auth tickets. Needed by games that verify a session ticket. |

**Without this file nothing is configured.** The loader leaves its path empty and
every setting silently takes its default: AppID 480 with no `ogAppId`, no DLC,
no plugins, no ticket emulation. The game launches and simply behaves as though
you had not set anything up, which is a confusing way to spend an evening.

Logs go to `%TEMP%\kryoto-online.log`.

## Licence

See [LICENSE.md](LICENSE.md). Note in particular that **selling this is
prohibited, modified or not** - that is the original authors' term and it
carries over here unchanged.
