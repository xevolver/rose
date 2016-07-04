#ifndef INTERVAL_PROPERTY_STATE_H
#define INTERVAL_PROPERTY_STATE_H

#include <iostream>
#include "VariableIdMapping.h"
#include "NumberIntervalLattice.h"
#include "PropertyState.h"

namespace SPRAY {
class IntervalPropertyState : public Lattice {
public:
  IntervalPropertyState();
  void toStream(std::ostream& os, VariableIdMapping* vim=0);
  bool approximatedBy(Lattice& other);
  void combine(Lattice& other);
  // adds integer variable
  void addVariable(VariableId varId);
  void setVariable(VariableId varId,NumberIntervalLattice num);
  NumberIntervalLattice getVariable(SPRAY::VariableId varId);
  SPRAY::VariableIdSet allVariableIds();
  // removes variable from state. Returns true if variable existed in state, otherwise false.
  bool variableExists(VariableId varId);
  bool removeVariable(VariableId varId);
  void topifyAllVariables();
  void topifyVariableSet(VariableIdSet varIdSet);
#if 0
  // adds pointer variable
  void addPointerVariable(VariableId);
  // adds array elements for indices 0 to number-1
  void addArrayElements(VariableId,int number);
#endif
  typedef std::map<VariableId,NumberIntervalLattice> IntervalMapType;
  bool isBot() { return _bot; }
  void setBot() { _bot=true; }
  void setEmptyState();
 private:
  IntervalMapType intervals;
  bool _bot;
};

} // end of namespace SPRAY

#endif
