FILESDIR := ./neotags_bin
BUILDDIR  = ${FILESDIR}/build
PREFIX   ?= ${HOME}/.vim_tags
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

DBG_TYPE := Debug
STRIP    ?= strip

all: install
	@printf "Cleaning...\n"
	@rm -rf "${FILESDIR}/build"

install: build
	@printf "\nInstalling into ~/.vim_tags/bin\n"
	@${STRIP} "${BUILDDIR}/src/neotags"
	@install -c -m755 "${BUILDDIR}/src/neotags" "${PREFIX}/bin"

build: mkdir
	@(cd "${BUILDDIR}" && cmake -DCMAKE_BUILD_TYPE=Release ..)
	@${MAKE} -C "${BUILDDIR}" ${MKFLAGS}

debug: dbg_install
	@printf "Cleaning...\n"
	rm -rf "${FILESDIR}/build"

dbg_install: dbg_build
	@printf "\nInstalling into ~/.vim_tags/bin\n"
	@cp -a "${BUILDDIR}/src/neotags" "${PREFIX}/bin/neo"
	@rm -f "${PREFIX}/bin/neotags"
	ln -sr "${PREFIX}/bin/Neotags.sh" "${PREFIX}/bin/neotags"

dbg_build: mkdir
	@(cd "${BUILDDIR}" && cmake -DCMAKE_BUILD_TYPE=${DBG_TYPE} ..)
	${MAKE} --no-print-directory -C "${BUILDDIR}" VERBOSE=1

mkdir:
	@mkdir -p "${BUILDDIR}"
	@rm -rf "${BUILDDIR}/{*,.*}"
