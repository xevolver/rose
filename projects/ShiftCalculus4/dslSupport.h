
namespace DSL_Support
   {
     bool isMatchingClassType(SgType* type, const std::string & name, bool isTemplateClass);
     bool isMatchingMemberFunction(SgMemberFunctionRefExp* memberFunctionRefExp, const std::string & name, bool isTemplateInstantiation);

  // SgFunctionCallExp* buildMemberFunctionCall(SgExpression* expressionRoot, SgType* type, const std::string & memberFunctionName, SgExpression* expression, bool isOperator);
     SgFunctionCallExp* buildMemberFunctionCall(SgExpression* expressionRoot, const std::string & memberFunctionName, SgExpression* expression, bool isOperator);

     SgFunctionCallExp* buildMemberFunctionCall(SgVariableSymbol* variableSymbol, const std::string & memberFunctionName, SgExpression* expression, bool isOperator);

     SgVariableDeclaration* buildDataPointer(const std::string & pointerVariableName, SgVariableSymbol* variableSymbol, SgScopeStatement* outerScope);

     SgVariableDeclaration* buildBoxRef(const std::string & pointerVariableName, SgVariableSymbol* variableSymbol, SgScopeStatement* outerScope, SgType* type);

     SgVariableDeclaration* buildMultiDimPointer(const std::string & pointerVariableName, SgVariableSymbol* variableSymbol, SgScopeStatement* outerScope, std::vector<SgVariableSymbol*> SymbolArray, int dimSize);
  // More refactoring of the code in the attribute evaluation.
  // bool detectVariableDeclarationOfSpecificType (SgNode* astNode, const std::string & className, SgInitializer* & initializer);
     SgInitializedName* detectVariableDeclarationOfSpecificType (SgNode* astNode, const std::string & className);

     SgFunctionCallExp* detectMemberFunctionOfSpecificClassType(SgNode* astNode, SgInitializedName* & initializedNameUsedToCallMemberFunction, 
          const std::string & className, bool isTemplateClass, const std::string & memberFunctionName, bool isTemplateFunctionInstantiation);

  // This function supports the DSL when constness is not rigidly followed.
     SgExpression* defining_expression ( SgNode* root, SgInitializedName* variable );


  // ********************************************************
  // DQ (10/24/2014): Added feature for constant expresssion 
  // evaluation. This code below is an advanced version of 
  // that is going into the SageInterface.  It should be 
  // available there shortly and when in place it should be 
  // removed from here and referenced from the SageInterface 
  // namespace.
  // ********************************************************

  // Datastructure (going into sageInterface.h)
     struct const_numeric_expr_t 
        {
          bool hasValue_;
          bool isIntOnly_;

       // At the moment even integer valued expressions are returned via this 
       // double floating point value (this will be fixed in the final version 
       // for SageInterface).
          double value_;
        };

  // Function Declaration in SageInterface
     struct const_numeric_expr_t evaluateConstNumericExpression(SgExpression *expr);

  // Code that goes into sageInterface.C
     class SimpleExpressionEvaluator2: public AstBottomUpProcessing <struct DSL_Support::const_numeric_expr_t> 
        {
          public:
               struct DSL_Support::const_numeric_expr_t getValueExpressionValue(SgValueExp *valExp);
               struct DSL_Support::const_numeric_expr_t evaluateVariableReference(SgVarRefExp *vRef);
               struct DSL_Support::const_numeric_expr_t evaluateSynthesizedAttribute(SgNode *node, SynthesizedAttributesList synList);
        };

// end of namespace
   }
