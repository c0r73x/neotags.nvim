#include "archive_util.h"
#include "neotags.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


#define SH(p) ((uint16_t)(uint8_t)((p)[0]) | ((uint16_t)(uint8_t)((p)[1]) << 8))
#define LG(p) ((uint64_t)(SH(p)) | ((uint64_t)(SH((p) + 2)) << 16))
#define TYPE_SIGNED(t)  (!((t)0 < (t)-1))
#define TYPE_WIDTH(t)   (sizeof(t) * CHAR_BIT)
#define TYPE_MAXIMUM(t)              \
        ((t)(!TYPE_SIGNED(t) ? (t)-1 \
                             : ((((t)1 << (TYPE_WIDTH(t) - 2)) - 1) * 2 + 1)))

#ifndef OFF_T_MAX
#   define OFF_T_MAX TYPE_MAXIMUM(off_t)
#endif

#if (defined(_WIN32) || defined(_WIN64)) && !defined(__CYGWIN__)
#   define OPEN_FLAGS O_RDONLY | O_BINARY;
#else
#   define OPEN_FLAGS O_RDONLY | O_NONBLOCK | O_NOCTTY
#endif


//=============================================================================


void gzip_size(struct archive_size *data, const char *name);

static int open_and_stat(const char *name);
static inline void read_error(const char *filename);
static void get_size(struct archive_size *size, int fd, const char *name);


//=============================================================================


static int
open_and_stat(const char *name)
{
        struct stat st;
        int flags = OPEN_FLAGS;
        int fd = open(name, flags);

        if (fd < 0) {
                warn("Failed to open file %s", name);
                return -1;
        } else if (fstat(fd, &st) != 0) {
                warnx("%s is too small - ignored\n", name);
                close(fd);
                return -1;
        } else if (!S_ISREG(st.st_mode)) {
                warnx("%s is not a directory or a regular file - ignored\n", name);
                close(fd);
                return -1;
        }

        return fd;
}


static inline
void read_error(const char *filename)
{
        if (errno != 0)
                err(1, "Error reading file %s", filename);
        else
                errx(1, "%s: unexpected end of file\n", filename);
}


static void
get_size(struct archive_size *size, int ifd, const char *name)
{
        off_t bytes_out = -1L;
        off_t bytes_in;

        bytes_in = lseek(ifd, (off_t)(-8), SEEK_END);

        if (bytes_in != -1L) {
                uint8_t buf[8];
                bytes_in += 8L;
                if (read(ifd, (char *)buf, sizeof(buf)) != sizeof(buf))
                        read_error(name);

                bytes_out = LG(buf + 4);
        }

        size->archive = (size_t)bytes_in;
        size->uncompressed = (size_t)bytes_out;
}


void
gzip_size(struct archive_size *size, const char *name)
{
        int fd = open_and_stat(name);
        if (fd < 0)
                exit(127);
        get_size(size, fd, name);
        close(fd);
}
