# $OpenBSD: Makefile,v 1.3 2017/07/09 15:28:35 espie Exp $

LIB=	LLVMTarget
NOPROFILE=

CPPFLAGS+=	-I${LLVM_SRCS}/include/llvm/Target

.include <bsd.own.mk>
SRCS=	Target.cpp \
	TargetIntrinsicInfo.cpp \
	TargetLoweringObjectFile.cpp \
	TargetMachine.cpp \
	TargetMachineC.cpp

.PATH:	${.CURDIR}/../../../llvm/lib/Target

install:
	@# Nothing here so far ...

.include <bsd.lib.mk>

ARLIBNAME=lib${LIB}.so
${ARLIBNAME}.a: ${SOBJS}
	ar cqD ${ARLIBNAME}.a ${SOBJS}

all: lib${LIB}.a lib${LIB}.so.a

CLEANFILES += ${ARLIBNAME}.a
