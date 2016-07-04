#include "sage3basic.h"

#include "MatlabParser.h"
#include <cstdio>
#include <iostream>
#include <string>
#include "lex.h"

 void yyerror (char const *s) {
 	std::cerr << s << std::endl;
 }

extern int yy_flex_debug;
extern int beginParse(SgProject* &, int argc, char* argv[]);
extern "C" FILE *yyin;

extern int yydebug;
extern void prep_lexer_for_script_file ();

SgProject* MatlabParser::frontend(int argc, char* argv[])
  {
    std::string fileName = std::string(argv[1]);
    FILE *matlabFile = fopen(fileName.c_str(), "r");

    if (!matlabFile)
      {
	std::cout << "File Cannot be opened" << std::endl;
	ROSE_ASSERT(false);
	return NULL;
      }

    yydebug = 0;

    prep_lexer_for_script_file ();
    
    // set flex to read from it instead of defaulting to STDIN:
    yyin = matlabFile;

    SgProject* project = NULL;
    beginParse(project, argc, argv);

    fclose(matlabFile);
   
    ROSE_ASSERT(project != NULL);
    
    return project;
  }

