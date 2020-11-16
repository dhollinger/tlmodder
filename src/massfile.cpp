/* 
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */ 

#include "massfile.h"

namespace tlmodder {

bool MassFile::isDirWhitelisted(FileName const& modDir)
{
	static char const* whitelist[] = {
		"MEDIA/AFFIXES",
		"MEDIA/CINEMATICS",
		"MEDIA/DUNGEONS",
		"MEDIA/FORMATIONS",
		"MEDIA/GRAPHS",
		"MEDIA/LAYOUTS",
		"MEDIA/LEVELSETS",
		"MEDIA/PARTICLES",
		"MEDIA/PERKS",
		"MEDIA/QUESTS",
		"MEDIA/RECIPES",
		"MEDIA/SETS",
		"MEDIA/SKILLS",
		"MEDIA/SOUNDS",
		"MEDIA/SPAWNCLASSES",
		"MEDIA/TRANSLATIONS",
		"MEDIA/UNITTHEMES",
		"MEDIA/MODELS",
		"MEDIA/UI"
	};
	
	for (char const* whitelistEntry : whitelist)
	{
		if (modDir.isChildOf(whitelistEntry))
			return true;
	}
	
	return false;
}

}
