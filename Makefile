FILESDIR := "./neotags_bin"

all: install
	${MAKE} -C "${FILESDIR}" distclean
	@rm -rf "${FILESDIR}/src/.deps"

install:
	@cd "${FILESDIR}" && sh configure --prefix="${HOME}/.vim_tags"
	@${MAKE} -C "${FILESDIR}"
	@${MAKE} -C "${FILESDIR}" install-strip
