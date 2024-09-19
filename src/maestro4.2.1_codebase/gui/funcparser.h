//===================================================================================================================== 
// funcparser.h : Declaration of CFunctionParser, a simple implementation of a function formula parser.
//===================================================================================================================== 

#if !defined(FUNCPARSER_H__INCLUDED_)
#define FUNCPARSER_H__INCLUDED_

#include "stdafx.h"
#include <windows.h>
#include "math.h"


class CFunctionParser
{
private:
   typedef enum {
      NUMERIC = 0,         // a numeric operand
      VARIABLE,            // a variable operand: "X0" .. "X9"
      PI,                  // the "PI" constant, a numeric operand
      MINUS,               // the supported operators...
      PLUS,
      TIMES,
      DIVIDE,
      NEGATE,
      LEFTPAREN,
      RIGHTPAREN,
      COMMA,
      SIN,
      COS,
      POW
   } TokenType;
   static const int NUMTOKENTYPES = 14;

   // tokens strings for all token types; the first two are not used because actual token varies!
   static LPCTSTR TOKENATOMS[NUMTOKENTYPES]; 

   // an operand or operator token in the postfix representation of the function
   struct Token {
      TokenType type;       // the token type
      double value;         // for a numeric operand, this is its value
      int varIdx;           // for a variable operand, this is its index in [0..9]
   };

   CString m_strFunc;
   CString m_parseErrorMsg;

   typedef CTypedPtrArray<CPtrArray, Token*> CTokenArray;

   CTokenArray m_postFixFunc;

   // no copy constructor or assignment operator defined
   CFunctionParser(const CFunctionParser& src); 
   CFunctionParser& operator=(const CFunctionParser& src);

public :
   CFunctionParser(const CString& s);
   ~CFunctionParser() { Reset(); }

   VOID SetDefinition(const CString& s);
   LPCTSTR GetDefinition() { return((LPCTSTR)m_strFunc); }
   BOOL IsValid() const { return(m_parseErrorMsg.IsEmpty()); }
   VOID GetParseErrorMessage(CString& errMsg) const {  errMsg = m_parseErrorMsg; }
   BOOL HasVariableX(int idx);
   double Evaluate(double* pdXVals, BOOL& bOk);

private:
   VOID Reset();
   VOID Parse();
   static int FindUnmatchedParen(const CString& s);
   static BOOL CheckFunctionArgCount(const CTokenArray& tokens);
   static BOOL IsInsideFunction(const CTokenArray& tokens);
   static int GetPrecedence(Token* t);
   static BOOL IsOperand(Token* t) 
   { 
      return((t!=NULL) && (t->type==NUMERIC || t->type==PI || t->type==VARIABLE)); 
   }
   static BOOL IsFuncOperator(Token* t)
   { 
      return((t!=NULL) && (t->type==SIN || t->type==COS || t->type==POW)); 
   }
   static BOOL IsBinaryOperator(Token *t) 
   {
      return((t!=NULL) && (t->type==MINUS || t->type==PLUS || t->type==TIMES || t->type==DIVIDE));  
   }
   static BOOL IsGroupingOperator(Token *t)
   {
      return((t!=NULL) && (t->type==LEFTPAREN || t->type==RIGHTPAREN || t->type==COMMA)); 
   }
};



#endif   // !defined(FUNCPARSER_H__INCLUDED_)
