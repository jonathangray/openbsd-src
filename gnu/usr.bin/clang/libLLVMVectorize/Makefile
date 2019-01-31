# $OpenBSD: Makefile,v 1.6 2019/01/27 16:59:42 patrick Exp $

LIB=	LLVMVectorize
NOPROFILE=

CPPFLAGS+=	-I${LLVM_SRCS}/include/llvm/Transforms

.include <bsd.own.mk>
SRCS=	LoadStoreVectorizer.cpp \
	LoopVectorizationLegality.cpp \
	LoopVectorize.cpp \
	SLPVectorizer.cpp \
	VPlan.cpp \
	VPlanHCFGBuilder.cpp \
	VPlanVerifier.cpp \
	Vectorize.cpp

.PATH:	${.CURDIR}/../../../llvm/lib/Transforms/Vectorize

install:
	@# Nothing here so far ...

.include <bsd.lib.mk>

ARLIBNAME=lib${LIB}.so
${ARLIBNAME}.a: ${SOBJS}
	ar cqD ${ARLIBNAME}.a ${SOBJS}

all: lib${LIB}.a lib${LIB}.so.a

CLEANFILES += ${ARLIBNAME}.a
