# $OpenBSD: Makefile,v 1.2 2017/07/09 15:28:34 espie Exp $

LIB=	LLVMAArch64Utils
NOPROFILE=

CPPFLAGS+=	-I${.OBJDIR}/../include/llvm/AArch64 \
		-I${LLVM_SRCS}/lib/Target/AArch64

.include <bsd.own.mk>
SRCS=	AArch64BaseInfo.cpp

.PATH:	${.CURDIR}/../../../llvm/lib/Target/AArch64/Utils

install:
	@# Nothing here so far ...

.include <bsd.lib.mk>

ARLIBNAME=lib${LIB}.so
${ARLIBNAME}.a: ${SOBJS}
	ar cqD ${ARLIBNAME}.a ${SOBJS}

all: lib${LIB}.a lib${LIB}.so.a

CLEANFILES += ${ARLIBNAME}.a
