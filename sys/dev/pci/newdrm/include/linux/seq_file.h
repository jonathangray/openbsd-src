/* Public domain. */

#ifndef _LINUX_SEQ_FILE_H
#define _LINUX_SEQ_FILE_H

#include <linux/bug.h>
#include <linux/string.h>
#include <linux/string_helpers.h>
#include <linux/fs.h>

struct seq_file {
};

static inline void
seq_printf(struct seq_file *m, const char *fmt, ...)
{
}

static inline void
seq_puts(struct seq_file *m, const char *s)
{
}

static inline void
seq_write(struct seq_file *m, const void *p, size_t s)
{
}

#endif
