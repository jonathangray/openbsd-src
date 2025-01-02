/* Public domain. */

#ifndef _LINUX_UUID_H
#define _LINUX_UUID_H

#define UUID_STRING_LEN 36
#define UUID_SIZE	16

typedef struct {
	uint8_t guid[UUID_SIZE];
} guid_t;

#endif
