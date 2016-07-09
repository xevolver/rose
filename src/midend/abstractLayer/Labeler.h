#ifndef LABELER_H
#define LABELER_H

/*************************************************************
 * Copyright: (C) 2012 Markus Schordan                       *
 * Author   : Markus Schordan                                *
 *************************************************************/

#include <set>
#include "RoseAst.h"
#include "VariableIdMapping.h"

#define NO_STATE -3
#define NO_ESTATE -4
#define NO_LABEL_ID -1

namespace SPRAY {

/*! 
  * \author Markus Schordan
  * \date 2012, 2014.
 */
class Label {
 public:
  Label();
  Label(size_t labelId);
  //Copy constructor
  Label(const Label& other);
  //Copy assignemnt operator
  Label& operator=(const Label& other);
  bool operator<(const Label& other) const;
  bool operator==(const Label& other) const;
  bool operator!=(const Label& other) const;
  bool operator>(const Label& other) const;
  bool operator>=(const Label& other) const;
  Label& operator+(int num);
  // prefix inc operator
  Label& operator++();
  // postfix inc operator
  Label operator++(int);
  size_t getId() const;

  friend std::ostream& operator<<(std::ostream& os, const Label& label);

 protected:
  size_t _labelId;
};

std::ostream& operator<<(std::ostream& os, const Label& label);

/*! 
  * \author Markus Schordan
  * \date 2012.
 */

// internal data structure (not used in Labeler's interface)
class LabelProperty {
 public:
   enum LabelType { LABEL_UNDEF=1, LABEL_OTHER=2, 
                    LABEL_FUNCTIONCALL=100, LABEL_FUNCTIONCALLRETURN,
                    LABEL_FUNCTIONENTRY, LABEL_FUNCTIONEXIT,
                    LABEL_BLOCKBEGIN, LABEL_BLOCKEND,
                    LABEL_EMPTY_STMT
   };
   std::string labelTypeToString(LabelType lt);

   LabelProperty();
   LabelProperty(SgNode* node);
   LabelProperty(SgNode* node, LabelType labelType);
   LabelProperty(SgNode* node, VariableIdMapping* variableIdMapping);
   LabelProperty(SgNode* node, LabelType labelType, VariableIdMapping* variableIdMapping);
   std::string toString();
   SgNode* getNode();
   bool isFunctionCallLabel();
   bool isFunctionCallReturnLabel();
   bool isFunctionEntryLabel();
   bool isFunctionExitLabel();
   bool isBlockBeginLabel();
   bool isBlockEndLabel();
   bool isEmptyStmtLabel();
 public:
   void initializeIO(VariableIdMapping* variableIdMapping);

 public:
   enum IOType { LABELIO_NONE, LABELIO_STDIN, LABELIO_STDOUTVAR, LABELIO_STDOUTCONST, LABELIO_STDERR
   };

   bool isStdOutLabel();
   bool isStdOutVarLabel();
   bool isStdOutConstLabel();
   bool isStdInLabel();
   bool isStdErrLabel();
   bool isIOLabel();
   VariableId getIOVarId();
   int getIOConst();

   void makeTerminationIrrelevant(bool t);
   bool isTerminationRelevant();
   bool isLTLRelevant();

 private:
   bool _isValid;
   SgNode* _node;
   LabelType _labelType;

 private:
   IOType _ioType;
   VariableId _variableId;
   int _ioValue;
   bool _isTerminationRelevant;
   bool _isLTLRelevant;
 };

/*! 
  * \author Markus Schordan
  * \date 2012.
 */
class LabelSet : public std::set<Label> {
 public:
  // temporary until all sets are properly using the std:algorithms for set operations
  LabelSet operator+(LabelSet& s2);
  LabelSet& operator+=(LabelSet& s2);

  std::string toString();
  bool isElement(Label lab);
};

typedef std::set<LabelSet> LabelSetSet;

/*! 
  * \author Markus Schordan
  * \date 2012, 2013.
 */
class Labeler {
 public:
  Labeler();
  static Label NO_LABEL;
  Labeler(SgNode* start);
  static std::string labelToString(Label lab);
  int isLabelRelevantNode(SgNode* node);
  virtual void createLabels(SgNode* node);

  /* Labels are numbered 0..n-1 where n is the number of labeled nodes (not all nodes are labeled).
     A return value of NO_LABEL means that this node has no label.
  */
  Label getLabel(SgNode* node);
  LabelSet getLabelSet(std::set<SgNode*>& nodeSet);

  /* Returns the node with the label 'label'. If the return value is 0 then no node exists for this label -
     this can only be the case if label is errornously higher than the number of labeled nodes or NO_LABEL.
  */
  SgNode* getNode(Label label);
  long numberOfLabels();
  std::string toString();
  Label functionCallLabel(SgNode* node);
  Label functionCallReturnLabel(SgNode* node);
  Label blockBeginLabel(SgNode* node);
  Label blockEndLabel(SgNode* node);
  Label functionEntryLabel(SgNode* node);
  Label functionExitLabel(SgNode* node);
  bool isFunctionEntryLabel(Label lab);
  bool isFunctionExitLabel(Label lab);
  bool isEmptyStmtLabel(Label lab);
  bool isBlockBeginLabel(Label lab);
  bool isBlockEndLabel(Label lab);
  bool isFunctionCallLabel(Label lab);
  bool isFunctionCallReturnLabel(Label lab);
  bool isConditionLabel(Label lab);
  bool isSwitchExprLabel(Label lab);
  bool isFirstLabelOfMultiLabeledNode(Label lab);
  bool isSecondLabelOfMultiLabeledNode(Label lab);
  class iterator {
  public:
    iterator();
    iterator(Label start, size_t numLabels);
    bool operator==(const iterator& x) const;
    bool operator!=(const iterator& x) const;
    Label operator*() const;
    iterator& operator++(); // prefix
    iterator operator++(int); // postfix
  private:
    bool is_past_the_end() const;
    Label _currentLabel;
    size_t _numLabels;
  };
  iterator begin();
  iterator end();
  virtual ~Labeler();
 protected:
  void computeNodeToLabelMapping();
  void registerLabel(LabelProperty);
  typedef std::vector<LabelProperty> LabelToLabelPropertyMapping;
  LabelToLabelPropertyMapping mappingLabelToLabelProperty;
  typedef std::map<SgNode*,Label> NodeToLabelMapping;
  NodeToLabelMapping mappingNodeToLabel;
  bool _isValidMappingNodeToLabel;
  void ensureValidNodeToLabelMapping();
};

class IOLabeler : public Labeler {
 public:
  IOLabeler(SgNode* start, VariableIdMapping* variableIdMapping);
  virtual bool isStdIOLabel(Label label);
  virtual bool isStdInLabel(Label label, VariableId* id=0);
  virtual bool isStdOutLabel(Label label); 
  virtual bool isStdOutVarLabel(Label label, VariableId* id=0);
  virtual bool isStdOutConstLabel(Label label, int* constvalue=0);
  virtual bool isStdErrLabel(Label label, VariableId* id=0);
  virtual  ~IOLabeler();

 protected:
  VariableIdMapping* _variableIdMapping;
};

} // end of namespace SPRAY

#endif
