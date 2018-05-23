#include "archive_util.h"
#include "neotags.h"
#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <lzma.h>

#ifdef _MSC_VER
    typedef int64_t ssize_t;
#endif

#if BUFSIZ <= 1024
#   define IO_BUFFER_SIZE 8192
#else
#   define IO_BUFFER_SIZE (BUFSIZ & ~7U)
#endif

#ifdef DOSISH
#   include <windows.h>
#else
#   include <unistd.h>
#endif

#define safe_close(FILE_DESC) do if ((FILE_DESC) > 2) close(FILE_DESC); while (0)
#define percentage(IA, IB) (((double)(IA) / (double)(IB)) * 100)


//============================================================================= 

#include <fcntl.h>

#ifndef O_BINARY
#   define O_BINARY 0
#endif

#ifndef O_NOCTTY
#   define O_NOCTTY 0
#endif

#define ARRAY_SIZE(_ARRAY_) (sizeof(_ARRAY_) / sizeof(*(_ARRAY_)))

char *prog_name;

typedef enum {
        IO_WAIT_MORE,    // Reading or writing is possible.
        IO_WAIT_ERROR,   // Error or user_abort
        IO_WAIT_TIMEOUT, // poll() timed out
} io_wait_ret;


/// Information about a .xz file
typedef struct xz_file_info_s {
        lzma_index *idx;
        uint64_t stream_padding;
        uint64_t memusage_max;
        bool all_have_sizes;
        uint32_t min_version;
} xz_file_info;


typedef union {
        uint8_t u8[IO_BUFFER_SIZE];
        uint32_t u32[IO_BUFFER_SIZE / sizeof(uint32_t)];
        uint64_t u64[IO_BUFFER_SIZE / sizeof(uint64_t)];
} io_buf;


struct xz_file {
        const char *name;
        int fd;
        bool eof;
        struct stat st;
};


//============================================================================= 


const char * message_strm(lzma_ret code);

static bool parse_indexes(struct xz_file *file, xz_file_info *xfi);
static size_t io_read(struct xz_file *file, io_buf *buf_union, size_t size);


//============================================================================= 
//============================================================================= 
//============================================================================= 


/// Opens the source file. Returns false on success, true on error.
static bool
io_open_src_real(struct xz_file *file)
{
        int flags = O_RDONLY | O_BINARY | O_NOCTTY;

        file->fd = open(file->name, flags);

        if (file->fd == -1) {
                assert(errno != EINTR);
                warn("%s:", file->name);
                return true;
        }

        if (fstat(file->fd, &file->st))
                goto error_msg;

        if (!S_ISREG(file->st.st_mode)) {
                warnx("%s: Not a regular file, skipping\n", file->name);
                goto error;
        }

        return false;

error_msg:
        warn("%s", file->name);
error:
        (void)close(file->fd);
        return true;
}


static struct xz_file *
io_open_src(const char *name)
{
        if (*name == '\0')
                return NULL;
        static struct xz_file file;

        file = (struct xz_file){
                .name = name,
                .fd = -1,
                .eof = false,
        };

        const bool error = io_open_src_real(&file);

        return error ? NULL : &file;
}


//============================================================================= 
//============================================================================= 

static bool
io_seek_src(struct xz_file *file, off_t pos)
{
        assert(pos >= 0);

        if (lseek(file->fd, pos, SEEK_SET) != pos) {
                warn("%s: Error seeking the file", file->name);
                return true;
        }

        file->eof = false;

        return false;
}


#define MEM_LIMIT UINT64_MAX

static bool
parse_indexes(struct xz_file *file, xz_file_info *xfi)
{
        if (file->st.st_size <= 0) {
                warnx("%s: File is empty\n", file->name);
                return false;
        }

        if (file->st.st_size < 2 * LZMA_STREAM_HEADER_SIZE) {
                warnx("%s: Too small to be a valid .xz file\n", file->name);
                return false;
        }

        io_buf buf;
        lzma_stream strm[] = {LZMA_STREAM_INIT};
        lzma_index *idx = NULL;
        lzma_ret ret = lzma_file_info_decoder(strm, &idx, MEM_LIMIT,
                                              (uint64_t)(file->st.st_size));
        if (ret != LZMA_OK)
                return false;

        for (bool done = false; !done ; )
        {
                if (strm->avail_in == 0) {
                        strm->next_in = buf.u8;
                        strm->avail_in = io_read(file, &buf, IO_BUFFER_SIZE);
                        if (strm->avail_in == SIZE_MAX)
                                warn("Unkown IO error");
                }

                ret = lzma_code(strm, LZMA_RUN);

                switch (ret) {
                case LZMA_OK:
                        break;

                case LZMA_SEEK_NEEDED:
                        // The cast is safe because liblzma won't ask us to seek past
                        // the known size of the input file which did fit into off_t.
                        assert(strm->seek_pos <= (uint64_t)(file->st.st_size));
                        if (io_seek_src(file, (off_t)(strm->seek_pos))) {
                                warnx("%d, %s: %s\n", ret, file->name,
                                      message_strm(ret));
                                return false;
                        }

                        strm->avail_in = 0;
                        break;

                case LZMA_STREAM_END:
                        xfi->idx = idx;

                        // Calculate xfi->stream_padding.
                        lzma_index_iter iter;
                        lzma_index_iter_init(&iter, xfi->idx);
                        while (!lzma_index_iter_next(&iter,
                                                     LZMA_INDEX_ITER_STREAM))
                                xfi->stream_padding += iter.stream.padding;

                        done = true;
                        break;

                default:
                        warnx("%d, %s: %s\n", ret, file->name, message_strm(ret));
                        return false;
                }
        }

        lzma_end(strm);
        return true;
}


static size_t
io_read(struct xz_file *file, io_buf *buf_union, size_t size)
{
        uint8_t *buf = buf_union->u8;
        size_t left = size;

        while (left > 0) {
                const ssize_t amount = read(file->fd, buf, left);

                if (amount == 0) {
                        file->eof = true;
                        break;
                }

                if (amount == -1) {
                        warn("%s: Read error", file->name);
                        return SIZE_MAX;
                }

                buf += (size_t)(amount);
                left -= (size_t)(amount);
        }

        return size - left;
}


const char *
message_strm(lzma_ret code)
{
        switch (code) {
        case LZMA_OK:                break;
        case LZMA_STREAM_END:        return "The stream ended, moron";
        case LZMA_NO_CHECK:          return "No integrity check; not verifying file integrity";
        case LZMA_UNSUPPORTED_CHECK: return "Unsupported type of integrity check; not verifying file integrity";
        case LZMA_GET_CHECK:         return stringify(LZMA_GET_CHECK);
        case LZMA_MEM_ERROR:         return strerror(ENOMEM);
        case LZMA_MEMLIMIT_ERROR:    return "Memory usage limit reached";
        case LZMA_FORMAT_ERROR:      return "File format not recognized";
        case LZMA_OPTIONS_ERROR:     return "Unsupported options";
        case LZMA_DATA_ERROR:        return "Compressed data is corrupt";
        case LZMA_BUF_ERROR:         return "Unexpected end of input";
        case LZMA_PROG_ERROR:        return stringify(LZMA_PROG_ERROR);
        case LZMA_SEEK_NEEDED:       return stringify(LZMA_SEEK_NEEDED);
        }
        return "Internal error (bug)";
}


//=============================================================================
//=============================================================================
//=============================================================================


#define XZ_FILE_INFO_INIT { NULL, 0, 0, true, 50000002 }

void
xz_size(struct archive_size *size, const char *filename)
{
        struct xz_file *file = io_open_src(filename);
        if (file == NULL)
                err(1, "Failed to open file %s", filename);

        xz_file_info xfi = XZ_FILE_INFO_INIT;

        if (parse_indexes(file, &xfi)) {
                size->archive = lzma_index_file_size(xfi.idx);
                size->uncompressed = lzma_index_uncompressed_size(xfi.idx);
                lzma_index_end(xfi.idx, NULL);
                safe_close(file->fd);
        } else {
                safe_close(file->fd);
                errx(1, "Error is fatal, exiting.\n");
        }
}
