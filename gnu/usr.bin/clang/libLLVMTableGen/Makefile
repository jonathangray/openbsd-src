# $OpenBSD: Makefile,v 1.3 2019/01/27 16:59:42 patrick Exp $

LIB=	LLVMTableGen
NOPROFILE=

CPPFLAGS+=	-I${LLVM_SRCS}/include/llvm/TableGen

.include <bsd.own.mk>
SRCS=	Error.cpp \
	Main.cpp \
	JSONBackend.cpp \
	Record.cpp \
	SetTheory.cpp \
	StringMatcher.cpp \
	TableGenBackend.cpp \
	TGLexer.cpp \
	TGParser.cpp

.PATH:	${.CURDIR}/../../../llvm/lib/TableGen

install:
	@# Nothing here so far ...

.include <bsd.lib.mk>

ARLIBNAME=lib${LIB}.so
${ARLIBNAME}.a: ${SOBJS}
	ar cqD ${ARLIBNAME}.a ${SOBJS}

all: lib${LIB}.a lib${LIB}.so.a

CLEANFILES += ${ARLIBNAME}.a
