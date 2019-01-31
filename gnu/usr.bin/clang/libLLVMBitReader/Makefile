# $OpenBSD: Makefile,v 1.3 2017/07/09 15:28:34 espie Exp $

LIB=	LLVMBitReader
NOPROFILE=

CPPFLAGS+=	-I${LLVM_SRCS}/include/llvm/Bitcode

.include <bsd.own.mk>
SRCS=	BitReader.cpp \
	BitcodeReader.cpp \
	BitstreamReader.cpp \
	MetadataLoader.cpp \
	ValueList.cpp

.PATH:	${.CURDIR}/../../../llvm/lib/Bitcode/Reader

install:
	@# Nothing here so far ...

.include <bsd.lib.mk>

ARLIBNAME=lib${LIB}.so
${ARLIBNAME}.a: ${SOBJS}
	ar cqD ${ARLIBNAME}.a ${SOBJS}

all: lib${LIB}.a lib${LIB}.so.a

CLEANFILES += ${ARLIBNAME}.a
