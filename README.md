# tlmodder 
by Jardík (jardasmid+tlmodder_at_gmail_dot_com)

### Build

Build depencies: cmake, GCC 4.7+

To compile, run:
```bash
$ cmake . && make
```

That will give you 3 executables:
```
adm2dat
dat2adm
tlmodder
```

### adm2dat
	converts binary *.DAT.ADM, *.LAYOUT.ADM, *.ANIMATION.ADM files to
	textual DAT, LAYOUT or ANIMATION formats.
	
	usage: adm2dat adm_file [dat_file]
	
	If no dat_file is given, it will be printed to stdout. DAT file
	will be UTF-8 encoded! Mods usualy come with UTF-16 encoded DAT
	files, but I have chosen UTF-8 for better editing. I have no idea
	if Torchlight for windows will eat such files, but my modder tool
	will.

### dat2adm
	converts textual DAT, LAYOUT, ANIMATION files to binary ADM format
	
	usage: dat2adm dat_file [adm_file]
	
	If no adm_file is given, 'dat_file.adm' will be used as output file.
	Input dat_file can be UTF-8, UTF-16LE or UTF-16BE encoded, with
	or without BOM. If no BOM is used for UTF-16, file must start with
	ASCII character, which should be true for all DAT files

### tlmodder
	this is the main tool to actually make mods working under linux.
	
	Use it like this:
	
	o Open tlmodder.cfg and set
	  MOD_DIR - path to directory where mods are stored
	            Default is ./mods
	            
	  ORIGINAL_GAME_DATA - path to directory where original pak.zip
	            archive is extracted. media directory contained in
	            the archive must be placed here.
	            Default is ./original
	            
	  OUTPUT_DIR - path to output directory. Resulting media directory
	            will be stored here after the program is run.
	            media directory should then be packed into zip archive
	            called pak.zip and placed into game's directory.
	            You should always delete all files in this directory
	            after adding/removing a mod and before running tlmodder,
	            to prevent conflicts.
	            Default is ./output
	            
	            (generating the zip archive directly is on 'I might once
	             implement it' list, but no promises)
	             
	  MERGE_CLASS_MODS - when you install multiple class or pet mods, you
	            will get colisions for media/UI/charactercreate.layout
	            file. Multiple mods will contain its own to add the
	            class/pet to New game screen. If you set this option
	            to 1, tlmodder will try to find all installed classes and
	            pets and to generate own charactercreate.layout containing
	            all the classes and pets. It will also try to merge item
	            wardrobes for the classes so that gear is not broken because
	            one mod would not contain other mod's class entry for the gear.
	            
	            This WILL add some extra pets even if you don't add any mods.
	            There are some pets in the original game not included in the
	            list in New game screen, but this mod will add them there
	            (fire elemental, gels, and such things)
	            
	            This feature should be considered beta. If it doesn't work for
	            you for any reason, set this to 0 and tlmodder won't touch
	            charactercreate.layout and it will use the one from mod with
	            highest priority set.
	            
	  LOOK_FOR_NEW - set to 1 if tlmodder should look for new mods in
	            MOD_DIR directory. If set to 0, only mods explicitly listed in
	            configuration file will be used. (they still must be located
	            inside MOD_DIR directory).
	            
	            Any mod that is not added via configuration file will get
	            priority 0 and then sorted alphabeticaly.
	
	o Optionaly, if you set LOOK_FOR_NEW to 0 or if you want to adjust mods'
	  priorities and not to rely on alphabetic order, add one or more MOD
	  sections.
	  
	  Default configuration file contains one such section for included
	  "JardikRespec" mod. It is based on RespecMod and adds additional
	  stat points potion (STR-5, DEX-5, MAG-5, DEF-5 and STAT+20).
	  
	  This section should serve as sample, how to configure the mod.
	  NAME     - mod's directory name within MOD_DIR directory
	  PRIORITY - mods with higher priority will overwrite files from mods
	             with lower priority, if they contain the same file. If the
	             priority is the same, alphabetic order of NAME will be used.
	  ENABLED  - if set to 0, mod will be ignored and not included in resulting
	             output directory
	
	o After everything is set, you can run tlmodder:
	
	  ./tlmodder [config_file]
	  
	  where config_file is configuration file name. If no config_file is given,
	  default (./tlmodder.cfg) will be used.
	  
	  After tlmodder is run, it will first list all the loaded mods. If any mod
	  explicitly listed in configuration file is not found, you will be asked if
	  continue or not. If everything goes right or you choose to continue, you
	  will get LOTS and LOTS and LOTS of messages about what is currently being
	  done, and it will take few minutes to complete.
	  
	  After everything is complete, you get Done! message and note that you should
	  pack the resulting media directory and overwrite zip from game directory and
	  to BACK UP you save game files (~/.runicgames/Torchlight/Save) in case
	  something goes wrong and/or new/old mods are not compatible for some reason.
	  
	  With some mods, particulary "Jarcho's Class Compilation v1.3" mod, you will
	  get errors while running tlmodder, pointing out errors detected in few DAT
	  files. You must manually fix these issues and run tlmodder again.
	  
	  Torchlight for Windows seems to silently ignore them, however, I chose not to.
	  Reason why is that one class (I can't remember which, sorry) besides closing
	  wrong sections, actually open wrong section for skill level (LEVEL1 instead of
	  LEVEL6 or something like that). This should be fixed to prevent mod malfunction.
	  There is also few issues with overlapping sections in some mods, these must also
	  be fixed manualy by edditing the dat file. Note that the DAT files are usually
	  UTF-16LE encoded, usually with BOM. Use editor which supports it!

Mods tested and working with tlmooder:
  InfusED
  The Hero's Legacy 1.10
  Valkyrie2_v4.31b (I think I had to delete one [/SKILL] line in one DAT file, not sure)
  Emberborn
  ID&PortalStones
  Jarcho's Class Compilation v1.3 (needs few manual fixes in DAT files, two pets
                                   causing the game crash, don't remember which)
  ThrowingWeapons
  Tl2_pets
  BrighterMap
  ChainLightningLLv2_1
  Enhanced Mastery
  DUI_AutoMap

* As pointed out, tlmodder will tell you where the error is, at which line, so it should
be easily fixable.



Well that should be about it. If you find something missing here, let me know. MAYBE
I will fix it eventually, maybe not. Point is Torchlight is too short and repetitive
game and I got bored of it pretty soon and boring game means maybe no bug fixes* for
this modding tool.

* BTW there is plenty of TODOs and FIXMEs in source files, one worth mentioning is
  incomplete UTF-8 to UTF-32 and then to UTF-16. I just didn't feel like finishing
  this anymore, but maybe if someone ask for it ... maybe. For now I suggest to
  stick to ASCII characters.

This software is licensed under the terms of
  DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE Version 2
See COPYING file for more info
