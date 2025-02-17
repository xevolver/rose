#ifndef ROSE_DISASSEMBLER_H
#define ROSE_DISASSEMBLER_H

#include "BinaryCallingConvention.h"
#include "Diagnostics.h"                                // rose::Diagnostics
#include "Registers.h"
#include "MemoryMap.h"
#include "integerOps.h"
#include "Map.h"
#include "BaseSemantics2.h"

namespace rose {
namespace BinaryAnalysis {


/** Virtual base class for instruction disassemblers.
 *
 *  The Disassembler class is a virtual class providing all non-architecture-specific functionality for the recursive
 *  disassembly of instructions; architecture-specific components are in subclasses such as DisassemblerArm,
 *  DisassemblerPowerpc, and DisassemblerX86. In general, there is no need to explicitly instantiate or call functions in any
 *  of these subclasses.
 *
 *  All details of the instructions, and the operands and operator expression trees, etc. are stored in the binary AST as
 *  separate IR nodes.  The SgAsmInstruction class and its architecture-specific subclasses represent individual instructions.
 *  The arguments for those instructions are represented by the SgAsmExpression class and subclasses thereof.
 *
 *  Disassembly normally happens automatically unless the -rose:read_executable_file_format_only switch is specified.
 *  Alternatively, this Disassembler class can be used to explicitly disassemble parts of a file. The Disassembler class
 *  handles all non-architecture-specific details of disassembly, such as where to search for instructions in the address
 *  space and how instructions are concatenated into basic blocks.  The Disassembler has a pure virtual method,
 *  disassembleOne(), that is implemented by architecture-specific subclasses and whose purpose is to disassemble one
 *  instruction.
 *
 *  New architectures can be added to ROSE without modifying any ROSE source code. One does this by subclassing an existing
 *  disassembler, overriding any necessary virtual methods, and registering an instance of this subclass with
 *  Disassembler::register_subclass().  If the new subclass can handle multiple architectures then a disassembler is
 *  registered for each of those architectures.
 *
 *  When ROSE needs to disassemble something, it calls Disassembler::lookup(), which in turn calls the can_disassemble()
 *  method for all registered disassemblers.  The first disassembler whose can_disassemble() returns true is used for the
 *  disassembly.
 *
 *  If an error occurs during the disassembly of a single instruction, the disassembler will throw an exception. When
 *  disassembling multiple instructions the exceptions are saved in a map, by virtual address, and the map is returned to the
 *  caller along with the instructions that were successfully disassembled.
 *
 *  The main interface to the disassembler is the disassembleBuffer() method. It searches for instructions based on the
 *  heuristics specified in the set_search() method, reading instruction bytes from a supplied buffer.  A MemoryMap object is
 *  supplied in order to specify a mapping from virtual address space to offsets in the supplied buffer. The
 *  disassembleBuffer() method is used by methods that disassemble whole sections, whole interpretations, or whole files; in
 *  turn, it calls disassembleBlock() which disassembles sequential instructions until a control flow branch is encountered.
 *
 *  A MemoryMap object can be built that describes the entire virtual address space and how it relates to offsets in the
 *  executable file.  This object, together with the entire contents of the file, can be passed to the disassembleBuffer()
 *  method in order to disassemble the entire executable in one call.  However, if the executable contains multiple
 *  independent interpretations (like a PE file that contains a Windows executable and a DOS executable) then the best
 *  practice is to disassemble each interpretation individually.  The disassemble() method is convenient for this.
 *  Disassemblers only disassemble memory that is marked with the protections returned by get_protections(); normally the
 *  execute bit must be set.
 *
 *  While the main purpose of the Disassembler class is to disassemble instructions, it also needs to be able to group those
 *  instructions into basic blocks (SgAsmBlock) and functions (SgAsmFunction). It uses an instance of the
 *  Partitioner class to do so.  The user can supply a partitioner to the disassembler or have the disassembler create a
 *  default partitioner on the fly.  The user is also free to call the partitioner directly on the InstructionMap object
 *  returned by most of the disassembler methods. As described in the Partitioner documentation, the output of a Disassembler
 *  can be provided as input to the Partitioner, or the Partitioner can be configured to call the Disassembler as needed.
 *
 *  For example, the following code disassembles every single possible address of all bytes that are part of an executable
 *  ELF Segment but which are not defined as any other known part of the file (not a table, ELF Section, header, etc.). By
 *  building a map and making a single disassembly call, we're able to handle instructions that span segments, where the bytes
 *  of the instruction are not contiguous in the file.
 *
 *  @code
 *  SgAsmGenericHeader *header = ....; // the ELF file header
 *
 *  // Create a memory map
 *  Loader *loader = Loader::find_loader(header);
 *  MemoryMap *map = loader->map_executable_sections(header);
 *
 *  // Disassemble everything defined by the memory map
 *  Disassembler *d = Disassembler::lookup(header)->clone();
 *  d->set_search(Disassembler::SEARCH_ALLBYTES); // disassemble at every address
 *  Disassembler::AddressSet worklist; // can be empty due to SEARCH_ALLBYTES
 *  Disassembler::InstructionMap insns;
 *  insns = d->disassembleBuffer(map, worklist);
 *
 *  // Print all instructions
 *  Disassembler::InstructionMap::iterator ii;
 *  for (ii=insns.begin(); ii!=insns.end(); ++ii)
 *      std::cout <<unparseInstructionWithAddress(ii->second) <<std::endl;
 *  @endcode
 *
 *  The following example shows how one can influence how ROSE disassembles instructions. Let's say you have a problem with
 *  the way ROSE is partitioning instructions into functions: it's applying the instruction pattern detector too aggressively
 *  when disassembling x86 programs, so you want to turn it off.  Here's how you would do that. First, create a subclass of
 *  the Disassembler you want to influence.
 *
 *  @code
 *  class MyDisassembler: public DisassemblerX86 {
 *  public:
 *      MyDisassembler(size_t wordsize): DisassemblerX86(wordsize) {
 *          Partitioner *p = new Partitioner();
 *          unsigned h = p->get_heuristics();
 *          7 &= ~SgAsmFunction::FUNC_PATTERN;
 *          p->set_heuristics(h);
 *          set_partitioner(p);
 *      }
 *  };
 *  @endcode
 *
 *  Then the new disassemblers are registered with ROSE:
 *
 *  @code
 *  Disassembler::register_subclass(new MyDisassembler(4)); // 32-bit
 *  Disassembler::register_subclass(new MyDisassembler(8)); // 64-bit
 *  @endcode
 *
 *  Additional examples are shown in the disassembleBuffer.C and disassemble.C sources in the tests/roseTests/binaryTests
 *  directory. They are examples of how a Disassembler object can be used to disassemble a buffer containing bare machine code
 *  when one doesn't have an associated executable file.
 */
class Disassembler {
public:
    /** Exception thrown by the disassemblers. */
    class Exception: public std::runtime_error {
    public:
        /** A bare exception not bound to any particular instruction. */
        Exception(const std::string &reason)
            : std::runtime_error(reason), ip(0), bit(0), insn(NULL)
            {}
        /** An exception bound to a virtual address but no raw data or instruction. */
        Exception(const std::string &reason, rose_addr_t ip)
            : std::runtime_error(reason), ip(ip), bit(0), insn(NULL)
            {}
        /** An exception bound to a particular instruction being disassembled. */
        Exception(const std::string &reason, rose_addr_t ip, const SgUnsignedCharList &raw_data, size_t bit)
            : std::runtime_error(reason), ip(ip), bytes(raw_data), bit(bit), insn(NULL)
            {}
        /** An exception bound to a particular instruction being assembled. */
        Exception(const std::string &reason, SgAsmInstruction *insn)
            : std::runtime_error(reason), ip(insn->get_address()), bit(0), insn(insn)
            {}
        ~Exception() throw() {}

        void print(std::ostream&) const;
        friend std::ostream& operator<<(std::ostream &o, const Exception &e);

        rose_addr_t ip;                 /**< Virtual address where failure occurred; zero if no associated instruction */
        SgUnsignedCharList bytes;       /**< Bytes (partial) of failed disassembly, including byte at failure. Empty if the
                                         *   exception is not associated with a particular byte sequence, such as if an
                                         *   attempt was made to disassemble at an invalid address. */
        size_t bit;                     /**< Bit offset in instruction byte sequence where disassembly failed (bit/8 is the
                                         *   index into the "bytes" list, while bit%8 is the bit within that byte. */
        SgAsmInstruction *insn;         /**< Instruction associated with an assembly error. */
    };

    /** Heuristics used to find instructions to disassemble. The set of heuristics to try can be set by calling the
     *  set_search() method prior to disassembling. Unless noted otherwise, the bit flags affect the disassembleBuffer methods,
     *  which call the basic block and individual instruction disassembler methods, but which are called by the methods that
     *  disassemble sections, headers, and entire files. */
    enum SearchHeuristic 
        {
        SEARCH_FOLLOWING = 0x0001,      /**< Disassemble at the address that follows each disassembled instruction, regardless
                                         *   of whether the following address is a successor. */
        SEARCH_IMMEDIATE = 0x0002,      /**< Disassemble at the immediate operands of other instructions.  This is a
                                         *   rudimentary form of constant propagation in order to better detect branch targets
                                         *   on RISC architectures where the target is first loaded into a register and then
                                         *   the branch is based on that register. */
        SEARCH_WORDS     = 0x0004,      /**< Like IMMEDIATE, but look at all word-aligned words in the disassembly regions.
                                         *   The word size, alignment, and byte order can be set explicitly; defaults are
                                         *   based on values contained in the executable file header supplied during
                                         *   construction. */
        SEARCH_ALLBYTES  = 0x0008,      /**< Disassemble starting at every possible address, even if that address is inside
                                         *   some other instruction. */
        SEARCH_UNUSED    = 0x0010,      /**< Disassemble starting at every address not already part of an instruction. The way
                                         *   this works is that the disassembly progresses as usual, and then this heuristic
                                         *   kicks in at the end in order to find addresses that haven't been disassembled.
                                         *   The search progresses from low to high addresses and is influenced by most of the
                                         *   other search heuristics as it progresses.  For instance, if SEARCH_DEADEND is not
                                         *   set then disassembly at address V is considered a failure if the basic block at
                                         *   V leads to an invalid instruction. */
        SEARCH_NONEXE    = 0x0020,      /**< Disassemble in sections that are not mapped executable. This heuristic only
                                         *   applies to disassembly methods that choose which sections to disassemble (for
                                         *   instance, disassembleInterp()).  The normal behavior is to disassemble sections
                                         *   that are known to contain code or are mapped with execute permission; specifying
                                         *   this heuristic also includes sections that are mapped but don't have execute
                                         *   permission.  It is not possible to disassemble non-mapped sections automatically
                                         *   since they don't have virtual addresses, but it is possible to supply your own
                                         *   virtual address mapping to one of the lower-level disassembler methods in order
                                         *   to accomplish this. */
        SEARCH_DEADEND   = 0x0040,      /**< Include basic blocks that end with a bad instruction. This is used by the methods
                                         *   that disassemble basic blocks (namely, any disassembly method other than
                                         *   disassembleOne()).  Normally, if disassembly of a basic block at virtual address
                                         *   V leads to a bad instruction, the entire basic block is discarded and address V
                                         *   is added to the bad address list (address V+1 is still available for the other
                                         *   heuristics). */
        SEARCH_UNKNOWN   = 0x0080,      /**< Rather than generating exceptions when an instruction cannot be disassembled,
                                         *   create a pseudo-instruction that occupies one byte. These will generally have
                                         *   opcodes like x86_unknown_instruction. */
        SEARCH_FUNCSYMS  = 0x0100,      /**< Disassemble beginning at every address that corresponds to the value of a
                                         *   function symbol. This heuristic only applies to disassembly methods called at the
                                         *   interpretation (SgAsmInterpretation) or header (SgAsmGenericHeader) level or
                                         *   above since that's where the symbols are grafted onto the abstract symbol tree. */
        SEARCH_DEFAULT   = 0x0101       /**< Default set of heuristics to use. The default is to be moderately timid. */
    };

    /** Given a string (presumably from the ROSE command-line), parse it and return the bit vector describing which search
     *  heuristics should be employed by the disassembler.  The input string should be a comma-separated list (without white
     *  space) of search specifications. Each specification should be an optional qualifier character followed by either an
     *  integer or a word. The accepted words are the lower-case versions of the constants enumerated by SearchHeuristic, but
     *  without the leading "SEARCH_".  The qualifier determines whether the bits specified by the integer or word are added
     *  to the return value ("+") or removed from the return value ("-").  The "=" qualifier acts like "+" but first zeros the
     *  return value. The default qualifier is "+" except when the word is "default", in which case the specifier is "=". An
     *  optional initial bit mask can be specified (defaults to SEARCH_DEFAULT). */
    static unsigned parse_switches(const std::string &s, unsigned initial=SEARCH_DEFAULT);

    /** An AddressSet contains virtual addresses (alternatively, relative virtual addresses) for such things as specifying
     *  which virtual addresses should be disassembled. */
    typedef std::set<rose_addr_t> AddressSet;

    /** The InstructionMap is a mapping from (absolute) virtual address to disassembled instruction. */
    typedef Map<rose_addr_t, SgAsmInstruction*> InstructionMap;

    /** The BadMap is a mapping from (absolute) virtual address to information about a failed disassembly attempt at that
     *  address. */
    typedef Map<rose_addr_t, Exception> BadMap;

    Disassembler()
        : p_registers(NULL), p_partitioner(NULL), p_search(SEARCH_DEFAULT),
          p_wordsize(4), p_sex(ByteOrder::ORDER_LSB), p_alignment(4), p_ndisassembled(0),
          p_protection(MemoryMap::EXECUTABLE)
        {ctor();}

    virtual ~Disassembler() {}


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                                  Data members
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    CallingConvention::Dictionary callingConventions_;

protected:
    const RegisterDictionary *p_registers;              /**< Description of registers available for this platform. */
    RegisterDescriptor REG_IP, REG_SP, REG_SS;          /**< Register descriptors initialized during construction. */
    class Partitioner *p_partitioner;                   /**< Used for placing instructions into blocks and functions. */
    unsigned p_search;                                  /**< Mask of SearchHeuristic bits specifying instruction searching. */
    size_t p_wordsize;                                  /**< Word size used by SEARCH_WORDS. */
    ByteOrder::Endianness p_sex;                        /**< Byte order for SEARCH_WORDS. */
    size_t p_alignment;                                 /**< Word alignment constraint for SEARCH_WORDS (0 and 1 imply byte). */
    static std::vector<Disassembler*> disassemblers;    /**< List of disassembler subclasses. */
    size_t p_ndisassembled;                             /**< Total number of instructions disassembled by disassembleBlock() */
    unsigned p_protection;                              /**< Memory protection bits that must be set to disassemble. */
    static double progress_interval;                    /**< Minimum interval between progress reports in seconds. */
    static double progress_time;                        /**< Time of last report, or zero if no report has been generated. */

    /** Prototypical dispatcher for creating real dispatchers */
    InstructionSemantics2::BaseSemantics::DispatcherPtr p_proto_dispatcher;

public:
    static Sawyer::Message::Facility mlog;              /**< Disassembler diagnostic streams. */

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                                  Registration and lookup methods
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

public:
    /** Register a disassembler instance. More specific disassembler instances should be registered after more general
     *  disassemblers since the lookup() method will inspect disassemblers in reverse order of their registration.
     *
     *  Thread safety: Multiple threads can register disassemblers simultaneously.  However, one seldom does this because the
     *  order that disassemblers are registered determines which disassembler is returned by the lookup() class methods. */
    static void register_subclass(Disassembler*);

    /** Predicate determining the suitability of a disassembler for a specific file header.  If this disassembler is capable
     *  of disassembling machine code described by the specified file header, then this predicate returns true, otherwise it
     *  returns false.
     *
     *  Thread safety: The thread safety of this virtual method depends on the implementation in the subclass. */
    virtual bool can_disassemble(SgAsmGenericHeader*) const = 0;

    /** Finds a suitable disassembler. Looks through the list of registered disassembler instances (from most recently
     *  registered to earliest registered) and returns the first one whose can_disassemble() predicate returns true.  Throws
     *  an exception if no suitable disassembler can be found.
     *
     *  Thread safety: Multiple threads can call this class method simultaneously even when other threads are registering
     *  additional disassemblers. */
    static Disassembler *lookup(SgAsmGenericHeader*);

    /** List of names recognized by @ref lookup.
     *
     *  Returns the list of names that the @ref lookup method recognizes. */
    static std::vector<std::string> isaNames();
    
    /** Finds a suitable disassembler. Looks through the list of registered disassembler instances (from most recently
     *  registered to earliest registered) and returns the first one whose can_disassemble() predicate returns true. This is
     *  done for each header contained in the interpretation and the disassembler for each header must match the other
     *  headers. An exception is thrown if no suitable disassembler can be found.
     *
     *  Thread safety: Multiple threads can call this class method simultaneously even when other threads are registering
     *  additional disassembles. However, no other thread can be changing attributes of the specified interpretation,
     *  particularly the list of file headers referenced by the interpretation. */
    static Disassembler *lookup(SgAsmInterpretation*);

    /** Finds a suitable disassembler.  Looks up a common disassembler by name.  If the name is the word "list" then a
     *  list of known names is printed to <code>std::cout</code>. */
    static Disassembler *lookup(const std::string&);

    /** Creates a new copy of a disassembler. The new copy has all the same settings as the original.
     *
     *  Thread safety: The thread safety of this virtual method depends on the implementation in the subclass. */
    virtual Disassembler *clone() const = 0;


    /***************************************************************************************************************************
     *                                  Main public disassembly methods
     ***************************************************************************************************************************/
public:
    /** This high-level method disassembles instructions belonging to part of a file described by an executable file header as
     *  indicated by the specified interpretation.  The disassembleInterp() method is called for the main disassembly work,
     *  then a partitioner is invoked to create functions and basic blocks, then the nodes are linked into the AST.
     *
     *  The heuristics used to partition instructions into functions, and the aggressiveness of the disassembler in finding
     *  instructions can be controlled by setting properties of this Disassembler object.  The MemoryMap describing how
     *  virtual memory to be disassembled is mapped into the binary files that ROSE is parsing is either taken from the map
     *  defined by the SgAsmInterpretation::p_map, or a new map is created by calling various Loaders and then saved into
     *  SgAsmInterpretation::p_map.
     *
     *  Addresses containing instructions that could not be disassembled are added to the optional @p bad map.  Successor
     *  addresses where no disassembly was attempted are added to the optional successors set.
     *
     *  In essence, this method replaces the old disassembleInterpretation function in the old Disassembler name space, and
     *  will probably be the method most often called by other parts of ROSE.  All of its functionality is based on the other
     *  lower-level methods of this class.
     *
     *  Thread safety: It is not safe for two or more threads to invoke this method on the same Disassembler object. */
    void disassemble(SgAsmInterpretation*, AddressSet *successors=NULL, BadMap *bad=NULL);

    /** This class method is for backward compatibility with the disassembleInterpretation() function in the old Disassembler
     *  namespace. It just creates a default Disassembler object, sets its search heuristics to the value specified in the
     *  SgFile node above the interpretation (presumably the value set with ROSE's "-rose:disassembler_search" switch),
     *  and invokes the disassemble() method.
     *
     *  See also, Partitioner::disassembleInterpretation() which does Partitioner-driven disassembly.
     *
     *  Thread safety: This class method is not thread safe. */
    static void disassembleInterpretation(SgAsmInterpretation*);



    /***************************************************************************************************************************
     *                                          Disassembler properties and settings
     ***************************************************************************************************************************/
public:
    /** Specifies the registers available on this architecture.  Rather than using hard-coded class, number, and position
     *  constants, the disassembler should perform a name lookup in this supplied register dictionary and use the values found
     *  therein. There's usually no need for the user to specify a value because either it will be obtained from an
     *  SgAsmInterpretation or the subclass will initialize it.
     *
     *  Thread safety: It is not safe to change the register dictionary while another thread is using this same Disassembler
     *  object. */
    void set_registers(const RegisterDictionary *rdict) {
        p_registers = rdict;
    }

    /** Returns the dictionary used for looking up register names.
     *
     *  Thread safety: This method is thread safe. */
    const RegisterDictionary *get_registers() const {
        return p_registers;
    }

    /** Property: Calling convention dictionary.
     *
     *  This is a dictionary of the common calling conventions for this architecture.
     *
     * @{ */
    const CallingConvention::Dictionary& callingConventions() const { return callingConventions_; }
    CallingConvention::Dictionary& callingConventions() { return callingConventions_; }
    void callingConventions(const CallingConvention::Dictionary &d) { callingConventions_ = d; }
    /** @} */

    /** Returns the register that points to instructions. */
    virtual RegisterDescriptor instructionPointerRegister() const {
        ASSERT_require(REG_IP.is_valid());
        return REG_IP;
    }

    /** Returns the register that points to the stack. */
    virtual RegisterDescriptor stackPointerRegister() const {
        ASSERT_require(REG_SP.is_valid());
        return REG_SP;
    }

    /** Returns the segment register for accessing the stack.  Not all architectures have this register, in which case the
     * default-constructed register descriptor is returned. */
    virtual RegisterDescriptor stackSegmentRegister() const {
        return REG_SS;                                  // need not be valid
    }

    /** Return an instruction semantics dispatcher if possible.
     *
     *  If instruction semantics are implemented for this architecure then return a pointer to a dispatcher. The dispatcher
     *  will have no attached RISC operators and can only be used to create a new dispatcher via its virtual constructor.  If
     *  instruction semantics are not implemented then the null pointer is returned. */
    const rose::BinaryAnalysis::InstructionSemantics2::BaseSemantics::DispatcherPtr& dispatcher() const {
        return p_proto_dispatcher;
    }

#ifndef USE_ROSE
  // DQ (2/11/2013): I think it is a problem to use this function (currently being evaluated using delta debug case).
  // This fails for ROSE compilign "rose.h" header file (ROSE compiling ROSE).
  // Generates error: ERROR: In parse_function_body(): (this should have been a function body) entry_kind = 0x3a4d47e8 = src-seq-sublist 

    /** Specifies the instruction partitioner to use when partitioning instructions into functions.  If none is specified then
     *  a default partitioner will be constructed when necessary.
     *
     *  Thread safety: It is not safe to change the partitioner while another thread is using this same Disassembler object. */
    void set_partitioner(class Partitioner *p) {
        p_partitioner = p;
    }
#else
    void set_partitioner(class Partitioner *p);
#endif

#ifndef USE_ROSE
    // DQ (2/11/2013): I think it is a problem to use this function (currently being evaluated using delta debug case).
    // This fails for ROSE compilign "rose.h" header file (ROSE compiling ROSE).

    /** Returns the partitioner object set by set_partitioner().
     *
     *  Thread safety: This method is thread safe. */
    class Partitioner *get_partitioner() const {
        return p_partitioner;
    }
#else
    class Partitioner *get_partitioner() const;
#endif

    /** Specifies the heuristics used when searching for instructions. The @p bits argument should be a bit mask of
     *  SearchHeuristic bits.
     *
     *  Thread safety: It is not safe to change the instruction search heuristics while another thread is using this same
     *  Disassembler object. */
    void set_search(unsigned bits) {
        p_search = bits;
    }

    /** Returns a bit mask of SearchHeuristic bits representing which heuristics would be used when searching for
     *  instructions.
     *
     *  Thread safety: This method is thread safe. */
    unsigned get_search() const {
        return p_search;
    }

    /** Property: Machine word size in bits.
     *
     *  Natural word size for the machine. This is usually the size of most registers. The default is based on the word size of
     *  the file header used when the disassembler is constructed.
     *
     *  Thread safety: It is not safe to change the instruction search word size while another thread is using this same
     *  Disassembler object.
     *
     * @{ */
    void set_wordsize(size_t);
    size_t get_wordsize() const {
        return p_wordsize;
    }
    /** @} */

    /** Specifies the alignment for the SEARCH_WORDS heuristic. The value must be a natural, postive power of two. The default
     *  is determined by the subclass (e.g., x86 would probably set this to one since instructions are not aligned, while ARM
     *  would set this to four.
     *
     *  Thread safety: It is not safe to change the instruction search alignment while another thread is using this same
     *  Disassembler object. */
    void set_alignment(size_t);

    /** Returns the alignment used by the SEARCH_WORDS heuristic.
     *
     *  Thread safety: This method is thread safe. */
    size_t get_alignment() const {
        return p_alignment;
    }

    /** Property: Memory byte order.
     *
     *  The byte order for memory. The default is based on the byte order of the file header used when the disassembler is
     *  constructed.
     *
     *  Thread safety: It is not safe to change the byte sex while another thread is using this same Disassembler object.
     *
     * @{ */
    void set_sex(ByteOrder::Endianness sex) {
        p_sex = sex;
    }
    ByteOrder::Endianness get_sex() const {
        return p_sex;
    }
    /** @} */

    /** Returns the number of instructions successfully disassembled. The counter is updated by disassembleBlock(), which is
     *  generally called by all disassembly methods except for disassembleOne().
     *
     *  Thread safety: This method is thread safe. */
    size_t get_ndisassembled() const {
        return p_ndisassembled;
    }

    /** Normally the disassembler will only read memory when the execute permission is turned on for the memory.  This can be
     *  changed to some other set of MemoryMap::Protection bits with this function. All bits that are set here must also be
     *  present for the memory being disassembled.
     *
     *  Thread safety: It is not safe to change the protection while another thread is using this same Disassembler object. */
    void set_protection(unsigned bitvec) {
        p_protection = bitvec;
    }

    /** Returns a bit vector describing which bits must be enabled in the MemoryMap in order for the disassembler to read from
     *  that memory.  The default is that MemoryMap::MM_PROT_EXEC must be set.
     *
     *  Thread safety:  This method is thread safe. */
    unsigned get_protection() const {
        return p_protection;
    }

    /** Set progress reporting properties.  A progress report is produced not more than once every @p min_interval seconds
     *  (default is 10) by sending a single line of ouput to the mlog[INFO] diagnostic stream.  Progress reporting can be
     *  disabled by supplying a negative value.  Progress report properties are class variables. Changing their
     *  values will immediately affect all disassemblers in all threads.
     *
     * Thread safety: This method is thread safe. */
    void set_progress_reporting(double min_interval);

    /** Initializes and registers disassembler diagnostic streams. See Diagnostics::initialize(). */
    static void initDiagnostics();

    /***************************************************************************************************************************
     *                                          Low-level disassembly functions
     ***************************************************************************************************************************/
public:
    /** This is the lowest level disassembly function and is implemented in the architecture-specific subclasses. It
     *  disassembles one instruction at the specified virtual address. The @p map is a mapping from virtual addresses to
     *  buffer and enables instructions to span file segments that are mapped contiguously in virtual memory by the loader but
     *  which might not be contiguous in the file.  The instruction's successor virtual addresses are added to the optional
     *  successor set (note that successors of an individual instruction can also be obtained via
     *  SgAsmInstruction::getSuccessors). If the instruction cannot be disassembled then an exception is thrown and the
     *  successors set is not modified.
     *
     *  Thread safety:  The safety of this method depends on its implementation in the subclass. In any case, no other thread
     *  can be modifying the MemoryMap or successors set at the same time. */
    virtual SgAsmInstruction *disassembleOne(const MemoryMap *map, rose_addr_t start_va, AddressSet *successors=NULL) = 0;

    /** Similar in functionality to the disassembleOne method that takes a MemoryMap argument, except the content buffer is
     *  mapped 1:1 to virtual memory beginning at the specified address.
     *
     *  Thread safety:  The safety of this method depends on the implementation of the disassembleOne() defined in the
     *  subclass. If the subclass is thread safe then this method can be called in multiple threads as long as the supplied
     *  buffer and successors set are not being modified by another thread. */
    SgAsmInstruction *disassembleOne(const unsigned char *buf, rose_addr_t buf_va, size_t buf_size, rose_addr_t start_va,
                                     AddressSet *successors=NULL);

    /** Like the disassembleOne method except it disassembles a basic block's worth of instructions beginning at the specified
     *  virtual address.  For the purposes of this function, a basic block is defined as starting from the specified
     *  instruction and continuing until we reach a branch instruction (e.g., "jmp", "je", "call", "ret", etc.), or an
     *  instruction with no known successors (e.g., "hlt", "int", etc), or the end of the buffer, or an instruction that
     *  cannot be disassembled. The @p map is a mapping from virtual addresses to offsets in the content buffer. The
     *  successors of the last instruction of the basic block are added to the optional successor set. An exception is thrown
     *  if the first instruction cannot be disassembled (or some other major error occurs), in which case the successors set
     *  is not modified.
     *
     *  If the SEARCH_DEADEND bit is set and an instruction cannot be disassembled then the address of that instruction is
     *  added to the successors and the basic block ends at the previous instruction.  If the SEARCH_DEADEND bit is clear and
     *  an instruction cannot be disassembled then the entire basic block is discarded, an exception is thrown (the exception
     *  address is the instruction that could not be disassembled), and the successors list is not modified.
     *
     *  A cache of previously disassembled instructions can be provided. If one is provided, then the cache will be updated
     *  with any instructions disassembled during the call to disassembleBlock().  Addresses where disassembly was tried but
     *  failed will be added to the cache as null pointers. This is convenient when the SEARCH_DEADEND bit is clear in
     *  conjunction with the SEARCH_FOLLOWING (or similar) being set, since this combination causes the disassembler to try
     *  every address in a dead-end block. Providing a cache in this case can speed up the disassembler by an order of
     *  magnitude.
     *
     *  Thread safety: The safety of this method depends on the implementation of disassembleOne() in the subclass. In any
     *  case, no other thread should be concurrently modifying the memory map, successors, or cache. */
    InstructionMap disassembleBlock(const MemoryMap *map, rose_addr_t start_va, AddressSet *successors=NULL,
                                    InstructionMap *cache=NULL);

    /** Similar in functionality to the disassembleBlock method that takes a MemoryMap argument, except the supplied buffer
     *  is mapped 1:1 to virtual memory beginning at the specified address.
     *
     *  Thread safety: The safety of this method depends on the implementation of disassembleOne() in the subclass. In any
     *  case, no other thread should be concurrently modifying the buffer, successors, or cache. */
    InstructionMap disassembleBlock(const unsigned char *buf, rose_addr_t buf_va, size_t buf_size, rose_addr_t start_va,
                                    AddressSet *successors=NULL, InstructionMap *cache=NULL);

    /** Disassembles instructions from the content buffer beginning at the specified virtual address and including all
     *  instructions that are direct or indirect successors of the first instruction.  The @p map is a mapping from virtual
     *  addresses to offsets in the content buffer.  Any successors of individual instructions that fall outside the buffer
     *  being disassembled will be added to the optional successors set.  If an address cannot be disassembled then the
     *  address and exception will be added to the optional @p bad map; any address which is already in the bad map upon
     *  function entry will not be disassembled. Note that bad instructions have no successors.  An exception is thrown if an
     *  error is detected before disassembly begins.
     *
     * Thread safety: The safety of this method depends on the implementation of disassembleOne() in the subclass. In any case,
     * no other thread should be concurrently modifying the memory map, successor list, or exception map. */
    InstructionMap disassembleBuffer(const MemoryMap *map, size_t start_va, AddressSet *successors=NULL, BadMap *bad=NULL);

    /** Similar in functionality to the disassembleBuffer methods that take a MemoryMap argument, except the supplied buffer
     *  is mapped 1:1 to virtual memory beginning at the specified address.
     *
     *  Thread safety:  The safety of this method depends on the implementation of disassembleOne() in the subclass. In any case,
     *  no other thread should be concurrently modifying the buffer, successor list, or exception map. */
    InstructionMap disassembleBuffer(const unsigned char *buf, rose_addr_t buf_va, size_t buf_size, rose_addr_t start_va,
                                     AddressSet *successors=NULL, BadMap *bad=NULL);

    /** Similar in functionality to the disassembleBuffer methods that take a single starting virtual address, except this one
     *  tries to disassemble from all the addresses specified in the workset.
     *
     *  Thread safety:  The safety of this method depends on the implementation of disassembleOne() in the subclass. In any case,
     *  no other thread should be concurrently modifying the memory map, successor list, or exception map. */
    InstructionMap disassembleBuffer(const MemoryMap *map, AddressSet workset, AddressSet *successors=NULL, BadMap *bad=NULL);

    /** Disassembles instructions in the specified section by assuming that it's mapped to a particular starting address.
     *  Disassembly will begin at the specified byte offset in the section. The section need not be mapped with execute
     *  permission; in fact, since a starting address is specified, it need not be mapped at all.  All other aspects of this
     *  method are similar to the disassembleBuffer method. */
    InstructionMap disassembleSection(SgAsmGenericSection *section, rose_addr_t section_va, rose_addr_t start_offset,
                                      AddressSet *successors=NULL, BadMap *bad=NULL);

    /** Disassembles instructions in a particular binary interpretation. If the interpretation has a memory map
     *  (SgAsmInterpretation::get_map()) then that map will be used for disassembly. Otherwise a new map is created to describe
     *  all code-containing sections of the header, and that map is stored in the interpretation.  The aggressiveness when
     *  searching for addresses to disassemble is controlled by the disassembler's set_search() method. If the interpretation
     *  has a register dictionary, then the disassembler's register dictionary will be set to the same value. All other aspects
     *  of this method are similar to the disassembleBuffer() method.
     *
     *  Thread safety:  Not thread safe. */
    InstructionMap disassembleInterp(SgAsmInterpretation *interp, AddressSet *successors=NULL, BadMap *bad=NULL);



    /***************************************************************************************************************************
     *                                  Methods for searching for disassembly addresses.
     ***************************************************************************************************************************/
public:
    /** Adds the address following a basic block to the list of addresses that should be disassembled.  This search method is
     *  invoked automatically if the SEARCH_FOLLOWING bit is set (see set_search()).  The @p tried argument lists all the
     *  addresses where we already tried to disassemble (including addresses where disassembly failed) and this method refuses
     *  to add the following address to the worklist if we've already tried that address.
     *
     *  Thread safety: Multiple threads can call this method using the same Disassembler object as long as they pass different
     *  worklist address sets and no other thread is modifying the other arguments. */
    void search_following(AddressSet *worklist, const InstructionMap &bb, rose_addr_t bb_va, 
                          const MemoryMap *map, const InstructionMap &tried);

    /** Adds values of immediate operands to the list of addresses that should be disassembled.  Such operands are often used
     *  in a closely following instruction as a jump target. E.g., "move 0x400600, reg1; ...; jump reg1". This search method is
     *  invoked automatically if the SEARCH_IMMEDIATE bit is set (see set_search()).  The @p tried argument lists all the
     *  addresses where we already tried to disassemble (including addresses where disassembly failed) and this method refuses
     *  to add the following address to the worklist if we've already tried that address.
     *
     *  Thread safety:  Multiple threads can call this method using the same Disassembler object as long as they pass different
     *  worklist address sets and no other thread is modifying the other arguments. */
    void search_immediate(AddressSet *worklist, const InstructionMap &bb,  const MemoryMap *map, const InstructionMap &tried);

    /** Adds all word-aligned values to work list, provided they specify a virtual address in the @p map.  The @p wordsize must
     *  be a power of two. This search method is invoked automatically if the SEARCH_WORDS bit is set (see set_search()). The
     *  @p tried argument lists all the addresses where we already tried to disassemble (including addresses where disassembly
     *  failed) and this method refuses to add the following address to the worklist if we've already tried that address.
     *
     *  Thread safety: Multiple threads can call this method using the same Disassembler object as long as they pass different
     *  worklist address sets and no other thread is modifying the other arguments. */
    void search_words(AddressSet *worklist, const MemoryMap *map, const InstructionMap &tried);

    /** Finds the lowest virtual address, greater than or equal to @p start_va, which does not correspond to a previous
     *  disassembly attempt as evidenced by its presence in the supplied instructions or bad map.  If @p avoid_overlaps is set
     *  then do not return an address if an already disassembled instruction's raw bytes include that address.  Only virtual
     *  addresses contained in the MemoryMap will be considered.  The @p tried argument lists all the addresses where we
     *  already tried to disassemble (including addresses where disassembly failed) and this method refuses to add the
     *  following address to the worklist if we've already tried that address. The address is returned by adding it to the
     *  worklist; nothing is added if no qualifying address can be found. This method is invoked automatically if the
     *  SEARCH_ALLBYTES or SEARCH_UNUSED bits are set (see set_search()).
     *
     *  Thread safety: Multiple threads can call this method using the same Disassembler object as long as they pass different
     *  worklist address sets and no other thread is modifying the other arguments. */
    void search_next_address(AddressSet *worklist, rose_addr_t start_va, const MemoryMap *map, const InstructionMap &insns,
                             const InstructionMap &tried, bool avoid_overlaps);

    /** Adds addresses that correspond to function symbols.  This method is invoked automatically if the SEARCH_FUNCSYMS bits
     *  are set (see set_search()). It applies only to disassembly at the file header (SgAsmGenericHeader) level or above.
     *
     *  Thread safety: Multiple threads can call this method using the same Disassembler object as long as they pass different
     *  worklist addresses and no other thread is modifying the memory map or changing the AST under the specified file header
     *  node. */
    void search_function_symbols(AddressSet *worklist, const MemoryMap*, SgAsmGenericHeader*);


    /***************************************************************************************************************************
     *                                          Miscellaneous methods
     ***************************************************************************************************************************/
public:
    /** Updates progress information. This should be called each time the subclass' disassembleOne() is about to return a new
     *  instruction.
     *
     *  Thread safety: This method is thread safe. The optional supplied instruction is only used to obtain a virtual address. */
    void update_progress(SgAsmInstruction*);

    /** Makes an unknown instruction from an exception.
     *
     *  Thread safety: The safety of this method depends on its implementation in the subclass. */
    virtual SgAsmInstruction *make_unknown_instruction(const Exception&) = 0;

    /** Marks parts of the file that correspond to instructions as having been referenced.
     *
     *  Thread safety: This method is not thread safe. */
    void mark_referenced_instructions(SgAsmInterpretation*, const MemoryMap*, const InstructionMap&);

    /** Calculates the successor addresses of a basic block and adds them to a successors set. The successors is always
     *  non-null when called. If the function is able to determine the complete set of successors then it should set @p
     *  complete to true before returning.
     *
     *  Thread safety: Thread safe provided no other thread is modifying the specified instruction map. */
    AddressSet get_block_successors(const InstructionMap&, bool *complete);

private:
    /** Initialize class (e.g., register built-in disassemblers). This class method is thread safe, using class_mutex. */
    static void initclass();
    static void initclass_helper();

    /** Called only during construction. Thread safe. */
    void ctor();

    /** Finds the highest-address instruction that contains the byte at the specified virtual address. Returns null if no such
     *  instruction exists.
     *
     *  Thread safety: This class method is thread safe provided no other thread is modifying the instruction map nor the
     *  instructions to which the map points, particularly the instructions' virtual address and raw bytes. */
    static SgAsmInstruction *find_instruction_containing(const InstructionMap &insns, rose_addr_t va);



};

} // namespace
} // namespace

#endif
