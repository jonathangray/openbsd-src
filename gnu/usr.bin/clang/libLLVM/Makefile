# $OpenBSD$

LIB=	LLVM
NOPROFILE=

.include <bsd.own.mk>
SSRCS=	\
	libLLVMAArch64AsmParser \
	libLLVMAArch64AsmPrinter \
	libLLVMAArch64CodeGen \
	libLLVMAArch64Desc \
	libLLVMAArch64Disassembler \
	libLLVMAArch64Info \
	libLLVMAArch64Utils \
	libLLVMAMDGPUAsmParser \
	libLLVMAMDGPUAsmPrinter \
	libLLVMAMDGPUCodeGen \
	libLLVMAMDGPUDesc \
	libLLVMAMDGPUDisassembler \
	libLLVMAMDGPUInfo \
	libLLVMAMDGPUUtils \
	libLLVMARMAsmParser \
	libLLVMARMAsmPrinter \
	libLLVMARMCodeGen \
	libLLVMARMDesc \
	libLLVMARMDisassembler \
	libLLVMARMInfo \
	libLLVMARMUtils \
	libLLVMAnalysis \
	libLLVMAsmParser \
	libLLVMAsmPrinter \
	libLLVMBinaryFormat \
	libLLVMBitReader \
	libLLVMBitWriter \
	libLLVMCodeGen \
	libLLVMCore \
	libLLVMCoroutines \
	libLLVMCoverage \
	libLLVMDebugInfoCodeView \
	libLLVMDebugInfoDWARF \
	libLLVMDebugInfoMSF \
	libLLVMDebugInfoPDB \
	libLLVMDemangle \
	libLLVMExecutionEngine \
	libLLVMGlobalISel \
	libLLVMIRReader \
	libLLVMInstCombine \
	libLLVMInstrumentation \
	libLLVMLTO \
	libLLVMLinker \
	libLLVMMC \
	libLLVMMCDisassembler \
	libLLVMMCParser \
	libLLVMMIRParser \
	libLLVMMipsAsmParser \
	libLLVMMipsAsmPrinter \
	libLLVMMipsCodeGen \
	libLLVMMipsDesc \
	libLLVMMipsDisassembler \
	libLLVMMipsInfo \
	libLLVMObjCARCOpts \
	libLLVMObject \
	libLLVMOption \
	libLLVMPasses \
	libLLVMPowerPCAsmParser \
	libLLVMPowerPCAsmPrinter \
	libLLVMPowerPCCodeGen \
	libLLVMPowerPCDesc \
	libLLVMPowerPCDisassembler \
	libLLVMPowerPCInfo \
	libLLVMProfileData \
	libLLVMScalarOpts \
	libLLVMSelectionDAG \
	libLLVMSparcAsmParser \
	libLLVMSparcAsmPrinter \
	libLLVMSparcCodeGen \
	libLLVMSparcDesc \
	libLLVMSparcDisassembler \
	libLLVMSparcInfo \
	libLLVMSupport \
	libLLVMTableGen \
	libLLVMTarget \
	libLLVMTransformUtils \
	libLLVMVectorize \
	libLLVMX86AsmParser \
	libLLVMX86AsmPrinter \
	libLLVMX86CodeGen \
	libLLVMX86Desc \
	libLLVMX86Disassembler \
	libLLVMX86Info \
	libLLVMX86Utils \
	libLLVMipo

SOBJS=
.for s in ${SSRCS}
.if exists(${.OBJDIR}/../lib${s}/lib${s}.so.0.0.a)
DPADD=${s}.list
LDADD+=`cat ${s}.list`

${s}.list: ${.OBJDIR}/../lib${s}/lib${s}.so.0.0.a
	ar t ${.OBJDIR}/../lib${s}/lib${s}.so.0.0.a | grep -v ^.ldadd | sed s,^,${.OBJDIR}/../lib${s}/, > ${.TARGET}
.endif
.endfor

libLLVM.a:
	@# Nothing here so far ...

install:
	@# Nothing here so far ...

.include <bsd.lib.mk>
