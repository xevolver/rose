#ifndef ROSE_DispatcherM68k_H
#define ROSE_DispatcherM68k_H

#include "BaseSemantics2.h"

namespace rose {
namespace BinaryAnalysis {
namespace InstructionSemantics2 {

/** Shared-ownership pointer to an M68k instruction dispatcher. See @ref heap_object_shared_ownership. */
typedef boost::shared_ptr<class DispatcherM68k> DispatcherM68kPtr;

class DispatcherM68k: public BaseSemantics::Dispatcher {
protected:
    // prototypical constructor
    DispatcherM68k(): BaseSemantics::Dispatcher(32, RegisterDictionary::dictionary_coldfire_emac()) {}

    DispatcherM68k(const BaseSemantics::RiscOperatorsPtr &ops, size_t addrWidth, const RegisterDictionary *regs)
        : BaseSemantics::Dispatcher(ops, addrWidth, regs ? regs : RegisterDictionary::dictionary_coldfire_emac()) {
        ASSERT_require(32==addrWidth);
        regcache_init();
        iproc_init();
        memory_init();
    }

    /** Loads the iproc table with instruction processing functors. This normally happens from the constructor. */
    void iproc_init();

    /** Load the cached register descriptors.  This happens at construction and on set_register_dictionary() calls. */
    void regcache_init();

    /** Make sure memory is set up correctly. For instance, byte order should be big endian. */
    void memory_init();

public:
    /** Cached register.
     *
     *  This register is cached so that there are not so many calls to Dispatcher::findRegister(). Changing the register
     *  dictionary via set_register_dictionary() invalidates all entries of the cache.
     *
     * @{ */
    RegisterDescriptor REG_D[8], REG_A[8], REG_FP[8], REG_PC, REG_CCR, REG_CCR_C, REG_CCR_V, REG_CCR_Z, REG_CCR_N, REG_CCR_X;
    RegisterDescriptor REG_MACSR_SU, REG_MACSR_FI, REG_MACSR_N, REG_MACSR_Z, REG_MACSR_V, REG_MACSR_C, REG_MAC_MASK;
    RegisterDescriptor REG_MACEXT0, REG_MACEXT1, REG_MACEXT2, REG_MACEXT3, REG_SSP, REG_SR_S, REG_SR, REG_VBR;
    // Floating-point condition code bits
    RegisterDescriptor REG_FPCC_NAN, REG_FPCC_I, REG_FPCC_Z, REG_FPCC_N;
    // Floating-point status register exception bits
    RegisterDescriptor REG_EXC_BSUN, REG_EXC_OPERR, REG_EXC_OVFL, REG_EXC_UNFL, REG_EXC_DZ, REG_EXC_INAN;
    RegisterDescriptor REG_EXC_IDE, REG_EXC_INEX;
    // Floating-point status register accrued exception bits
    RegisterDescriptor REG_AEXC_IOP, REG_AEXC_OVFL, REG_AEXC_UNFL, REG_AEXC_DZ, REG_AEXC_INEX;
    /** @} */

    /** Construct a prototypical dispatcher.  The only thing this dispatcher can be used for is to create another dispatcher
     *  with the virtual @ref create method. */
    static DispatcherM68kPtr instance() {
        return DispatcherM68kPtr(new DispatcherM68k);
    }

    /** Constructor. */
    static DispatcherM68kPtr instance(const BaseSemantics::RiscOperatorsPtr &ops, size_t addrWidth,
                                      const RegisterDictionary *regs=NULL) {
        return DispatcherM68kPtr(new DispatcherM68k(ops, addrWidth, regs));
    }

    /** Virtual constructor. */
    virtual BaseSemantics::DispatcherPtr create(const BaseSemantics::RiscOperatorsPtr &ops, size_t addrWidth=0,
                                                const RegisterDictionary *regs=NULL) const ROSE_OVERRIDE {
        if (0==addrWidth)
            addrWidth = addressWidth();
        if (!regs)
            regs = get_register_dictionary();
        return instance(ops, addrWidth, regs);
    }

    /** Dynamic cast to DispatcherM68kPtr with assertion. */
    static DispatcherM68kPtr promote(const BaseSemantics::DispatcherPtr &d) {
        DispatcherM68kPtr retval = boost::dynamic_pointer_cast<DispatcherM68k>(d);
        ASSERT_not_null(retval);
        return retval;
    }

    virtual void set_register_dictionary(const RegisterDictionary *regdict) ROSE_OVERRIDE;

    virtual RegisterDescriptor instructionPointerRegister() const ROSE_OVERRIDE;

    virtual RegisterDescriptor stackPointerRegister() const ROSE_OVERRIDE;

    virtual int iproc_key(SgAsmInstruction *insn_) const ROSE_OVERRIDE {
        SgAsmM68kInstruction *insn = isSgAsmM68kInstruction(insn_);
        ASSERT_not_null(insn);
        return insn->get_kind();
    }

    virtual BaseSemantics::SValuePtr read(SgAsmExpression*, size_t value_nbits, size_t addr_nbits=0) ROSE_OVERRIDE;

    /** Determines if an instruction should branch. */
    BaseSemantics::SValuePtr condition(M68kInstructionKind, BaseSemantics::RiscOperators*);

    /** Update accrued floating-point exceptions. */
    void accumulateFpExceptions();

    /** Set floating point condition codes according to result. */
    void adjustFpConditionCodes(const BaseSemantics::SValuePtr &result, SgAsmFloatType*);
};

} // namespace
} // namespace
} // namespace

#endif
