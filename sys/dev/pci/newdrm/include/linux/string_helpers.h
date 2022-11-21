/* Public domain. */

#ifndef _LINUX_STRING_HELPERS_H
#define _LINUX_STRING_HELPERS_H

static inline const char *
str_yes_no(bool x)
{
	if (x)
		return "yes";
	return "no";
}

#endif
