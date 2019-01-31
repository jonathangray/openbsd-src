# $OpenBSD: Makefile,v 1.4 2019/01/27 16:59:41 patrick Exp $

LIB=	LLVMGlobalISel
NOPROFILE=

.include <bsd.own.mk>
SRCS=	CallLowering.cpp \
	GlobalISel.cpp \
	IRTranslator.cpp \
	InstructionSelect.cpp \
	InstructionSelector.cpp \
	LegalityPredicates.cpp \
	LegalizeMutations.cpp \
	Legalizer.cpp \
	LegalizerHelper.cpp \
	LegalizerInfo.cpp \
	Localizer.cpp \
	MachineIRBuilder.cpp \
	RegBankSelect.cpp \
	RegisterBank.cpp \
	RegisterBankInfo.cpp \
	Utils.cpp

.PATH:	${.CURDIR}/../../../llvm/lib/CodeGen/GlobalISel

install:
	@# Nothing here so far ...

.include <bsd.lib.mk>

ARLIBNAME=lib${LIB}.so
${ARLIBNAME}.a: ${SOBJS}
	ar cqD ${ARLIBNAME}.a ${SOBJS}

all: lib${LIB}.a lib${LIB}.so.a

CLEANFILES += ${ARLIBNAME}.a
