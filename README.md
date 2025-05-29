# GameLauncher
A quickly hacked launcher for the Steam version of Encounter

# Change history
- 1.0.0.0 
  - First version  
- 1.0.0.1 
   - Added options to minimize and quit after launch  
- 1.0.0.2 
  - Added options to not restore the position of the dialog on startup  
  - Modified the achievement handling to reset achievements of older game versions  
- 1.0.0.3
  - Added hyperlinks to open the homepage, manual and support emails  
- 1.0.0.4
  - Support email should not have two : after the mailto  
  - German layout should be QWERTZ, not QWERTYZ  
  - Fixed (I hope) the problem with the minimized window that cannot be restored
- 1.0.0.5
  - Fixed a number of typos in French
- 1.0.0.6
  - The emulator application now appears with "Encounter" as title and icon instead of the default "Oricutron 1.2" and default Oric icon
  - Changed the initialization of Steam so the error messages appear localized in English or French depending of the user settings
- 1.0.0.7
  - Updated the Oricutron version to the latest build
  - Added a checkbox to enable or disable the status bar
- 1.0.0.8
  - Reverted to the previous Oricutron
  - Removed the status bar checkbox
  - Extended the search for window handle

# Steam 
The main game and the demo have different AppID:
- Main Game
  - AppID:   3319780  -> https://partner.steamgames.com/apps/builds/3319780
  - DepotID: 3319781
- Demo:
  - AppID:   3476830  -> https://partner.steamgames.com/apps/builds/3476830
  - DepotID: 3476831
 
## Updating the build in Steam
- Prepare and upload the new build with either SteamCMD or the SteamPipe Build Uploader
- Log on https://partner.steamgames.com/dashboard
- Select the game in Recent Apps
- Go to Technical Tools -> Edit Steamworks Settings (https://partner.steamgames.com/apps/view/3319780)
- Select SteamPipe -> Builds (https://partner.steamgames.com/apps/builds/3319780)
- Select the uploaded build and change the build menu to "default" and click on "Preview Build"
- Eventually add a comment and click on Set Build Live Now
- At that point the build is now on Steam and available to all the users

## Creating new events
- Go to "Community & Moderation" -> Post/Manage events (https://steamcommunity.com/games/3319780/partnerevents/)
- Make sure to have English and French event pictures

## Setting the price
- https://partner.steamgames.com/pricing/dashboard/293994

## Accessing the user settings
- Open %LOCALAPPDATA%\EncounterByDefenceForce

## Achievements
Achievements can be created and edited on https://partner.steamgames.com/apps/achievements/3319780

Clearing achievements
- Open the steam console with steam://open/console
- Reset a single achievement: achievement_clear 3319780 <achievement name> 
- Reset all achievements: reset_all_stats 3319780 


# Itch.io
Game builds can be uploaded to itch manually or automatically using Butler.

The itch game page for Encounter is https://defenceforce.itch.io/encounter


## Using Butler.
Documentation for Butler can be found there: https://itch.io/docs/butler/

## Pushing builds with butler
See: https://itch.io/docs/butler/pushing.html

The only command that you need to remember is butler push
```
butler push directory user/game:channel
```
Where:

- directory is what you want to upload. It can also be a .zip file.  
- user/game is the project you're uploading  
  - for example: finji/overland for https://finji.itch.io/overland — all lower-case  
- channel is which slot you're uploading it to
  - for example: windows-beta, osx-bonus, linux-universal, or soundtrack
	
*Channel names will determine the initial set of tags of a slot, but you can always fix them later.*


Example for Encounter:
```
cd C:\Users\Mike\AppData\Roaming\itch\apps\butler

:: Push the main version of the game and check the status
butler push C:\Projects\Encounter\ItchIoContent defenceforce/encounter:windows
butler status defenceforce/encounter:windows

:: Push the demo version of the game and check the status
butler push C:\Projects\Encounter\ItchIoContent defenceforce/encounter:windows_demo
butler status defenceforce/encounter:windows_demo
```

In details.
```
cd C:\Users\Mike\AppData\Roaming\itch\apps\butler
butler push C:\Projects\Encounter\ItchIoContent defenceforce/encounter:windows


      ..........................
    ':cccclooooolcccccloooooocccc:,.
  ':cccccooooooolccccccooooooolccccc,.
 ,;;;;;;cllllllc:;;;;;;clllllll:;;;;;;.
 ,,,,,,,;cccccc;,,,,,,,,cccccc:,,,,,,,.
 .',,,'..':cc:,...,,,'...;cc:,...',,'.
   .,;:dxl;,;;cxdc,,,;okl;,,,:odc,,,.
   ,kkkkkx:'..'okkkkkkxxo'...;oxxxxx,
   ,kkkk:       ...''...       ,dxxx,
   ,kkk:          .:c'          'xxx;
   ,kko         .,ccc:;.         :xx;
   ,kx.         .,;;,,'..         cl'
   ,kc           .''''.           'l'
   ,x.       ..............       .l'
   ,x'      ,oddddddddoolcc,      .l'
   'xo,...;ldxxxxxxxdollllllc;...'cl'
   .:ccc:ccccccccc:;;;;;;;;;;;;;;;;,.


Welcome to the itch.io command-line tools!
If it hasn't already, open the following link in your browser to authenticate:
https://itch.io/user/oauth?client_id=butler&redirect_uri=http%3A%2F%2F127.0.0.1%3A55591%2Foauth%2Fcallback&response_type=token&scope=wharf

(To copy text in cmd.exe: Alt+Space, Edit->Mark, select text, press Enter)

If you're running this on a remote server, the redirect will fail to load.
In that case, copy the address you're redirected to, paste it below, and press enter.

Authenticated successfully! Saving key in C:\Users\Mike\.config\itch\butler_creds...

∙ For channel `windows`: pushing first build
∙ Pushing 3.55 MiB (54 files, 9 dirs, 0 symlinks)
√ Added 3.55 MiB fresh data
√ 1.53 MiB patch (57.08% savings)
∙ Build is now processing, should be up in a bit.

Use the `butler status defenceforce/encounter:windows` for more information.
```
then the status can be checked:
```
cd C:\Users\Mike\AppData\Roaming\itch\apps\butler
butler status defenceforce/encounter:windows

+---------+-----------+------------+---------+
| CHANNEL |  UPLOAD   |   BUILD    | VERSION |
+---------+-----------+------------+---------+
| windows | #12273292 | √ #1143390 |       1 |
+---------+-----------+------------+---------+

```

which should also print a notification in the itch launcher and website:

> Today   
> NEWBuild windows 1 for Encounter HD (Oric) is now live 1m

