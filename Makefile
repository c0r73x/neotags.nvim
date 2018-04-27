FILESDIR := ./neotags_bin
BUILDDIR  = ${FILESDIR}/build
NPROC    != if (command -v nproc >/dev/null 2>&1); then \
                nproc;                                  \
            else                                        \
                echo 2;                                 \
            fi

all: install
	@printf "\nCleaning...\n"
	rm -rf "${FILESDIR}/build"

install: build
	@printf "\nInstalling into ~/.vim_tags/bin\n"
	install -c -m755 "${BUILDDIR}/src/neotags" "${HOME}/.vim_tags/bin"

build: mkdir
	@(cd "${BUILDDIR}" && cmake -DCMAKE_BUILD_TYPE=Release ..)
	@${MAKE} -C "${BUILDDIR}" -j "${NPROC}"

mkdir:
	@mkdir -p "${BUILDDIR}"
	@rm -rf "${BUILDDIR}/{*,.*}"

# FILESDIR := "./neotags_bin"
# 
# all: install
#         ${MAKE} -C "${FILESDIR}" distclean
#         @rm -rf "${FILESDIR}/src/.deps"
# 
# install:
#         @cd "${FILESDIR}" && sh configure --prefix="${HOME}/.vim_tags"
#         @${MAKE} -C "${FILESDIR}"
#         @${MAKE} -C "${FILESDIR}" install-strip
