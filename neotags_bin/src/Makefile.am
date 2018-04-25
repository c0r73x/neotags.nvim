bin_PROGRAMS=neotags
neotags_SOURCES=	neotags.c     \
			utility.c     \
			linked_list.c \
			neotags.h

#			options.c

# if NEED_BSD_FUNCS
neotags_SOURCES+=	bsd_funcs.c bsd_funcs.h
# endif

neotags_LDADD =		${PCRE2_LIBS}
