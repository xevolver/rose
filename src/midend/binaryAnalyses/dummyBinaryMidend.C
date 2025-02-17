#include "sage3basic.h"
#include "BinaryDataFlow.h"
#include "BinaryTaintedFlow.h"
#include "BinaryNoOperation.h"
#include "BinaryCallingConvention.h"

namespace rose {
namespace BinaryAnalysis {

void DataFlow::initDiagnostics() {}
void TaintedFlow::initDiagnostics() {}
void InstructionSemantics2::initDiagnostics() {}
void NoOperation::initDiagnostics() {}
void CallingConvention::initDiagnostics() {}

namespace PointerDetection {
    void initDiagnostics() {}
}

namespace StackDelta {
    void initDiagnostics() {}
}

namespace Strings {
    void initDiagnostics() {}
}


} // namespace
} // namespace
