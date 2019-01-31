# $OpenBSD: Makefile,v 1.2 2017/07/09 15:28:34 espie Exp $

LIB=	LLVMAsmParser
NOPIC=
NOPROFILE=

CPPFLAGS+=	-I${LLVM_SRCS}/include/llvm/Analysis

SRCS+=	LLLexer.cpp \
	LLParser.cpp \
	Parser.cpp

.PATH:	${.CURDIR}/../../../llvm/lib/AsmParser

install:
	@# Nothing here so far ...
