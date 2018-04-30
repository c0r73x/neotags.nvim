FILESDIR := ./neotags_bin
BUILDDIR  = ${FILESDIR}/build
NPROC    != if (command -v nproc >/dev/null 2>&1); then \
                nproc;                                  \
            else                                        \
                echo 2;                                 \
            fi
MKFLAGS  != if (${MAKE} --no-print-directory --help >/dev/null 2>&1); then \
                echo --no-print-directory --silent -j ${NPROC};            \
            else                                                           \
                echo -j ${NPROC};                                          \
            fi                                                             \

all: install
	@printf "Cleaning...\n"
	@rm -rf "${FILESDIR}/build"

install: build
	@printf "\nInstalling into ~/.vim_tags/bin\n"
	@install -c -m755 "${BUILDDIR}/src/neotags" "${HOME}/.vim_tags/bin"

build: mkdir
	@(cd "${BUILDDIR}" && cmake -DCMAKE_BUILD_TYPE=Release ..)
	@${MAKE} -C "${BUILDDIR}" ${MKFLAGS}

mkdir:
	@mkdir -p "${BUILDDIR}"
	@rm -rf "${BUILDDIR}/{*,.*}"
