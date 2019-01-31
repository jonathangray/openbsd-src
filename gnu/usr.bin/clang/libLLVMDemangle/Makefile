# $OpenBSD: Makefile,v 1.2 2017/07/09 15:28:35 espie Exp $

LIB=	LLVMDemangle
NOPROFILE=

CPPFLAGS+=	-I${LLVM_SRCS}/include/llvm/Demangle

.include <bsd.own.mk>
SRCS=	ItaniumDemangle.cpp

.PATH:	${.CURDIR}/../../../llvm/lib/Demangle

install:
	@# Nothing here so far ...

.include <bsd.lib.mk>

ARLIBNAME=lib${LIB}.so
${ARLIBNAME}.a: ${SOBJS}
	ar cqD ${ARLIBNAME}.a ${SOBJS}

all: lib${LIB}.a lib${LIB}.so.a

CLEANFILES += ${ARLIBNAME}.a
