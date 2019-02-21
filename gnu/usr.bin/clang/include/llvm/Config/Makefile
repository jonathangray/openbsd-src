# $OpenBSD: Makefile,v 1.4 2019/03/05 06:04:32 jsg Exp $

.include <bsd.own.mk>

.include "../../../Makefile.arch"

DEFS=	AsmParsers.def AsmPrinters.def Disassemblers.def Targets.def

HDRS=	abi-breaking.h llvm-config.h
INCDIR=	/usr/include/llvm/Config

all: ${DEFS}

clean cleandir:
	rm -f ${DEFS}

AsmParsers.def:
	printf "LLVM_ASM_PARSER(${LLVM_ARCH})\n#undef LLVM_ASM_PARSER\n" >$@
AsmPrinters.def:
	printf "LLVM_ASM_PRINTER(${LLVM_ARCH})\n#undef LLVM_ASM_PRINTER\n" >$@
Disassemblers.def:
	printf "LLVM_DISASSEMBLER(${LLVM_ARCH})\n#undef LLVM_DISASSEMBLER\n" >$@
Targets.def:
	printf "LLVM_TARGET(AMDGPU)\n" >$@
	printf "LLVM_TARGET(${LLVM_ARCH})\n#undef LLVM_TARGET\n" >>$@

install includes: ${DEFS}
	${INSTALL} -d -m 755 ${DESTDIR}${INCDIR}
	@cd ${.CURDIR}; for i in $(HDRS); do \
	    j="cmp -s $$i ${DESTDIR}${INCDIR}/$$i || \
		${INSTALL} ${INSTALL_COPY} -o ${BINOWN} -g ${BINGRP} \
		-m 444 $$i ${DESTDIR}${INCDIR}"; \
	    echo $$j; \
	    eval "$$j"; \
	done
	@cd ${.OBJDIR}; for i in $(DEFS); do \
	    j="cmp -s $$i ${DESTDIR}${INCDIR}/$$i || \
		${INSTALL} ${INSTALL_COPY} -o ${BINOWN} -g ${BINGRP} \
		-m 444 $$i ${DESTDIR}${INCDIR}"; \
	    echo $$j; \
	    eval "$$j"; \
	done

.include <bsd.obj.mk>
