#ifndef	SKYRIMOUTFITSYSTEMSE_VERSION_INCLUDED
#define SKYRIMOUTFITSYSTEMSE_VERSION_INCLUDED

#define SKYRIMOUTFITSYSTEMSE_MAKE_STR_HELPER(x) #x
#define SKYRIMOUTFITSYSTEMSE_MAKE_STR(x) SKYRIMOUTFITSYSTEMSE_MAKE_STR_HELPER(x)

#define SKYRIMOUTFITSYSTEMSE_VERSION_MAJOR		0
#define SKYRIMOUTFITSYSTEMSE_VERSION_MINOR		4
#define SKYRIMOUTFITSYSTEMSE_VERSION_PATCH		0
#define SKYRIMOUTFITSYSTEMSE_VERSION_BETA		0
#define SKYRIMOUTFITSYSTEMSE_VERSION_VERSTRING	SKYRIMOUTFITSYSTEMSE_MAKE_STR(SKYRIMOUTFITSYSTEMSE_VERSION_MAJOR) "." SKYRIMOUTFITSYSTEMSE_MAKE_STR(SKYRIMOUTFITSYSTEMSE_VERSION_MINOR) "." SKYRIMOUTFITSYSTEMSE_MAKE_STR(SKYRIMOUTFITSYSTEMSE_VERSION_PATCH) "." SKYRIMOUTFITSYSTEMSE_MAKE_STR(SKYRIMOUTFITSYSTEMSE_VERSION_BETA)

#endif
