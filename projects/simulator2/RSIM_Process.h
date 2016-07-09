#ifndef ROSE_RSIM_Process_H
#define ROSE_RSIM_Process_H

#include "RSIM_Callbacks.h"
#include <Sawyer/BiMap.h>

class RSIM_Thread;
class RSIM_Simulator;
class RSIM_FutexTable;

/** Represents a single simulated process. The process object holds resources that are shared among its threads. Some of the
 *  properties of a simulated process (such as PID) are shared with the real process (the process running the simulator).
 *
 *  Thread safety: Since an RSIM_Process may contain multiple RSIM_Thread objects and each RSIM_Thread is matched by a real
 *  thread, many of the RSIM_Process methods must be thread safe. */
class RSIM_Process {
public:
    /** Creates an empty process containing no threads. */
    explicit RSIM_Process(RSIM_Simulator *simulator)
        : simulator(simulator), tracingFile_(NULL), tracingFlags_(0),
          brkVa_(0), mmapNextVa_(0), mmapRecycle_(false), mmapGrowsDown_(false), disassembler_(NULL), futexes(NULL),
          interpretation_(NULL), entryPointOriginalVa_(0), entryPointStartVa_(0),
          terminated(false), termination_status(0), mainHeader_(NULL), project_(NULL), wordSize_(0), core_flags(0),
          btrace_file(NULL), core_styles(CORE_ELF), core_base_name("x-core.rose") {
        ctor();
    }

    ~RSIM_Process();

    RSIM_Simulator *get_simulator() const {
        return simulator;
    }

private:
    RSIM_Simulator *simulator;
    void ctor();


    /**************************************************************************************************************************
     *                                  Thread synchronization
     **************************************************************************************************************************/
private:
    mutable SAWYER_THREAD_TRAITS::RecursiveMutex  instance_rwlock; /**< One read-write lock per object.  See rwlock(). */

public:

    /** Returns the read-write lock for this object.
     *
     *  Although most RSIM_Process methods are already thread safe, it is sometimes necessary to protect access to data members
     *  This method returns a per-object lock.  The returned lock is the same lock as the inherently thread-safe methods of
     *  this class already use.
     *
     *  These locks should be held for as little time as possible, and certainly not over a system call that might block.
     *
     *  @{ */
    SAWYER_THREAD_TRAITS::RecursiveMutex &rwlock() {
        return instance_rwlock;
    }
    SAWYER_THREAD_TRAITS::RecursiveMutex &rwlock() const {
        return instance_rwlock;
    }
    /** @} */



    /**************************************************************************************************************************
     *                                  Tracing and debugging
     **************************************************************************************************************************/
private:
    std::string tracingFileName_;       /**< Pattern for trace file names. May include %d for thread ID. */
    FILE *tracingFile_;                 /**< Stream to which debugging output is sent (or NULL to suppress it) */
    unsigned tracingFlags_;             /**< Bit vector of what to trace. See TraceFlags. */
    struct timeval time_created;        /**< Time at which process object was created. */

public:

    /** Property: name of tracing file.
     *
     *  The name of the file to create for tracing output. All occurrences of the substring "${pid}" are replaced with the
     *  actual process ID.
     *
     * @{ */
    const std::string& tracingName() const { return tracingFileName_; }
    void tracingName(const std::string &s) { tracingFileName_ = s; }
    /** @} */

    /** Initialize tracing by (re)opening the trace file with the name pattern that was specified with set_trace_name().  The
     *  pattern should be a printf-style format with an optional integer specifier for the thread ID. */
    void open_tracing_file();

    /** Property: file for tracing, or NULL if tracing is disabled.
     *
     *  All trace facilities use the same file.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads concurrently.
     *
     * @{ */
    FILE *tracingFile() const { return tracingFile_; }
    void tracingFile(FILE *f) { tracingFile_ = f; }
    /** @} */

    /** Sets tracing file and facilities. */
    void set_tracing(FILE*, unsigned flags);

    /** Property: bits enabling various tracing.
     *
     *  A bit mask describing what should be traced by new threads created for this process. The return value is the
     *  bitwise OR of the facilityBitMask() for each enabled facility. */
    unsigned tracingFlags() const { return tracingFlags_; }

    /** Returns the time at which this process was created. */
    const struct timeval &get_ctime() const {
        return time_created;
    }


    
    /**************************************************************************************************************************
     *                                  Callbacks
     **************************************************************************************************************************/
private:
    RSIM_Callbacks callbacks;           /**< Callbacks for a process, and to initialize callbacks of new threads. */

public:

    /** Obtain the set of callbacks for this object.  Many of the process callbacks are used to initialize thread callbacks
     *  when a new thread is created.
     *
     *  Thread safety:  This method is thread safe.  Furthermore, most methods of the returned object are also thread safe.
     *
     *  @{ */
    RSIM_Callbacks &get_callbacks() {
        return callbacks;
    }
    const RSIM_Callbacks &get_callbacks() const {
        return callbacks;
    }
    /** @} */

    /** Set all callbacks for this process.  Note that callbacks can be added or removed individually by invoking methods on
     *  the callback object return by get_callbacks().
     *
     *  Thread safety:  This method is thread safe. */
    void set_callbacks(const RSIM_Callbacks &cb);

    /** Install a callback object.
     *
     *  This is just a convenient way of installing a callback object.  It appends it to the BEFORE slot (by default) of the
     *  appropriate queue.  If @p everwhere is true (not the default) then it also appends the callback to the appropriate
     *  callback list of all existing threads.  Regardless of whether a callback is applied to existing  threads, whenever a new
     *  thread is created it gets a clone of all its process callbacks.
     *
     *  @{ */  // ******* Similar functions in RSIM_Simulator and RSIM_Thread ******
    void install_callback(RSIM_Callbacks::InsnCallback *cb,
                          RSIM_Callbacks::When when=RSIM_Callbacks::BEFORE, bool everywhere=false);
    void install_callback(RSIM_Callbacks::MemoryCallback *cb,
                          RSIM_Callbacks::When when=RSIM_Callbacks::BEFORE, bool everywhere=false);
    void install_callback(RSIM_Callbacks::SyscallCallback *cb,
                          RSIM_Callbacks::When when=RSIM_Callbacks::BEFORE, bool everywhere=false);
    void install_callback(RSIM_Callbacks::SignalCallback *cb,
                          RSIM_Callbacks::When when=RSIM_Callbacks::BEFORE, bool everywhere=false);
    void install_callback(RSIM_Callbacks::ThreadCallback *cb,
                          RSIM_Callbacks::When when=RSIM_Callbacks::BEFORE, bool everywhere=false);
    void install_callback(RSIM_Callbacks::ProcessCallback *cb,
                          RSIM_Callbacks::When when=RSIM_Callbacks::BEFORE, bool everywhere=false);
    /** @} */

    /** Remove a callback object.
     *
     *  This is just a convenient way of removing callback objects.  It removes up to one instance of the callback from the
     *  process and, if @p everwhere is true (not the default) it recursively calls the removal methods for all threads of the
     *  process.  The comparison to find a callback object is by callback address.  If the callback has a @p clone() method
     *  that allocates a new callback object, then the callback specified as an argument probably won't be found in any of the
     *  threads.
     *
     * @{ */
    void remove_callback(RSIM_Callbacks::InsnCallback *cb,
                         RSIM_Callbacks::When when=RSIM_Callbacks::BEFORE, bool everywhere=false);
    void remove_callback(RSIM_Callbacks::MemoryCallback *cb,
                         RSIM_Callbacks::When when=RSIM_Callbacks::BEFORE, bool everywhere=false);
    void remove_callback(RSIM_Callbacks::SyscallCallback *cb,
                         RSIM_Callbacks::When when=RSIM_Callbacks::BEFORE, bool everywhere=false);
    void remove_callback(RSIM_Callbacks::SignalCallback *cb,
                         RSIM_Callbacks::When when=RSIM_Callbacks::BEFORE, bool everywhere=false);
    void remove_callback(RSIM_Callbacks::ThreadCallback *cb,
                         RSIM_Callbacks::When when=RSIM_Callbacks::BEFORE, bool everywhere=false);
    void remove_callback(RSIM_Callbacks::ProcessCallback *cb,
                         RSIM_Callbacks::When when=RSIM_Callbacks::BEFORE, bool everywhere=false);
    /** @} */

    /**************************************************************************************************************************
     *                                  Process memory
     **************************************************************************************************************************/
private:
    typedef std::vector<std::pair<MemoryMap, std::string > > MapStack;
    MapStack map_stack;                                 // Memory map transaction stack.
    rose_addr_t brkVa_;                                 // Current value for brk() syscall; initialized by load()
    rose_addr_t mmapNextVa_;                            // Minimum address to use when looking for mmap free space
    bool mmapRecycle_;                                  // If false, then never reuse mmap addresses
    bool mmapGrowsDown_;                                // If true then search down from a maximum, otherwise up from min

public:

    /** Returns the memory map for the simulated process.  MemoryMap is not thread safe [as of 2011-03-31], so all access to
     *  the map should be protected by the process-wide read-write lock returned by the rwlock() method.
     * @{ */
    MemoryMap& get_memory() {
        assert(!map_stack.empty());
        return map_stack.back().first;
    }
    const MemoryMap& get_memory() const {
        assert(!map_stack.empty());
        return map_stack.back().first;
    }
    /** @} */

    /** Add a memory mapping to a specimen.  The new mapping starts at specimen address @p va (zero causes this method to
     *  choose an appropriate address) for @p size bytes.  The @p rose_perms are the MemoryMap::Protection bits, @p flags are
     *  the same as for mmap() and are defined in <sys/mman.h>, and @p offset and @p fd are as for mmap().  Returns a negative
     *  error number on failure.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    rose_addr_t mem_map(rose_addr_t va, size_t size, unsigned rose_perms, unsigned flags, size_t offset, int fd);

    /** Set the process brk value and adjust the specimen's memory map accordingly.  The return value is either a negative
     *  error number (such as -ENOMEM) or the new brk value.  The @p stream will be used to show the memory map
     *  after it is adjusted.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    rose_addr_t mem_setbrk(rose_addr_t newbrk, Sawyer::Message::Stream &stream);

    /** Unmap some specimen memory.  The starting virtual address, @p va, and number of bytes, @p sz, need not be page aligned,
     *  but if they are then the real munmap() is also called, substituting the real address for @p va.  The return value is a
     *  negative error number on failure, or zero on success.  The @p stream will be used to show the memory map after it is
     *  adjusted.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    int mem_unmap(rose_addr_t va, size_t sz, Sawyer::Message::Stream &stream);

    /** Change protection bits on part of the specimen virtual memory.  The @p rose_perms specify the desired permission bits
     *  to set in the specimen's memory map (in terms of MemoryMap::Protection bits) while the @p real_perms are the desired
     *  permissions of the real underlying memory in the simulator itself (using constants from <sys/mman.h>.
     * 
     *  The return value is a negative error number on failure, or zero on success.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    int mem_protect(rose_addr_t va, size_t sz, unsigned rose_perms, unsigned real_perms);
    
    /** Dump a memory map description to the specified message object.  The @p intro is an optional message to be printed
     *  before the map (it should include a newline character), and @p prefix is an optional string to print before each line
     *  of the map (defaulting to four spaces).
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    void mem_showmap(Sawyer::Message::Stream &stream, const char *intro=NULL, const char *prefix=NULL);

    /** Returns true if the specified specimen virtual address is mapped; false otherwise.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    bool mem_is_mapped(rose_addr_t va) const;

    /** Returns the memory address in ROSE where the specified specimen address is located.
     *
     *  Thread safety: This method is thread safe; it can be invoked on a single object by multiple threads
     *  concurrently. However, the address that is returned might be unmapped before the caller can do anything with it. */
    void *my_addr(rose_addr_t va, size_t size);

    /** Does the opposite, more or less, of my_addr(). Return a specimen virtual address that maps to the specified address in
     *  the simulator.  There may be more than one, in which case we return the lowest.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    rose_addr_t guest_va(void *addr, size_t nbytes);

    /** Copies data into the specimen address space.  Copies up to @p size bytes from @p buf into specimen memory beginning at
     *  virtual address @p va.  If the requested number of bytes cannot be copied because (part of) the destination address
     *  space is not mapped or because (part of) the destination address space does not have write permission, the this method
     *  will write as much as possible up to the first invalid address.  The return value is the number of bytes copied.
     *
     *  The specimen memory map must have the required permission bits set in order for the write to succeed.  By default, the
     *  memory must be writable.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    size_t mem_write(const void *buf, rose_addr_t va, size_t size, unsigned req_perms=MemoryMap::WRITABLE);

    /** Copies data from specimen address space.  Copies up to @p size bytes from the specimen memory beginning at virtual
     *  address @p va into the beginning of @p buf.  If the requested number of bytes cannot be copied because (part of) the
     *  destination address space is not mapped or because (part of) the destination address space does not have read
     *  permission, then this method will read as much as possible up to the first invalid address.  The return value is the
     *  number of bytes copied.
     *
     *  The specimen memory map must have the required permission bits set in order for the read to succeed.  By default, the
     *  memory must be readable.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads
     *  concurrently. */
    size_t mem_read(void *buf, rose_addr_t va, size_t size, unsigned req_perms=MemoryMap::READABLE);

    /** Reads a NUL-terminated string from specimen memory. The NUL is not included in the string.  If a limit is specified
     *  then the returned string will contain at most this many characters (a value of zero implies no limit).  If the string
     *  cannot be read, then "error" (if non-null) will point to a true value and the returned string will include the
     *  characters up to the error.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    std::string read_string(rose_addr_t va, size_t limit=0, bool *error=NULL);

    /** Reads a null-terminated vector of pointers to NUL-terminated strings from specimen memory.  If some sort of
     *  segmentation fault or bus error would occur, then set *error to true and return all that we read, otherwise set it to
     *  false.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    std::vector<std::string> read_string_vector(rose_addr_t va, size_t ptrSize, bool *error=NULL);

    /** Memory transactions.
     *
     *  Memory transactions are intended to be used to run an analysis without affecting the main memory of the simulation,
     *  allowing the analysis to modify memory as it runs and then restoring the original values when the analysis has
     *  completed.  As such, the usual case of starting a transaction and then rolling back has been optimized: the
     *  mem_transaction_start() and mem_transaction_rollback() methods.  The mem_transaction_commit() is a much slower
     *  operation if large memory segments have been the targets of write operations (regardless of whether the write operation
     *  caused the memory values to change).
     *
     *  The mem_transaction_start() pushes a copy of the current memory map onto the transaction stack.  The new memory map's
     *  segments have their copy-on-write bits set.  A transaction can be given an optional name.  The name does not need to be
     *  unique. Returns the number of transaction on the stack (including the new one).
     *
     *  The mem_transaction_rollback() pops memory maps from the transaction stack until it finds one with the specified name
     *  (or no-name).  The matching memory map is also popped, leaving the stack in the same situation as it was before the
     *  matching mem_transaction_start() call.  Returns the number of items popped from the stack.  If the resulting stack is
     *  empty, then a new transaction is started and has the same name as the oldest transaction (old bottom of stack) and an
     *  empty map.  If the specified name does not exist on the transaction stack, then the stack is not modified and zero is
     *  returned.
     *
     *  The mem_transaction_commit() operates like mem_transaction_rollback() except each time a map is popped from the stack
     *  it's data is copied into the map underneath.  Any address that exists in the popped map that does not exist in the
     *  underlying map is created in the underlying map.  Any value that is different in the popped map than the underlying map
     *  is written to the underlying map.
     *
     *  There are a few things that can go wrong:
     *
     *  <ol>
     *    <li>When data is written into a transaction, the underlying memory buffer is copied.  The copying will break any
     *    association between addresses and mapped files for that segment for the duration of the transaction.  If another
     *    transaction is started before the first one is finished, then the file-address association will also be missing in
     *    that second transaction.  Mapped files are used for things like interprocess communication, so don't expect that to
     *    work within a memory transaction.  Some specimens use memory-mapped files to access and/or modify the content of that
     *    file without having to make system calls.<li>
     *    <li>Changes to the mapping itself (adding or removing transactions, changing transaction permissions, etc) are not
     *    committed accurately during mem_transaction_commit().</li>
     *    <li>Changes to memory buffer properties (writability, file mapping, etc) are not committed accurately during
     *    mem_transaction_commit().</li>
     *  </ol>
     *
     * @{ */
    size_t mem_transaction_start(const std::string &name="");
    void mem_transaction_commit(const std::string &name="");
    size_t mem_transaction_rollback(const std::string &name="");
    /** @} */

    /** Name of current memory transaction.
     *
     *  Each memory transaction can be given an optional name.  Names need not be unique.  Any function that looks up a
     *  transaction by name will do so by scanning the transaction stack from most-recent to oldest transaction and return the
     *  first one that matches. */
    std::string mem_transaction_name() const;

    /** Number of outstanding memory transactions.
     *
     *  Returns the number of memory transactions on the transaction stack.  There is normally always at least one transaction,
     *  but the stack might be empty before the specimen is loaded. */
    size_t mem_ntransactions() const;

    /** Property: brk address.
     *
     * @sa mem_setbrk, which also adjusts the memory map.
     *
     * @{ */
    rose_addr_t brkVa() const { return brkVa_; }
    void brkVa(rose_addr_t va) { brkVa_ = va; }
    /** @} */

    /** Property: mmap starting address.
     *
     *  This is the address where we search for a free area to satisfy a mmap request.
     *
     * @{ */
    rose_addr_t mmapNextVa() const { return mmapNextVa_; }
    void mmapNextVa(rose_addr_t va) { mmapNextVa_ = va; }
    /** @} */

    /** Property: recycle mmap addresses.
     *
     *  If false, then each call to mmap will return an address beyond (less or greater depending on architecture) the last
     *  address chosen by the simulated OS.
     *
     * @{ */
    bool mmapRecycle() const { return mmapRecycle_; }
    void mmapRecycle(bool b) { mmapRecycle_ = b; }
    /** @} */

    /** Property: whether mmap addresses grow down.
     *
     *  If set, then memory regions to satisfy mmap requests are searched downward from a maximum address, otherwise upward
     *  from a minimum address.
     *
     * @{ */
    bool mmapGrowsDown() const { return mmapGrowsDown_; }
    void mmapGrowsDown(bool b) { mmapGrowsDown_ = b; }
    /** @} */

    /**************************************************************************************************************************
     *                                  Segment registers
     **************************************************************************************************************************/
public:
    /** Set a global descriptor table entry.  This should only be called via RSIM_Thread::set_gdt(). In Linux, three of the GDT
     *  entries (GDT_ENTRY_TLS_MIN through GDT_ENTRY_TLS_MAX) are updated from the thread_struct every time a thread is
     *  scheduled.  The simulator works a bit differently since all threads are effectively always running. The simulator keeps
     *  a single GDT in the RSIM_Process but threads always access it through an RSIM_Thread object. This allows each
     *  RSIM_Thread to override the TLS-related entries.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    void set_gdt(const SegmentDescriptor&);

    /** Returns a reference to the segment descriptor in the GDT. */
    SegmentDescriptor& gdt_entry(int idx);

    static const int GDT_ENTRIES = 8192;                     /**< Number of GDT entries. */
    static const int GDT_ENTRY_TLS_MIN = 6;                  /**< First TLS entry (this would be 12 on x86_64) */
    static const int GDT_ENTRY_TLS_ENTRIES = 3;              /**< Number of TLS entries */
    static const int GDT_ENTRY_TLS_MAX = GDT_ENTRY_TLS_MIN + GDT_ENTRY_TLS_ENTRIES - 1; /**< Last TLS entry */

private:
    /**< Global descriptor table. Entries GDT_ENTRY_TLS_MIN through GDT_ENTRY_TLS_MAX are unused. */
    SegmentDescriptor gdt[GDT_ENTRIES];


    
    /**************************************************************************************************************************
     *                                  Instructions and disassembly
     **************************************************************************************************************************/
private:
    rose::BinaryAnalysis::Disassembler *disassembler_;         /**< Disassembler to use for obtaining instructions */
    rose::BinaryAnalysis::Disassembler::InstructionMap icache;        /**< Cache of disassembled instructions */

public:
    /** Disassembles the instruction at the specified virtual address. For efficiency, instructions are cached by the
     *  process. Instructions are removed from the cache (but not deleted) when the memory at the instruction address changes.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    SgAsmInstruction *get_instruction(rose_addr_t va);

    /** Disassemble a process memory image.
     * 
     *  This method disassembles an entire process based on the current memory map (or the supplied submap), returning a
     *  pointer to a SgAsmBlock AST node and inserting all new instructions into the instruction cache used by
     *  get_instruction().
     *
     *  If the @p fast argument is set then the partitioner is not run and the disassembly simply disassembles all executable
     *  addresses that haven't been disassembled yet, adding them to the process' instruction cache.  The return value in this
     *  case is an SgAsmBlock that contains all the instructions.
     *
     *  If a memory map is supplied then that map is used in preference to the process' memory map.  The supplied map should be
     *  a subset of the process' memory map since new instructions will be added to the process' instruction cache.  One should
     *  be careful when passing a memory map containing non-readable specimen segments: those segments might map to simulator
     *  memory that is also non-readable and when the x86 disassembler reads a 15-byte chunk of memory to decode an instruction
     *  it will get a segfault if any part of that 15 bytes is non-readable.
     * 
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threaads concurrently.
     *  The callers are serialized and each caller will generate a new AST that does not share nodes with any AST returned by
     *  any previous call by this thread or any other. */
    SgAsmBlock *disassemble(bool fast=false, MemoryMap *map=NULL);

    /** Property: Disassembler.
     *
     *  The disassembler that is being used to obtain instructions. This disassembler is chosen automatically when the specimen
     *  is loaded.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads
     *  concurrently. However, the disassembler object which is returned can probably not be used concurrently by multiple
     *  threads. See documentation for Disassembler for thread safety details.
     *
     * @{ */
    rose::BinaryAnalysis::Disassembler *disassembler() const { return disassembler_; }
    void disassembler(rose::BinaryAnalysis::Disassembler *d) { disassembler_ = d; }
    /** @} */

    /** Returns the total number of instructions processed across all threads.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    size_t get_ninsns() const;

    /***************************************************************************************************************************
     *                                  File descriptors
     ***************************************************************************************************************************/
private:
    Sawyer::Container::BiMap<int, int> fileDescriptors_; // File descriptor mapping from guest to host (and vice versa)

public:
    /** Obtain host file descriptor from guest file descriptor.
     *
     *  Returns the host file descriptor associated with the specified guest file descriptor. Returns -1 if there is no such
     *  guest or host file descriptor. */
    int hostFileDescriptor(int guestFd);

    /** Obtain guest file descriptor from host file descriptor.
     *
     *  Returns the guest file descriptor associated with the specified host file descriptor. Returns -1 if there is no such
     *  guest or host file descriptor. */
    int guestFileDescriptor(int hostFd);

    /** Allocate a new guest file descriptor.
     *
     *  Allocates a new guesst file descriptor and associates it with the specified host descriptor, or returns the already
     *  associated guest descriptor if the host file descriptor is already mapped. */
    int allocateGuestFileDescriptor(int hostFd);

    /** Allocate a new guest/host descriptor pair.
     *
     *  Adds the specified pair to the descriptor mapping table, erasing any previous associations for the guest or host
     *  descriptor. */
    void allocateFileDescriptors(int guestFd, int hostFd);

    /** Erase guest file descriptor.
     *
     *  Erases the guest file descriptor from the mapping if it exists. */
    void eraseGuestFileDescriptor(int guestFd);

    /**************************************************************************************************************************
     *                                  Signal handling
     **************************************************************************************************************************/
private:
    /** Simulated actions for signal handling. Array element N is signal N+1. All threads of a process share signal
     *  dispositions (see CLONE_SIGHAND bit). */
    SigAction signal_action[_NSIG];

    /** The asynchronous signal reception queue.  Signals are pushed onto the tail of this queue asynchronously by
     *  signal_enqueue(), and removed synchronously from the head by signal_dequeue(). */
    struct AsyncSignalQueue {
        AsyncSignalQueue()
            : head(0), tail(0) {
            memset(info, 0, sizeof info); /* only for debugging */
        }
        static const size_t size = 128; /**< Up to 127 signals can be on the queue, plus one guard entry */
        RSIM_SignalHandling::SigInfo info[size];
        size_t head;                    /**< Index of oldest signal */
        size_t tail;                    /**< One beyond index of youngest signal (incremented asynchronously) */
    } sq;

public:
    /** Signals that have arrived for the process-as-a-whole which cannot be delivered to any thread because all threads have
     *  these signals masked. Signals are placed onto this queue by sys_kill() (when necessary) and removed from it by each
     *  thread using RSIM_Thread::signal_dequeue(). */
    RSIM_SignalHandling sighand;

    /** Simulates a sigaction() system call.  Returns zero on success; negative errno on failure.
     *
     *  Thread safety: This method is thread safe. */
    int sys_sigaction(int signo, const SigAction *new_action, SigAction *old_action);

    /** Simulates a kill() system call. Returns zero on success; negative errno on failure.
     *
     *  Thread safety: This method is thread safe. */
    int sys_kill(pid_t pid, const RSIM_SignalHandling::SigInfo&);

    /** Signal queue used for asynchronous reception of signals from other processes. Signal numbers are pushed onto the tail
     *  of this queue by signal_enqueue(), called from the RSIM_Simulator::signal_receiver() signal handler.  Therefore, this
     *  function must be async signal safe. */
    void signal_enqueue(const RSIM_SignalHandling::SigInfo&);

    /** Removes one signal from the queue.  Returns the oldest signal on the queue, or zero if the queue is empty.
     *
     *  Thread safety: This method is thread safe. */
    int signal_dequeue(RSIM_SignalHandling::SigInfo *info/*out*/);

    /** Assigns process-wide signals to threads.
     *
     *  Thread safety: This method is thread safe. */
    void signal_dispatch();



    /**************************************************************************************************************************
     *                                  Fast User-space Mutexes (Futexes)
     **************************************************************************************************************************/
private:
    RSIM_FutexTable *futexes;

public:
    /** Return futex table.
     *
     *  Specimen futexes have two parts: user-space atomic instructions that operate on a memory word, and the futex system
     *  call to handle the contended cases.  The futex system call has two operations: wait and wake.  The wait operation
     *  causes the calling thread to enter itself onto a queue based on the real address of the futex and block until
     *  signaled.  The wake operation causes matching threads on the queue to resume running and remove themselves from the
     *  queue.
     *
     *  The queues are implemented as an RSIM_FutexTable object, which creates/opens shared memory for communication across
     *  processes.  The futex shared memory is created/opened the same way as the RSIM_Simulator main semaphore.  In fact, that
     *  semaphore is used to protect access to the futex table.
     *
     *  Thread safety:  This method is thread safe.  The futex table is created when the RSIM_Process object is created.  The
     *  actual futex queues are shared among all simulator processes that open the same shared memory object based on the name
     *  of the RSIM_Simulator semaphore. */
    RSIM_FutexTable *get_futexes() {
        return futexes;
    }



    /***************************************************************************************************************************
     *                                  Thread creation/join simulation
     ***************************************************************************************************************************/
private:
    /**< Contains various things that are needed while we clone a new thread to handle a simulated clone call. */
    struct Clone {
        SAWYER_THREAD_TRAITS::RecursiveMutex mutex; /**< Protects entire structure. */
        SAWYER_THREAD_TRAITS::ConditionVariable cond; /**< For coordinating between creating thread and created thread. */
        boost::thread   hostThread;          /**< Real thread hosting the simulated thread; moved to RSIM_Thread obj. */
        RSIM_Process    *process;            /**< Process creating the new thread. */
        unsigned        flags;               /**< Various CLONE_* flags passed to the clone system call. */
        pid_t           newtid;              /**< Created thread's TID filled in by clone_thread_helper(); negative on error */
        int             seq;                 /**< Sequence number for new thread, used for debugging. */
        rose_addr_t     parent_tid_va;       /**< Optional address at which to write created thread's TID; clone() argument */
        rose_addr_t     child_tls_va;        /**< Address of TLS user_desc_32 to load into GDT; clone() argument */
        PtRegs          regs;                /**< Initial registers for child thread. */

        Clone(RSIM_Process *process, unsigned flags, rose_addr_t parent_tid_va, rose_addr_t child_tls_va, const PtRegs &regs)
            : process(process), flags(flags), newtid(-1), seq(-1),
              parent_tid_va(parent_tid_va), child_tls_va(child_tls_va), regs(regs) {}
    };
    static Clone clone_info;
    
    /** Helper to create a new simulated thread and corresponding real thread. Do not call this directly; call clone_thread()
     *  instead.  Thread creation is implemented in two parts: clone_thread() is the main entry point and is called by the
     *  thread that wishes to create a new thread, and clone_thread_helper() is the part run by the new thread.
     *
     *  We need to do a little dancing to return the ID of the new thread to the creating thread.  This is where the
     *  clone_mutex, clone_cond, and clone_newtid class data members are used.  The creating thread blocks on the clone_cond
     *  condition variable while the new thread fills in clone_newtid with its own ID and then signals the condition
     *  variable. The clone_mutex is only used to protect the clone_newtid. */
    static void clone_thread_helper(void *process);

    /** Create a new thread.
     *
     *  This should be called only by the real thread which will be simulating the specimen's thread, i.e., the
     *  hostThread. Each real thread should simulate a single specimen thread. This is normally invoked by
     *  clone_thread_helper().
     *
     *  Thread safety: This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    RSIM_Thread *create_thread(boost::thread &hostThread);

public:
    /** Sets the main thread.  This discards all known threads from the "threads" map and reinitializes the map with the single
     *  specified thread.
     *
     *  Thread safety:  Not thread safe.  This should only be called during process initialization. */
    void set_main_thread(RSIM_Thread *t);

    /** Returns the main (only) thread.  Fails if there is more than one thread. */
    RSIM_Thread *get_main_thread() const;

    /** Creates a new simulated thread and corresponding real thread.  Returns the ID of the new thread, or a negative errno.
     *  The @p parent_tid_va and @p child_tid_va are optional addresses at which to write the new thread's TID if the @p flags
     *  contain the CLONE_PARENT_TID and/or CLONE_CHILD_TID bits.  We gaurantee that the TID is written to both before this
     *  method returns.  The @p child_tls_va also points to a segment descriptor if the CLONE_SETTLS bit is
     *  set.  The @p regs are the values with which to initialize the new threads registers.  The new thread does not start
     *  executing.
     *
     *  Thread safety: This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    pid_t clone_thread(unsigned flags, rose_addr_t parent_tid_va, rose_addr_t child_tls_va, const PtRegs &regs,
                       bool startExecuting);

    /** Returns the thread having the specified thread ID, or null if there is no thread with the specified ID.  Thread objects
     *  are never deleted while a simulator is running, but they are unlinked from their parent RSIM_Process when the simulated
     *  thread dies (and the simulating thread exits).
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    RSIM_Thread *get_thread(pid_t tid) const;

    /** Returns a vector of current threads.  These are all the threads that currently belong to the process.
     *
     *  Thread safety:  This method is thread safe; it can be invoked on a single object by multiple threads
     *  concurrently. However, by time the vector is returned to the caller, the list of active threads may have changed.
     *  Fortunately, RSIM doesn't delete RSIM_Thread objects when a thread exits, so at least all the returned pointers will
     *  still be valid. */
    std::vector<RSIM_Thread*> get_all_threads() const;

    /** Remove a thread from this process.  This is normally called by the specified thread when that thread exits.  Calling
     *  this method twice for the same thread will result in a failed assertion.
     *
     *  Thread safety: This method is thread safe; it can be invoked on a single object by multiple threads concurrently. */
    void remove_thread(RSIM_Thread*);



    /***************************************************************************************************************************
     *                                  Process loading, linking, exit, etc.
     ***************************************************************************************************************************/
private:
    SgAsmInterpretation *interpretation_;        /**< Chosen by the load() method. */
    std::string interpname;                     /**< Name of interpreter from ".interp" section or "--interp=" switch */
    rose_addr_t entryPointOriginalVa_;          /**< Original executable entry point (a specimen virtual address). */
    rose_addr_t entryPointStartVa_;             /**< Entry point where simulation starts (e.g., the dynamic linker). */
    bool terminated;                            /**< True when the process has finished running. */
    int termination_status;                     /**< As would be returned by the parent's waitpid() call. */
    std::vector<SgAsmGenericHeader*> headers_;   /**< Headers of files loaded into the process (only those that we parse). */
    SgAsmGenericHeader *mainHeader_;                     // header for the main specimen
    SgProject *project_;                         /**< AST project node for the main specimen (not interpreter or libraries). */
    size_t wordSize_;                                   // natural word size in bits (32 or 64)

public:
    /** Thrown by exit system calls. */
    struct Exit {
        Exit(int status, bool exit_process): status(status), exit_process(exit_process) {}
        int status;                             /**< Same value as returned by waitpid(). */
        bool exit_process;                      /**< If true, then exit the entire process. */
    };

    /** Returns the interpreter name for dynamically linked ELF executables.  This name comes from the ".interp" section of the
     *  ELF file and is usually the name of the dynamic linker, ld-linux.so.   The interpreter name is set automatically when
     *  the main executable is parsed, or it can be set manually with set_interpname(). */
    std::string get_interpname() const {
        return interpname;
    }

    /** Overrides the interpreter name that would have been obtained from the ELF ".interp" section.  This is useful when one
     *  has written a custom linker.  Regardless of whether the linker is explicitly specified or comes from the ELF file, it
     *  will be loaded into the specimen address space and executed. */
    void set_interpname(const std::string &s) {
        interpname = s;
    }

    /** Loads a new executable image into an existing process.
     *
     *  The executable name comes from the parent simulator. If the name contains no slashes, then the corresponding file is
     *  found by trying each of the directories listed in the PATH environment variable, otherwise the specified name is used
     *  directly.
     *
     *  Once the executable file is located, the ROSE frontend() is invoked to parse the container (ELF, PE, etc) but
     *  instructions are not disassembled yet.  A particular interpretation (SgAsmInterpretation) is chosen if more than one is
     *  available, and this becomes the node returned by get_interpretation().  The file header associated with this
     *  interpretation becomes the return value of this method.
     *
     *  Then one of three things happens:
     *
     *  @li If @p pid is not -1 then it must be the process ID for some existing process which will be used to initialize the
     *      simulator's memory and registers (operating system state is not initialized from the existing process because there
     *      is no way to do that).  The simulated process will have only one thread.
     *
     *  @li If native loading is enabled, then a native process is created under a debugger but not allowed to execute. The
     *      simulator's memory and registers are initialized from the native process, and the native process is then killed.
     *
     *  @li Otherwise, the simulator's memory map and registers are initialized by emulating the exec actions of the Linux
     *      kernel. Namely, loading the specified executable into simulated memory, loading the ELF interpreter (dynamic
     *      linker) if necessary, and loading any user-supplied VDSO such as a copy of linux-gate.so or linux-vdso.so.
     *
     *  A disassembler is chosen based on the interpretation. The disassembler can be obtained by calling get_disassembler().
     *
     *  Operating system simulation data is initialized (brk, mmap, etc)
     *
     *  Returns 0 on success, negative error number on failure. */
    int load(int pid = -1);

    /** Returns the list of projects loaded for this process.  The list is initialized by the load() method. Headers are in
     *  order of their addresses.
     *
     * @{ */
    const std::vector<SgAsmGenericHeader*>& headers() const { return headers_; }
    std::vector<SgAsmGenericHeader*>& headers() { return headers_; }
    /** @} */

    /** File header for the main executable. */
    SgAsmGenericHeader *mainHeader() {
        return mainHeader_; 
    }

    /** Word size in bits. This returns null until after @ref load is called. */
    size_t wordSize() const {
        return wordSize_;
    }
    
    /** Returns the project node. This returns null until after load() is called. */
    SgProject *get_project() const {
        return project_;
    }
    
    /** Returns the interpretation that is being simulated.  The interpretation was chosen by the load() method. */
    SgAsmInterpretation *get_interpretation() const {
        return interpretation_;
    }

    /** Property: original entry point.
     *
     *  This is the entry point specified in the executable's file header and might not necessarily be the same address as
     *  where the simulation begins.
     *
     *  Thread safety:  This method is thread safe.
     *
     * @{ */
    rose_addr_t entryPointOriginalVa() const { return entryPointOriginalVa_; }
    void entryPointOriginalVa(rose_addr_t va) { entryPointOriginalVa_ = va; }
    /** @} */

    /** Property: address at which simulation starts.
     *
     *  This might not be the same as the program entry address returned by @ref entryPointOriginalVa -- when an ELF executable
     *  is dynamically linked, this is the entry point for the dynamic linker.
     *
     *  Thread safety:  This method is thread safe.
     *
     * @{ */
    rose_addr_t entryPointStartVa() const { return entryPointStartVa_; }
    void entryPointStartVa(rose_addr_t va) { entryPointStartVa_ = va; }
    /** @} */

    /** Exit entire process. Saves the exit status (like that returned from waitpid), then joins all threads except the main
     *  thread.
     *
     *  Thread safety: This method can be called by any thread or multiple threads. This function returns only when called by
     *  the main thread. */
    void sys_exit(int status);

    /** Returns true if simulated process has terminated. */
    bool has_terminated() {
        return terminated;
    }

    /** Returns the process exit status. If the process has not exited, then zero is returned. */
    int get_termination_status() {
        return termination_status;
    }

public:
    void btrace_close();

    /** Sets the core dump styles. */
    void set_core_styles(unsigned bitmask) {
        core_flags = bitmask;
    }

    /** Generate an ELF Core Dump on behalf of the specimen.  This is a real core dump that can be used with GDB and contains
     *  the same information as if the specimen had been running natively and dumped its own core. In other words, the core
     *  dump we generate here does not have references to the simulator even though it is being dumped by the simulator. */
    void dump_core(int signo, std::string base_name="");

    /** Start an instruction trace file. No-op if "binary_trace" is null. */
    void binary_trace_start();

    /** Add an instruction to the binary trace file. No-op if "binary_trace" is null. */
    void binary_trace_add(RSIM_Thread*, const SgAsmInstruction*);

private:
    std::map<pid_t, RSIM_Thread*> threads;      /**< All threads associated with this process. */

private:
    unsigned core_flags;                        /**< Bit vector describing how to produce core dumps. */

public: /* FIXME */
    FILE *btrace_file;                          /**< Stream for binary trace. See projects/traceAnalysis/trace.C for details */

    static const uint32_t brk_base=0x08000000;  /* Lowest possible brk() value */
    unsigned core_styles;                       /* What kind of core dump(s) to make for dump_core() */
    std::string core_base_name;                 /* Name to use for core files ("core") */
};

#endif /* ROSE_RSIM_Process_H */
