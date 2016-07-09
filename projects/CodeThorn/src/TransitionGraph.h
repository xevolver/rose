#ifndef TRANSITION_GRAPH
#define TRANSITION_GRAPH

#include "StateRepresentations.h"

namespace CodeThorn {
  /*! 
   * \author Markus Schordan
   * \date 2012.
   */
  class Transition {
  public:
    Transition();
    Transition(const EState* source,Edge edge, const EState* target);
    const EState* source; // source node
    Edge edge;
    const EState* target; // target node
    string toString() const;
  private:
  };
  
  /*! 
   * \author Markus Schordan
   * \date 2012.
   */
  class TransitionHashFun {
  public:
    TransitionHashFun();
    long operator()(Transition* s) const;
  private:
  };
  
  class TransitionEqualToPred {
  public:
    TransitionEqualToPred();
    bool operator()(Transition* t1, Transition* t2) const;
  private:
  };
  
  bool operator==(const Transition& t1, const Transition& t2);
  bool operator!=(const Transition& t1, const Transition& t2);
  bool operator<(const Transition& t1, const Transition& t2);
  
  typedef set<const Transition*> TransitionPtrSet;
  typedef set<const EState*> EStatePtrSet;
  
  /*! 
   * \author Markus Schordan
   * \date 2012.
   */
  class Analyzer;
  class TransitionGraph : public HSetMaintainer<Transition,TransitionHashFun,TransitionEqualToPred> {
  public:
    typedef set<const Transition*> TransitionPtrSet;
    TransitionGraph();
    void setModeLTLDriven(bool mode) { _modeLTLDriven=mode; }
    bool getModeLTLDriven() { return _modeLTLDriven; }
    EStatePtrSet transitionSourceEStateSetOfLabel(Label lab);
    EStatePtrSet estateSetOfLabel(Label lab);
    EStatePtrSet estateSet();
    long numberOfObservableStates(bool inlcudeIn=true, bool includeOut=true, bool includeErr=true);
    void add(Transition trans);
    string toString() const;
    LabelSet labelSetOfIoOperations(InputOutput::OpType op);
    Label getStartLabel() { assert(_startLabel!=Label()); return _startLabel; }
    void setStartLabel(Label lab) { _startLabel=lab; }
    // this allows to deal with multiple start transitions (must share same start state)
    const EState* getStartEState();
    void setStartEState(const EState* estate);
    Transition getStartTransition();

    void erase(TransitionGraph::iterator transiter);
    void erase(const Transition trans);

    //! deprecated
    void reduceEStates(set<const EState*> toReduce);
    void reduceEState(const EState* estate);
    //! reduces estates. Adds edge-annotation PATH. Structure preserving by remapping existing edges.
    void reduceEStates2(set<const EState*> toReduce);
    void reduceEState2(const EState* estate); // used for semantic folding
    TransitionPtrSet inEdges(const EState* estate);
    TransitionPtrSet outEdges(const EState* estate);
    EStatePtrSet pred(const EState* estate);
    EStatePtrSet succ(const EState* estate);
    bool checkConsistency();
    const Transition* hasSelfEdge(const EState* estate);
    // deletes EState and *deletes* all ingoing and outgoing transitions
    void eliminateEState(const EState* estate);
    int eliminateBackEdges();
    void determineBackEdges(const EState* state, set<const EState*>& visited, TransitionPtrSet& tpSet);
    void setIsPrecise(bool v);
    void setIsComplete(bool v);
    bool isPrecise();
    bool isComplete();
    void setAnalyzer(Analyzer* analyzer) {
      ROSE_ASSERT(getModeLTLDriven());
      _analyzer=analyzer;
    }
 private:
    Label _startLabel;
    int _numberOfNodes; // not used yet
    map<const EState*,TransitionPtrSet > _inEdges;
    map<const EState*,TransitionPtrSet > _outEdges;
    set<const EState*> _recomputedestateSet;
    bool _preciseSTG;
    bool _completeSTG;
    bool _modeLTLDriven;

    // only used by ltl-driven mode in function succ
    Analyzer* _analyzer;
    // only used by ltl-driven mode in function succ
    const EState* _startEState;
  };
}
#endif
