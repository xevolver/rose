include $(top_srcdir)/config/Makefile.for.ROSE.includes.and.libs

########################################################################################################
# This directory demonstrates the use of the INTEL PIN Dynamic Binary Instrumentation Tool within ROSE #
########################################################################################################

# if configured to use Intel Pin
if ROSE_USE_INTEL_PIN

# Reference location to where some of the example pin tools are located in the Intel Pin distribution.
INTEL_PIN_TOOLS_PATH = $(INTEL_PIN_PATH)/source/tools/SimpleExamples

# INTEL PIN paths are now included in ROSE_INCLUDES with --with-Intel_Pin.
AM_CPPFLAGS = $(ROSE_INCLUDES)

# These are ROSE analysis applications that mix static and dynamic analysis.
noinst_PROGRAMS = dynamicStaticBinaryAnalysis staticDynamicBinaryAnalysis staticBinaryAnalysis commandlineHandling staticCallGraphEmbeddedTraceBinaryAnalysis 

# These are example Pin tools (some ruse ROSE and demonstrat how to link pin tools with ROSE for mixed analysis).
noinst_OBJECTS = graphTrace.o imageload.o dynamicBinaryInfo.o memcopy-elf.o

LIBTOOL_RPATH_LDADD = $(ROSE_LIBS_WITH_PATH) $(RT_LIBS) $(INTEL_PIN_LIB_PATHS) -lpin -lxed -ldwarf -lelf -ldl

# DQ (3/14/2009): This factors the lib paths (now defined in config/Makefile.for.ROSE.includes.and.libs).
# INTEL_PIN_WITH_DWARF_LIBS_PATH = $(INTEL_PIN_LIB_PATHS) -lpin -lxed -ldl $(ROSE_DWARF_LIBS_WITH_PATH)

# This ROSE application mixes dynamic analysis with static analysis (dynamic analysis first and then static analysis)
# as a result is must link to both ROSE and Pin.
dynamicStaticBinaryAnalysis_SOURCES = dynamicStaticBinaryAnalysis.C
dynamicStaticBinaryAnalysis_LDADD   = $(LIBTOOL_RPATH_LDADD)

# This ROSE application mixes dynamic analysis with static analysis (dynamic analysis first and then static analysis)
# as a result is must link to both ROSE and Pin.
staticDynamicBinaryAnalysis_SOURCES = staticDynamicBinaryAnalysis.C
staticDynamicBinaryAnalysis_LDADD   = $(LIBTOOL_RPATH_LDADD)

# Since this does not use Intel Pin yet, it need not include more libs (simple ROSE link step).
# This ROSE application mixes static analysis with dynamic analysis (static analysis first and then dynamic analysis)
staticBinaryAnalysis_SOURCES = staticBinaryAnalysis.C
staticBinaryAnalysis_LDADD   = $(ROSE_LIBS_WITH_PATH) $(RT_LIBS)

# This code demonstrates the commandline handling required to mix Pin with ROSE.
commandlineHandling_SOURCES = commandlineHandling.C
commandlineHandling_LDADD   = $(LIBTOOL_RPATH_LDADD)

staticCallGraphEmbeddedTraceBinaryAnalysis_SOURCES = staticCallGraphEmbeddedTraceBinaryAnalysis.C
staticCallGraphEmbeddedTraceBinaryAnalysis_LDADD   = $(LIBTOOL_RPATH_LDADD)

# This is a general variable to link against (uses -lrose and works with static or dynamic linking):
# NON_LIBTOOL_RPATH = -Wl,-rpath $(top_builddir)/src/.libs -L$(top_builddir)/src/.libs -lrose $(INTEL_PIN_LIB_PATHS) -lpin -lxed -ldwarf -lelf -ldl $(ROSE_DWARF_LIBS_WITH_PATH)
# NON_LIBTOOL_RPATH = -Wl,-rpath $(top_builddir)/src/.libs -L$(top_builddir)/src/.libs -lrose $(INTEL_PIN_LIB_PATHS) -lpin -lxed -ldl $(ROSE_DWARF_LIBS_WITH_PATH)
NON_LIBTOOL_RPATH = -Wl,-rpath $(top_builddir)/src/.libs -L$(top_builddir)/src/.libs -lrose $(INTEL_PIN_WITH_DWARF_LIBS_PATH)


# This we were we will put test codes (not there yet):
TESTCODES_REQUIRED_TO_PASS = 
#  pintest2008_01.C

TESTCODE_CURRENTLY_FAILING = 
#  pintest2008_02.C 

# "make check" tests to be run:
ROSE_PIN_TESTS = testMemcopy testGraphTrace testImageload testDynamicBinaryInfo


# This is a test code (executable) to be used with the Intel Pin dynamic instrumentation
# (it can be linked staticly or dynamically).
input_testcode: $(srcdir)/input_testcode.C
#	g++ -static $(srcdir)/input_testcode.C -o input_testcode
	g++ $(srcdir)/input_testcode.C -o input_testcode


# **************************************************************************
# This tests the Intel Pin tool using one of its internal tools (should have been built if Intel Pin was built)
# This tests the installation of Pin and nothing about the use of Pin with ROSE.
testPinInstallation: input_testcode $(INTEL_PIN_TOOLS_PATH)/obj-intel64/icount.so
	$(INTEL_PIN_PATH)/pin -slow_asserts -t $(INTEL_PIN_TOOLS_PATH)/obj-intel64/icount.so -- ./input_testcode


# **************************************************************************
# This Pin tool generates a dot graph of the trace of execution of a binary (and uses support functions in ROSE).
# So it must be linked explicitly (not using libtool) and reference either the shared library or the static library.
# When the shared library (librose.so) is used, $(top_builddir)/src/.libs must be in the LD_LIBRARY_PATH.
graphTrace.so: graphTrace.o
	g++ -g -shared -Wl,-Bsymbolic -Wl,--version-script=$(INTEL_PIN_PATH)/source/include/pintool.ver -o graphTrace.so graphTrace.o $(NON_LIBTOOL_RPATH)

# Test example program generate a graph of the execution trace of a binary
testGraphTrace: graphTrace.so input_testcode
	$(INTEL_PIN_PATH)/pin -slow_asserts -t graphTrace.so -- ./input_testcode


# **************************************************************************
# This Pin tool generates log of the image loading and unloading (of shared libraries) of a binary
imageload.so: imageload.o
#	g++ -g -shared -Wl,-Bsymbolic -Wl,--version-script=$(INTEL_PIN_PATH)/source/include/pintool.ver -o imageload.so imageload.o $(INTEL_PIN_LIB_PATHS) -lpin -lxed -ldwarf -lelf -ldl
#	g++ -g -shared -Wl,-Bsymbolic -Wl,--version-script=$(INTEL_PIN_PATH)/source/include/pintool.ver -o imageload.so imageload.o $(INTEL_PIN_LIB_PATHS) -lpin -lxed -ldl $(ROSE_DWARF_LIBS_WITH_PATH)
	g++ -g -shared -Wl,-Bsymbolic -Wl,--version-script=$(INTEL_PIN_PATH)/source/include/pintool.ver -o imageload.so imageload.o $(INTEL_PIN_WITH_DWARF_LIBS_PATH)

# Test example program generate a graph of the execution trace of a binary
testImageload: imageload.so input_testcode
	$(INTEL_PIN_PATH)/pin -slow_asserts -t imageload.so -- ./input_testcode


# **************************************************************************
# This Pin tool generates log of the information take from each image (main program and each loaded shared library) of the binary
dynamicBinaryInfo.so: dynamicBinaryInfo.o
#	g++ -g -shared -Wl,-Bsymbolic -Wl,--version-script=$(INTEL_PIN_PATH)/source/include/pintool.ver -o dynamicBinaryInfo.so dynamicBinaryInfo.o $(INTEL_PIN_LIB_PATHS) -lpin -lxed -ldwarf -lelf -ldl
#	g++ -g -shared -Wl,-Bsymbolic -Wl,--version-script=$(INTEL_PIN_PATH)/source/include/pintool.ver -o dynamicBinaryInfo.so dynamicBinaryInfo.o $(INTEL_PIN_LIB_PATHS) -lpin -lxed -ldl $(ROSE_DWARF_LIBS_WITH_PATH)
	g++ -g -shared -Wl,-Bsymbolic -Wl,--version-script=$(INTEL_PIN_PATH)/source/include/pintool.ver -o dynamicBinaryInfo.so dynamicBinaryInfo.o $(INTEL_PIN_WITH_DWARF_LIBS_PATH)

# Test example program generates information about the binary (from section data for main program and each shared library).
testDynamicBinaryInfo: dynamicBinaryInfo.so input_testcode
	$(INTEL_PIN_PATH)/pin -slow_asserts -t dynamicBinaryInfo.so -- ./input_testcode

# **************************************************************************
# This Pin tool is a start an a tool to write out the memory image of a process (ongoing work).
memcopy-elf.so: memcopy-elf.o
#	g++ -g -shared -Wl,-Bsymbolic -Wl,--version-script=$(INTEL_PIN_PATH)/source/include/pintool.ver -o memcopy-elf.so memcopy-elf.o $(INTEL_PIN_LIB_PATHS) -lpin -lxed -ldwarf -lelf -ldl
#	g++ -g -shared -Wl,-Bsymbolic -Wl,--version-script=$(INTEL_PIN_PATH)/source/include/pintool.ver -o memcopy-elf.so memcopy-elf.o $(INTEL_PIN_LIB_PATHS) -lpin -lxed -ldl $(ROSE_DWARF_LIBS_WITH_PATH)
	g++ -g -shared -Wl,-Bsymbolic -Wl,--version-script=$(INTEL_PIN_PATH)/source/include/pintool.ver -o memcopy-elf.so memcopy-elf.o $(INTEL_PIN_WITH_DWARF_LIBS_PATH)

# Test example program to dump memory image of process (ongoing work).
testMemcopy: memcopy-elf.so input_testcode
	$(INTEL_PIN_PATH)/pin -slow_asserts -t memcopy-elf.so -- ./input_testcode
# **************************************************************************


# **************************************************************************
# This builds an Intel Pin tool (for testing the command line handling with ROSE)
commandlineHandling.so: commandlineHandling.o
	g++ -g -shared -Wl,-Bsymbolic -Wl,--version-script=$(INTEL_PIN_PATH)/source/include/pintool.ver -o commandlineHandling.so commandlineHandling.o $(NON_LIBTOOL_RPATH)

testCommandlineHandling: input_testcode commandlineHandling.so
	$(INTEL_PIN_PATH)/pin -slow_asserts -t commandlineHandling.so -- ./input_testcode


# **************************************************************************
# This builds an Intel Pin tool (for use with Intel Pin dynamic instrumentation) from a ROSE translator.
# This allows for mixed static and dynamic analysis.
dynamicStaticBinaryAnalysis.so: dynamicStaticBinaryAnalysis.o
	g++ -g -shared -Wl,-Bsymbolic -Wl,--version-script=$(INTEL_PIN_PATH)/source/include/pintool.ver -o dynamicStaticBinaryAnalysis.so dynamicStaticBinaryAnalysis.o $(NON_LIBTOOL_RPATH)

# DQ (3/9/2009): This test fails after the Pin Tool is called due to a linking problem with the -lrose shared library.
# This runs the Intel Pin dynamic instumentation on "input_testcode" using the ROSE static analysis tool
# In this case the dynamic instrumentation can call static analysis.
testDynamicStaticBinaryAnalysis: input_testcode dynamicStaticBinaryAnalysis.so
	$(INTEL_PIN_PATH)/pin -slow_asserts -t dynamicStaticBinaryAnalysis.so -- ./input_testcode


# **************************************************************************
# This builds an Intel Pin tool (for use with Intel Pin dynamic instrumentation) from a ROSE translator.
# This allows for mixed static and dynamic analysis (first static analysis then dynamic analysis).
staticDynamicBinaryAnalysis.so: staticDynamicBinaryAnalysis.o
	g++ -g -shared -Wl,-Bsymbolic -Wl,--version-script=$(INTEL_PIN_PATH)/source/include/pintool.ver -o staticDynamicBinaryAnalysis.so staticDynamicBinaryAnalysis.o $(NON_LIBTOOL_RPATH)

# DQ (3/9/2009): This test fails after the Pin Tool is called due to a linking problem with the -lrose shared library.
# This runs the Intel Pin dynamic instumentation on "input_testcode" using the ROSE static analysis tool
# In this case the dynamic instrumentation can call static analysis.
testStaticDynamicBinaryAnalysis: input_testcode staticDynamicBinaryAnalysis.so
	$(INTEL_PIN_PATH)/pin -slow_asserts -t staticDynamicBinaryAnalysis.so -- input_testcode


# **************************************************************************
# This builds an Intel Pin tool (for use with Intel Pin dynamic instrumentation) from a ROSE translator.
# This allows for mixed static and dynamic analysis (first static analysis then dynamic analysis).
staticCallGraphEmbeddedTraceBinaryAnalysis.so: staticCallGraphEmbeddedTraceBinaryAnalysis.o
	g++ -g -shared -Wl,-Bsymbolic -Wl,--version-script=$(INTEL_PIN_PATH)/source/include/pintool.ver -o staticCallGraphEmbeddedTraceBinaryAnalysis.so staticCallGraphEmbeddedTraceBinaryAnalysis.o $(NON_LIBTOOL_RPATH)

# DQ (3/9/2009): This test fails after the Pin Tool is called due to a linking problem with the -lrose shared library.
# This runs the Intel Pin dynamic instumentation on "input_testcode" using the ROSE static analysis tool
# In this case the dynamic instrumentation can call static analysis.
testStaticCallGraphEmbeddedTraceBinaryAnalysis: input_testcode staticCallGraphEmbeddedTraceBinaryAnalysis.so
	$(INTEL_PIN_PATH)/pin -slow_asserts -t staticCallGraphEmbeddedTraceBinaryAnalysis.so -- input_testcode


# **************************************************************************
# This is a simple test of calling the static analysis on a binary.
testStaticBinaryAnalysis: input_testcode staticBinaryAnalysis
	./staticBinaryAnalysis input_testcode


# **************************************************************************
# Automake's testing mechanism (which defines the "make check" rule) requires passing tests.
TESTCODES = \
$(TESTCODES_REQUIRED_TO_PASS)

# QMTest allows both passing and failing tests.
ALL_TESTCODES = \
$(TESTCODES_REQUIRED_TO_PASS) \
$(TESTCODE_CURRENTLY_FAILING)

# File option to accumulate performance information about the compilation
PERFORMANCE_REPORT_OPTION = -rose:compilationPerformanceFile $(top_builddir)/PIN_ROSE_PERFORMANCE_DATA.csv

ROSE_FLAGS = -rose:verbose 0 $(PERFORMANCE_REPORT_OPTION)

VALGRIND_OPTIONS = --tool=memcheck -v --num-callers=30 --leak-check=no --error-limit=no --show-reachable=yes
# VALGRIND = /usr/apps/valgrind/new/bin/valgrind $(VALGRIND_OPTIONS)
VALGRIND =

PASSING_TEST_Objects = ${TESTCODES:.C=.o}
TEST_Objects = ${ALL_TESTCODES:.C=.o}

# $(TEST_Objects): preprocessor $(srcdir)/$(@:.o=.C)
$(TEST_Objects): 
	echo "Calling ..."
	$(VALGRIND) ./staticBinaryAnalysis $(ROSE_FLAGS) -c $(srcdir)/$(@:.o=.C)

copyFiles:
	cp $(srcdir)/*.h $(top_srcdir)/tests/CompilerTests/Pin_tests
	cp $(srcdir)/*.C $(top_srcdir)/tests/CompilerTests/Pin_tests

check-local:
	@echo "Development tests for Intel Pin tests."
#  Run this test explicitly since it has to be run using a specific rule and can't be lumped with the rest
#	These C programs must be called externally to the test codes in the "TESTCODES" make variable
#	@$(MAKE) $(PASSING_TEST_Objects)
#	@$(MAKE) testMemcopy testGraphTrace testImageload testDynamicBinaryInfo
	@$(MAKE) $(ROSE_PIN_TESTS)
	@$(MAKE) testStaticBinaryAnalysis
	@$(MAKE) testStaticDynamicBinaryAnalysis
	@$(MAKE) testDynamicStaticBinaryAnalysis
	@echo "********************************************************************************************************"
	@echo "****** ROSE/tests/roseTests/binaryTests/Pin_tests: make check rule complete (terminated normally) ******"
	@echo "********************************************************************************************************"

else
check-local:
	@echo "Intel Pin development tests NOT run.  To run these reconfigure with:"
	@echo "*******************************************************************************************************"
	@echo "*************** make check not run here: configure --with-IntelPin=<path to Intel Pin> ****************"
	@echo "*******************************************************************************************************"

# endif of configuration for ROSE_USE_INTEL_PIN
endif

EXTRA_DIST = CMakeLists.txt \
     README dynamicStaticBinaryAnalysis.C staticDynamicBinaryAnalysis.C dynamicBinaryInfo.C \
     graphTrace.C imageload.C input_testcode.C memcopy-elf.C staticBinaryAnalysis.C \
     commandlineHandling.C

clean-local:
	rm -f *.o rose_*.[s] *.dot *.pdf *~ *.ps *.out *.new *.dump rose_performance_report_lockfile.lock *.so
	rm -f pinRoseTranslator.so  input_testcode pin.log
	rm -rf QMTest

print:
	@echo "INTEL_PIN_WITH_DWARF_LIBS_PATH = $(INTEL_PIN_WITH_DWARF_LIBS_PATH)"

