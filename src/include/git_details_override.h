#ifndef BIOS_GIT_DETAILS_OVERRIDE
#define BIOS_GIT_DETAILS_OVERRIDE

#undef PACKAGE_GIT_BRANCH
#undef PACKAGE_GIT_ORIGIN
#undef PACKAGE_GIT_TSTAMP
#undef PACKAGE_GIT_HASH_S
#undef PACKAGE_GIT_HASH_L
#undef PACKAGE_GIT_STATUS

#undef PACKAGE_GIT_BRANCH_ESCAPED
#undef PACKAGE_GIT_ORIGIN_ESCAPED
#undef PACKAGE_GIT_TSTAMP_ESCAPED
#undef PACKAGE_GIT_HASH_S_ESCAPED
#undef PACKAGE_GIT_HASH_L_ESCAPED
#undef PACKAGE_GIT_STATUS_ESCAPED

#undef PACKAGE_BUILD_HOST_UNAME
#undef PACKAGE_BUILD_HOST_NAME
#undef PACKAGE_BUILD_HOST_OS
#undef PACKAGE_BUILD_TSTAMP

#undef PACKAGE_BUILD_HOST_UNAME_ESCAPED
#undef PACKAGE_BUILD_HOST_NAME_ESCAPED
#undef PACKAGE_BUILD_HOST_OS_ESCAPED
#undef PACKAGE_BUILD_TSTAMP_ESCAPED

extern char * PACKAGE_GIT_BRANCH;
extern char * PACKAGE_GIT_ORIGIN;
extern char * PACKAGE_GIT_TSTAMP;
extern char * PACKAGE_GIT_HASH_S;
extern char * PACKAGE_GIT_HASH_L;
extern char * PACKAGE_GIT_STATUS;

extern char * PACKAGE_GIT_BRANCH_ESCAPED;
extern char * PACKAGE_GIT_ORIGIN_ESCAPED;
extern char * PACKAGE_GIT_TSTAMP_ESCAPED;
extern char * PACKAGE_GIT_HASH_S_ESCAPED;
extern char * PACKAGE_GIT_HASH_L_ESCAPED;
extern char * PACKAGE_GIT_STATUS_ESCAPED;

extern char * PACKAGE_BUILD_HOST_UNAME;
extern char * PACKAGE_BUILD_HOST_NAME;
extern char * PACKAGE_BUILD_HOST_OS;
extern char * PACKAGE_BUILD_TSTAMP;

extern char * PACKAGE_BUILD_HOST_UNAME_ESCAPED;
extern char * PACKAGE_BUILD_HOST_NAME_ESCAPED;
extern char * PACKAGE_BUILD_HOST_OS_ESCAPED;
extern char * PACKAGE_BUILD_TSTAMP_ESCAPED;

#endif // BIOS_GIT_DETAILS_OVERRIDE

