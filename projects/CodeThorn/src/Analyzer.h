#ifndef ANALYZER_H
#define ANALYZER_H

/*************************************************************
 * Copyright: (C) 2012 by Markus Schordan                    *
 * Author   : Markus Schordan                                *
 * License  : see file LICENSE in the CodeThorn distribution *
 *************************************************************/

#include <iostream>
#include <fstream>
#include <set>
#include <string>
#include <sstream>
#include <list>

#include <omp.h>

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

#include "AstTerm.h"
#include "Labeler.h"
#include "CFAnalysis.h"
#include "RoseAst.h"
#include "SgNodeHelper.h"
#include "ExprAnalyzer.h"
#include "StateRepresentations.h"
#include "TransitionGraph.h"
#include "PropertyValueTable.h"
#include "CTIOLabeler.h"
#include "VariableValueMonitor.h"

// we use INT_MIN, INT_MAX
#include "limits.h"

namespace CodeThorn {

/*! 
  * \author Markus Schordan
  * \date 2012.
 */
  class AstNodeInfo : public AstAttribute {
  public:
    // MS 2016: necessary with the new attribute mechanism but possibly not
    // necessary if the mechanism is adapted. Look for
    // (*i)->addNewAttribute("info",attr); in Analyzer.C
    virtual std::string attribute_class_name() const {
      return "AstNodeInfo";
    }
    virtual AstNodeInfo* copy() {
      AstNodeInfo* newNodeInfo=new AstNodeInfo();
      newNodeInfo->label=this->label;
      newNodeInfo->initialLabel=this->initialLabel;
      newNodeInfo->finalLabelsSet=this->finalLabelsSet;
      return newNodeInfo;
    }
    AstNodeInfo::OwnershipPolicy
      getOwnershipPolicy() const ROSE_OVERRIDE {
      return CONTAINER_OWNERSHIP;
    }

  AstNodeInfo():label(0),initialLabel(0){}
    std::string toString() { std::stringstream ss;
      ss<<"\\n lab:"<<label<<" ";
      ss<<"init:"<<initialLabel<<" ";
      ss<<"final:"<<finalLabelsSet.toString();
      return ss.str(); 
    }
    void setLabel(Label l) { label=l; }
    void setInitialLabel(Label l) { initialLabel=l; }
    void setFinalLabels(LabelSet lset) { finalLabelsSet=lset; }
  private:
    Label label;
    Label initialLabel;
    LabelSet finalLabelsSet;
  };

  typedef std::list<const EState*> EStateWorkList;
  typedef std::pair<int, const EState*> FailedAssertion;
  enum AnalyzerMode { AM_ALL_STATES, AM_LTL_STATES };

/*! 
  * \author Markus Schordan
  * \date 2012.
 */
  class Analyzer {
    friend class Visualizer;
    friend class VariableValueMonitor;
  public:
    
    Analyzer();
    ~Analyzer();
    
    void initAstNodeInfo(SgNode* node);
    bool isActiveGlobalTopify();
    static std::string nodeToString(SgNode* node);
    void initializeSolver1(std::string functionToStartAt,SgNode* root, bool oneFunctionOnly);
    void initializeTraceSolver(std::string functionToStartAt,SgNode* root);
    void continueAnalysisFrom(EState* newStartEState);
    
    PState analyzeAssignRhs(PState currentPState,VariableId lhsVar, SgNode* rhs,ConstraintSet& cset);
    EState analyzeVariableDeclaration(SgVariableDeclaration* nextNodeToAnalyze1,EState currentEState, Label targetLabel);
    std::list<EState> transferFunction(Edge edge, const EState* estate);
    
    void addToWorkList(const EState* estate);
    const EState* addToWorkListIfNew(EState estate);
    const EState* takeFromWorkList();
    bool isInWorkList(const EState* estate);
    bool isEmptyWorkList();
    const EState* topWorkList();
    const EState* popWorkList();
    void swapWorkLists();
    
    void recordTransition(const EState* sourceEState, Edge e, const EState* targetEState);
    void printStatusMessage(bool);
    bool isLTLRelevantEState(const EState* estate);
    bool isLTLRelevantLabel(Label label);
    bool isStdIOLabel(Label label);
    bool isStartLabel(Label label);
    std::set<const EState*> nonLTLRelevantEStates();
    bool isTerminationRelevantLabel(Label label);
    
    // 6 experimental functions
    // reduces all states different to stdin and stdout.
    void stdIOFoldingOfTransitionGraph();
    void semanticFoldingOfTransitionGraph();
    // requires semantically reduced STG
    bool checkEStateSet();
    bool isConsistentEStatePtrSet(std::set<const EState*> estatePtrSet);
    bool checkTransitionGraph();

    // bypasses and removes all states that are not standard I/O states
    // (old version, works correctly, but has a long execution time)
    void removeNonIOStates();
    // bypasses and removes all states that are not stdIn/stdOut/stdErr/failedAssert states
    void reduceToObservableBehavior();
    // erases transitions that lead directly from one output state to another output state
    void removeOutputOutputTransitions();
    // erases transitions that lead directly from one input state to another input state
    void removeInputInputTransitions();
    // cuts off all paths in the transition graph that lead to leaves 
    // (recursively until only paths of infinite length remain)
    void pruneLeavesRec();
    // connects start, input, output and worklist states according to possible paths in the transition graph. 
    // removes all states and transitions that are not necessary for the graph that only consists of these new transitions. The two parameters allow to select input and/or output states to remain in the STG.
    void reduceGraphInOutWorklistOnly(bool includeIn=true, bool includeOut=true, bool includeErr=false);
    // extracts input sequences leading to each discovered failing assertion where discovered for the first time.
    // stores results in PropertyValueTable "reachabilityResults".
    // returns length of the longest of these sequences if it can be guaranteed that all processed traces are the
    // shortest ones leading to the individual failing assertion (returns -1 otherwise).
    int extractAssertionTraces();

    // determines whether lab is a function call label of a function call of the form 'x=f(...)' and returns the varible-id of the lhs, if it exists.
    bool isFunctionCallWithAssignment(Label lab,VariableId* varId=0);
  private:

    // only used in LTL-driven mode
    void setStartEState(const EState* estate);

    /*! if state exists in stateSet, a pointer to the existing state is returned otherwise 
      a new state is entered into stateSet and a pointer to it is returned.
    */
    const PState* processNew(PState& s);
    const PState* processNewOrExisting(PState& s);
    const EState* processNew(EState& s);
    const EState* processNewOrExisting(EState& s);
    const EState* processCompleteNewOrExisting(const EState* es);
    void topifyVariable(PState& pstate, ConstraintSet& cset, VariableId varId);
    bool isTopified(EState& s);
    EStateSet::ProcessingResult process(EState& s);
    EStateSet::ProcessingResult process(Label label, PState pstate, ConstraintSet cset, InputOutput io);
    const ConstraintSet* processNewOrExisting(ConstraintSet& cset);
    
    EState createEStateFastTopifyMode(Label label, const PState* oldPStatePtr, const ConstraintSet* oldConstraintSetPtr);
    EState createEState(Label label, PState pstate, ConstraintSet cset);
    EState createEState(Label label, PState pstate, ConstraintSet cset, InputOutput io);

    //returns a list of transitions representing existing paths from "startState" to all possible input/output/error states (no output -> output)
    // collection of transitions to worklist states currently disabled. the returned set has to be deleted by the calling function.
    boost::unordered_set<Transition*>* transitionsToInOutErrAndWorklist( const EState* startState, 
								      bool includeIn, bool includeOut, bool includeErr);                                                          
    boost::unordered_set<Transition*>* transitionsToInOutErrAndWorklist( const EState* currentState, const EState* startState, 
                                                            	      boost::unordered_set<Transition*>* results, boost::unordered_set<const EState*>* visited,
								      bool includeIn, bool includeOut, bool includeErr);
    // adds a string representation of the shortest input path from start state to assertEState to reachabilityResults. returns the length of the 
    // counterexample input sequence.
    int addCounterexample(int assertCode, const EState* assertEState);
    // returns a list of EStates from source to target. Target has to come before source in the STG (reversed trace). 
    std::list<const EState*>reverseInOutSequenceBreadthFirst(const EState* source, const EState* target, bool counterexampleWithOutput = false);
    // returns a list of EStates from source to target (shortest input path). 
    // please note: target has to be a predecessor of source (reversed trace)
    std::list<const EState*> reverseInOutSequenceDijkstra(const EState* source, const EState* target, bool counterexampleWithOutput = false);
    std::list<const EState*> filterStdInOutOnly(std::list<const EState*>& states, bool counterexampleWithOutput = false) const;
    std::string reversedInOutRunToString(std::list<const EState*>& run);
    //returns the shortest possible number of input states on the path leading to "target".
    int inputSequenceLength(const EState* target);
    // the following functions are used by solver 9
    bool searchForIOPatterns(PState* startPState, int assertion_id, std::list<int>& inputSuffix, std::list<int>* partialTrace = NULL, int* inputPatternLength=NULL);
    bool containsPatternTwoRepetitions(std::list<int>& sequence);
    bool containsPatternTwoRepetitions(std::list<int>& sequence, int startIndex, int endIndex);
    bool computePStateAfterInputs(PState& pState, std::list<int>& inputs, int thread_id, std::list<int>* iOSequence=NULL);
    bool computePStateAfterInputs(PState& pState, int input, int thread_id, std::list<int>* iOSequence=NULL);
    bool searchPatternPath(int assertion_id, PState& pState, std::list<int>& inputPattern, std::list<int>& inputSuffix, int thread_id,std::list<int>* iOSequence=NULL);
    std::list<int> inputsFromPatternTwoRepetitions(std::list<int> pattern2r);
    string convertToCeString(std::list<int>& ceAsIntegers, int maxInputVal);
    int pStateDepthFirstSearch(PState* startPState, int maxDepth, int thread_id, std::list<int>* partialTrace, int maxInputVal, int patternLength, int PatternIterations);

  public:
    SgNode* getCond(SgNode* node);
    void generateAstNodeInfo(SgNode* node);
    std::string generateSpotSTG();
  private:
    void generateSpotTransition(std::stringstream& ss, const Transition& t);
    //less than comarisions on two states according to (#input transitions * #output transitions)
    bool indegreeTimesOutdegreeLessThan(const EState* a, const EState* b);
  public:
    //stores a backup of the created transitionGraph
    void storeStgBackup();
    //load previous backup of the transitionGraph, storing the current version as a backup instead
    void swapStgWithBackup();
    //solver 8 becomes the active solver used by the analyzer. Deletion of previous data iff "resetAnalyzerData" is set to true.
    void setAnalyzerToSolver8(EState* startEState, bool resetAnalyzerData);
    //! requires init
    void runSolver4();
    void runSolver5();
    void runSolver8();
    void runSolver9();
    void runSolver10();
    void runSolver11();
    void runSolver12();
    void runSolver();
    // first: list of new states (worklist), second: set of found existing states
    typedef pair<EStateWorkList,EStateSet> SubSolverResultType;
    SubSolverResultType subSolver(const EState* currentEStatePtr);
    //! The analyzer requires a CFAnalysis to obtain the ICFG.
    void setCFAnalyzer(CFAnalysis* cf) { cfanalyzer=cf; }
    CFAnalysis* getCFAnalyzer() const { return cfanalyzer; }
    
    //void initializeVariableIdMapping(SgProject* project) { variableIdMapping.computeVariableSymbolMapping(project); }

    // access  functions for computed information
    VariableIdMapping* getVariableIdMapping() { return &variableIdMapping; }
    CTIOLabeler* getLabeler() const {
      CTIOLabeler* ioLabeler=dynamic_cast<CTIOLabeler*>(cfanalyzer->getLabeler());
      ROSE_ASSERT(ioLabeler);
      return ioLabeler;
    }
    Flow* getFlow() { return &flow; }
    PStateSet* getPStateSet() { return &pstateSet; }
    EStateSet* getEStateSet() { return &estateSet; }
    TransitionGraph* getTransitionGraph() { return &transitionGraph; }
    ConstraintSetMaintainer* getConstraintSetMaintainer() { return &constraintSetMaintainer; }
    
    //private: TODO
    Flow flow;
    SgNode* startFunRoot;
    CFAnalysis* cfanalyzer;
    VariableValueMonitor variableValueMonitor;
    void setVariableValueThreshold(int threshold) { variableValueMonitor.setThreshold(threshold); }
  public:
    //! compute the VariableIds of variable declarations
    VariableIdMapping::VariableIdSet determineVariableIdsOfVariableDeclarations(set<SgVariableDeclaration*> decls);
    //! compute the VariableIds of SgInitializedNamePtrList
    VariableIdMapping::VariableIdSet determineVariableIdsOfSgInitializedNames(SgInitializedNamePtrList& namePtrList);
    
    std::set<std::string> variableIdsToVariableNames(VariableIdMapping::VariableIdSet);
    typedef std::list<SgVariableDeclaration*> VariableDeclarationList;
    VariableDeclarationList computeUnusedGlobalVariableDeclarationList(SgProject* root);
    VariableDeclarationList computeUsedGlobalVariableDeclarationList(SgProject* root);
    
    bool isFailedAssertEState(const EState* estate);
    bool isVerificationErrorEState(const EState* estate);
    //! adds a specific code to the io-info of an estate which is checked by isFailedAsserEState and determines a failed-assert estate. Note that the actual assert (and its label) is associated with the previous estate (this information can therefore be obtained from a transition-edge in the transition graph).
    EState createFailedAssertEState(const EState estate, Label target);
    EState createVerificationErrorEState(const EState estate, Label target);
    //! list of all asserts in a program
    std::list<SgNode*> listOfAssertNodes(SgProject *root);
    //! rers-specific error_x: assert(0) version 
    std::list<std::pair<SgLabelStatement*,SgNode*> > listOfLabeledAssertNodes(SgProject *root);
    void initLabeledAssertNodes(SgProject* root) {
      _assertNodes=listOfLabeledAssertNodes(root);
    }
    size_t getNumberOfErrorLabels();
    std::string labelNameOfAssertLabel(Label lab) {
      std::string labelName;
      for(std::list<std::pair<SgLabelStatement*,SgNode*> >::iterator i=_assertNodes.begin();i!=_assertNodes.end();++i)
        if(lab==getLabeler()->getLabel((*i).second))
          labelName=SgNodeHelper::getLabelName((*i).first);
      //assert(labelName.size()>0);
      return labelName;
    }
    bool isCppLabeledAssertLabel(Label lab) {
      return labelNameOfAssertLabel(lab).size()>0;
    }
    
    InputOutput::OpType ioOp(const EState* estate) const;

    PropertyValueTable* loadAssertionsToReconstruct(string filePath);
    
    void setDisplayDiff(int diff) { _displayDiff=diff; }
    void setSolver(int solver) { _solver=solver; }
    int getSolver() { return _solver;}
    void setSemanticFoldThreshold(int t) { _semanticFoldThreshold=t; }
    void setNumberOfThreadsToUse(int n) { _numberOfThreadsToUse=n; }
    int getNumberOfThreadsToUse() { return _numberOfThreadsToUse; }
    void insertInputVarValue(int i) { _inputVarValues.insert(i); }
    void addInputSequenceValue(int i) { _inputSequence.push_back(i); }
    void resetToEmptyInputSequence() { _inputSequence.clear(); }
    void resetInputSequenceIterator() { _inputSequenceIterator=_inputSequence.begin(); }
    const EState* getEstateBeforeMissingInput() {return _estateBeforeMissingInput;}
    const EState* getLatestErrorEState() {return _latestErrorEState;}
    void setTreatStdErrLikeFailedAssert(bool x) { _treatStdErrLikeFailedAssert=x; }
    int numberOfInputVarValues() { return _inputVarValues.size(); }
    std::set<int> getInputVarValues() { return _inputVarValues; }
    std::list<std::pair<SgLabelStatement*,SgNode*> > _assertNodes;
    VariableId globalVarIdByName(std::string varName) { return globalVarName2VarIdMapping[varName]; }
    void setStgTraceFileName(std::string filename) {
      _stg_trace_filename=filename;
      std::ofstream fout;
      fout.open(_stg_trace_filename.c_str());    // create new file/overwrite existing file
      fout<<"START"<<endl;
      fout.close();    // close. Will be used with append.
    }
  private:
    std::string _stg_trace_filename;
 public:
    // only used temporarily for binary-binding prototype
    std::map<std::string,VariableId> globalVarName2VarIdMapping;
    std::vector<bool> binaryBindingAssert;
    void setAnalyzerMode(AnalyzerMode am) { _analyzerMode=am; }
    void setMaxTransitions(size_t maxTransitions) { _maxTransitions=maxTransitions; }
    void setMaxIterations(size_t maxIterations) { _maxIterations=maxIterations; }
    void setMaxTransitionsForcedTop(size_t maxTransitions) { _maxTransitionsForcedTop=maxTransitions; }
    void setMaxIterationsForcedTop(size_t maxIterations) { _maxIterationsForcedTop=maxIterations; }
    void setStartPState(PState startPState) { _startPState=startPState; }
    void setReconstructMaxInputDepth(size_t inputDepth) { _reconstructMaxInputDepth=inputDepth; }
    void setReconstructMaxRepetitions(size_t repetitions) { _reconstructMaxRepetitions=repetitions; }
    void setReconstructPreviousResults(PropertyValueTable* previousResults) { _reconstructPreviousResults = previousResults; };
    void setPatternSearchMaxDepth(size_t iODepth) { _patternSearchMaxDepth=iODepth; }
    void setPatternSearchRepetitions(size_t patternReps) { _patternSearchRepetitions=patternReps; }
    void setPatternSearchMaxSuffixDepth(size_t suffixDepth) { _patternSearchMaxSuffixDepth=suffixDepth; }
    void setPatternSearchAssertTable(PropertyValueTable* patternSearchAsserts) { _patternSearchAssertTable = patternSearchAsserts; };
    enum ExplorationMode { EXPL_DEPTH_FIRST, EXPL_BREADTH_FIRST, EXPL_LOOP_AWARE, EXPL_LOOP_AWARE_SYNC, EXPL_RANDOM_MODE1 };
    void setPatternSearchExploration(ExplorationMode explorationMode) { _patternSearchExplorationMode = explorationMode; };
    void eventGlobalTopifyTurnedOn();
    bool isIncompleteSTGReady();
    bool isPrecise();
    PropertyValueTable reachabilityResults;
    int reachabilityAssertCode(const EState* currentEStatePtr);
    void setExplorationMode(ExplorationMode em) { _explorationMode=em; }
    ExplorationMode getExplorationMode() { return _explorationMode; }
    void setSkipSelectedFunctionCalls(bool defer) {
      _skipSelectedFunctionCalls=true; 
      exprAnalyzer.setSkipSelectedFunctionCalls(true);
    }
    void setSkipArrayAccesses(bool skip) {
      exprAnalyzer.setSkipArrayAccesses(skip);
    }
    bool getSkipArrayAccesses() {
      return exprAnalyzer.getSkipArrayAccesses();
    }
    ExprAnalyzer* getExprAnalyzer();
    std::list<FailedAssertion> getFirstAssertionOccurences(){return _firstAssertionOccurences;}
    void incIterations() {
      if(isPrecise()) {
#pragma omp atomic
        _iterations+=1;
      } else {
#pragma omp atomic
        _approximated_iterations+=1;
      }
    }
    bool isLoopCondLabel(Label lab);
    int getApproximatedIterations() { return _approximated_iterations; }
    int getIterations() { return _iterations; }
    string getVarNameByIdCode(int varIdCode) {return variableIdMapping.variableName(variableIdMapping.variableIdFromCode(varIdCode));};
    void mapGlobalVarInsert(std::string name, int* addr);
  public:
    boost::unordered_map <std::string,int*> mapGlobalVarAddress;
    boost::unordered_map <int*,std::string> mapAddressGlobalVar;
    void setCompoundIncVarsSet(set<VariableId> ciVars);
    void setSmallActivityVarsSet(set<VariableId> ciVars);
    void setAssertCondVarsSet(set<VariableId> acVars);
    enum GlobalTopifyMode {GTM_IO, GTM_IOCF, GTM_IOCFPTR, GTM_COMPOUNDASSIGN, GTM_FLAGS};
    void setGlobalTopifyMode(GlobalTopifyMode mode);
    void setExternalErrorFunctionName(std::string externalErrorFunctionName);
    // enables external function semantics 
    void enableExternalFunctionSemantics();
    void disableExternalFunctionSemantics();
    bool isUsingExternalFunctionSemantics() { return _externalFunctionSemantics; }
    void setModeLTLDriven(bool ltlDriven) { transitionGraph.setModeLTLDriven(ltlDriven); }
    bool getModeLTLDriven() { return transitionGraph.getModeLTLDriven(); }
  private:
    GlobalTopifyMode _globalTopifyMode;
    set<VariableId> _compoundIncVarsSet;
    set<VariableId> _smallActivityVarsSet;
    set<VariableId> _assertCondVarsSet;
    set<int> _inputVarValues;
    std::list<int> _inputSequence;
    std::list<int>::iterator _inputSequenceIterator;
    ExprAnalyzer exprAnalyzer;
    VariableIdMapping variableIdMapping;
    EStateWorkList* estateWorkListCurrent;
    EStateWorkList* estateWorkListNext;
    EStateWorkList estateWorkListOne;
    EStateWorkList estateWorkListTwo;
    EStateSet estateSet;
    PStateSet pstateSet;
    ConstraintSetMaintainer constraintSetMaintainer;
    TransitionGraph transitionGraph;
    TransitionGraph backupTransitionGraph;
    set<const EState*> transitionSourceEStateSetOfLabel(Label lab);
    int _displayDiff;
    int _numberOfThreadsToUse;
    int _semanticFoldThreshold;
    VariableIdMapping::VariableIdSet _variablesToIgnore;
    int _solver;
    AnalyzerMode _analyzerMode;
    set<const EState*> _newNodesToFold;
    long int _maxTransitions;
    long int _maxIterations;
    long int _maxTransitionsForcedTop;
    long int _maxIterationsForcedTop;
    PState _startPState;
    int _reconstructMaxInputDepth;
    int _reconstructMaxRepetitions;
    PropertyValueTable* _reconstructPreviousResults;
    PropertyValueTable*  _patternSearchAssertTable;
    int _patternSearchMaxDepth;
    int _patternSearchRepetitions;
    int _patternSearchMaxSuffixDepth;
    ExplorationMode _patternSearchExplorationMode;
    bool _treatStdErrLikeFailedAssert;
    bool _skipSelectedFunctionCalls;
    ExplorationMode _explorationMode;
    std::list<FailedAssertion> _firstAssertionOccurences;
    const EState* _estateBeforeMissingInput;
    const EState* _latestOutputEState;
    const EState* _latestErrorEState;
    bool _topifyModeActive;
    int _swapWorkListsCount; // currently only used for debugging purposes
    int _iterations;
    int _approximated_iterations;
    int _curr_iteration_cnt;
    int _next_iteration_cnt;
    bool _externalFunctionSemantics;
    string _externalErrorFunctionName; // the call of this function causes termination of analysis
    string _externalNonDetIntFunctionName;
    string _externalNonDetLongFunctionName;
    string _externalExitFunctionName;
    //bool _modeLTLDriven;
  }; // end of class Analyzer
  
} // end of namespace CodeThorn

#include "RersSpecialization.h"

#endif
