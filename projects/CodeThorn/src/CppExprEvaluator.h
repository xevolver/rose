#ifndef CPPEXPREVALUATOR_H
#define CPPEXPREVALUATOR_H

#include "VariableIdMapping.h"
#include "PropertyState.h"
#include "NumberIntervalLattice.h"
#include "PointerAnalysisInterface.h"

namespace SPRAY {
class CppExprEvaluator {
 public:
  CppExprEvaluator(NumberIntervalLattice* d, VariableIdMapping* vim);
  NumberIntervalLattice evaluate(SgNode* node);
  NumberIntervalLattice evaluate(SgNode* node, PropertyState* pstate);
  void setDomain(NumberIntervalLattice* domain);
  void setPropertyState(PropertyState* pstate);
  void setVariableIdMapping(VariableIdMapping* variableIdMapping);
  bool isValid();
  void setShowWarnings(bool warnings);
  void setPointerAnalysis(SPRAY::PointerAnalysisInterface* pointerAnalysisInterface);
  void setSoundness(bool s);
 private:
  bool isExprRootNode(SgNode* node);
  SgNode* findExprRootNode(SgNode* node);
  NumberIntervalLattice* domain;
  VariableIdMapping* variableIdMapping;
  PropertyState* propertyState;
  bool _showWarnings;
  SPRAY::PointerAnalysisInterface* _pointerAnalysisInterface;
  bool _sound;
};
}
#endif
