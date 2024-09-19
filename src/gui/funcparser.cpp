/*===================================================================================================================== 
funcparser.cpp : Implementation of CFunctionParser, a simple implementation of a function formula parser.

CFunctionParser is an implementation of a simple function parser that takes a function expression in string form and
converts it to a list of operand and operator tokens that represent the function in postfix order. It was developed
solely to provide a way to express a trial random variable as a function of other RVs, and its implementation is
narrowly limited to that purpose :
 -- Supported operands : Any numeric token, the number "PI" (must be capitalized) or a string token "X0" .. "X9",
representing the values of up to 10 different independent variables.Note that  the "X" must be a capital.
 -- Supported operators : Binary operators "-", "+", "*", "/"; unary negate operator "-" (distinguished from binary
   minus operator by context); grouping operators "(", ")", and ",".The comma operator is ONLY used to separate the
   arguments to a function operator; function operators "sin", "cos", and "pow".
 -- Operator precedence(from highest to lowest) : function operators; the negate operator; multiply and divide
   operators; add and subtract operators; grouping operators.Associativity is left to right.

USAGE:
 -- After construction, change the function definition with SetDefinition().
 -- Call IsValid() to see if the current function definition is valid IAW the rules of this function parser. If not
valid, GetParseErrorMessage() provides a brief error description.
 -- Use HasVariableX() to determine which independent variables "X0" .. "X9" participate in the function.
 -- Use Evaluate() to evaluate the function for any set of values of the independent variables.
=====================================================================================================================*/ 

#include "stdafx.h"
#include "stdlib.h"                          // CRT utilities for CFunctionParser
#include "ctype.h"                           // CRT utilities for CFunctionParser
#include "util.h"
#include "funcparser.h"


//===================================================================================================================== 
// Implementation of class CFunctionParser
//=====================================================================================================================

// tokens strings for all TOKENTYPEs except the first two, in order
LPCTSTR CFunctionParser::TOKENATOMS[] = {
      _T("0"),   // NUMERIC - not used because actual token varies
      _T("x0"),  // VARIABLE - not used becasue actual token varies
      _T("pi"),
      _T("-"), 
      _T("+"),
      _T("*"),
      _T("/"),
      _T("-"),
      _T("("),
      _T(")"),
      _T(","),
      _T("sin"),
      _T("cos"),
      _T("pow")
}; 

/**
 Construct a function parser for the specified function.
 @param s [in] The string representation of the function to be parsed and evaluated.
*/
CFunctionParser::CFunctionParser(const CString& s)
{
   m_strFunc = s;
   Parse();
}

/**
 Set the function string to be parsed and evaluated by this function parser.
 @param s [in] The function string. It may or may not be a valid function.
*/
VOID CFunctionParser::SetDefinition(const CString& s)
{
   m_strFunc = s;
   Parse();
}

/**
 Does the function currently represented by this function parser include the specified independent variable, "Xn", 
 where the integer argument specifies the variable index "n"?
 @param idx [in] The variable index. Must be in [0..9].
 @return TRUE if function is valid and contains a reference to the specified variable; else FALSE.
 */
BOOL CFunctionParser::HasVariableX(int idx)
{
   if(IsValid())
   {
      for(int i = 0; i<m_postFixFunc.GetSize(); i++)
      {
         Token* t = m_postFixFunc.GetAt(i);
         if(t->type == VARIABLE && t->varIdx == idx) return(TRUE);
      }
   }
   return(FALSE);
}

/** 
 Reset the function parser's postfix representation of the function definition. This should be called whenever parsing
 a new function definition, or just prior to destroying the parser object.
*/
VOID CFunctionParser::Reset()
{
   int i = 0;
   while(i < m_postFixFunc.GetSize()) { delete m_postFixFunc.GetAt(i++); }
   m_postFixFunc.RemoveAll();
}

/**
 Evaluate the function currently represented by this function parser.

 @param pdXVals [in] Array containing values of the independent variables at which function is evaluated: [X0..X9]. 
 While the current function may only use some of the variables, this array must always contain 10 elements.
 @param bOk [out] Set to TRUE if function was successfully evaluated. Returns FALSE if function is invalid, or if
 it does not evaluate to a real number for the set of variable values provided.
 @return If successful, the function evaluation for the set of variable values provided. 0 on failure.
*/
double CFunctionParser::Evaluate(double* pdXVals, BOOL& bOk)
{
   bOk = TRUE;
   if(!IsValid())
   {
      bOk = FALSE;
      return(0.0);
   }

   // operand stack holds intermediate results as we proceed through the function in postfix order
   CList<double, double> operands(10);

   // evaluate function: Operand values are pushed onto the stack in the order encountered. When an operator is met, 
   // the required number of operand values are popped from the stack and supplied to the operator, and then the
   // result is pushed back onto the stack. At the end of this process, the function result should be the last value
   // on the operand stack. If an operation cannot be performed (divide by zero, e.g.), then we stop immediately and
   // clear the output flag indicating that the function could not be evaluated. Note that the postfix array 
   // representation of the function is not altered, so it can be reused each time this method is invoked.
   for(int i=0; bOk && i<m_postFixFunc.GetSize(); i++)
   {
      Token* t = m_postFixFunc.GetAt(i);
      if(t->type == NUMERIC || t->type == PI)
         operands.AddHead(t->value);
      else if(t->type == VARIABLE)
         operands.AddHead(pdXVals[t->varIdx]);
      else if(t->type == NEGATE || t->type == SIN || t->type == COS)
      {
         double arg = operands.RemoveHead();
         if(t->type == NEGATE) arg = -arg;
         else if(t->type == SIN) arg = ::sin(arg);
         else arg = ::cos(arg);
         operands.AddHead(arg);
      }
      else
      {
         double arg0 = operands.RemoveHead();
         double arg1 = operands.RemoveHead();
         double val = 0;
         if(t->type == MINUS) val = arg1 - arg0;
         else if(t->type == PLUS) val = arg1 + arg0;
         else if(t->type == TIMES) val = arg1 * arg0;
         else if(t->type == DIVIDE)
         {
            if(arg0 == 0.0) bOk = FALSE;
            else val = arg1 / arg0;
         }
         else if(t->type == POW) val = ::pow(arg1, arg0);

         if(bOk) operands.AddHead(val);
      }
   }

   if(bOk) bOk = BOOL(operands.GetSize() == 1); 
   return(bOk ? operands.RemoveHead() : 0);
}

/**
 Parse the current function definition string into operand and operator tokens, then store these tokens in postfix 
 order for function evaluation. If an error occurs while parsing, the postfix token array is cleared and an error
 description is set; in this case, IsValid() will return FALSE until a new, parsable function definition is set.
*/
VOID CFunctionParser::Parse()
{
   Reset();
   m_parseErrorMsg = _T("");

   // PHASE 1: Break up string into operator and operator tokens. Validate syntax -- abort if function string invalid.

   // special case: empty function string
   if(m_strFunc.IsEmpty())
   {
      m_parseErrorMsg = _T("Function string is empty");
      return;
   }

   // check for an unmatched parenthesis
   int iUnmatched = FindUnmatchedParen(m_strFunc);
   if(iUnmatched >= 0)
   {
      m_parseErrorMsg.Format(_T("Unmatched parenthesis at index %d"), iUnmatched);
      return;
   }

   // this buffer is used to convert numeric string tokens to a number. We expect numeric operands to have far fewer
   // than 50 characters!
   char strNum[50];

   CTokenArray tokens;

   int iLen = m_strFunc.GetLength();
   int iPos = 0;
   BOOL bOk = TRUE;
   while(bOk && iPos < iLen)
   {
      // find next character that is not whitespace
      while(iPos < iLen && ::isspace(m_strFunc.GetAt(iPos))) ++iPos;
      if(iPos >= iLen) break;

      // remember start of new token, for error report
      int iTokenStart = iPos;
      Token* pToken = NULL;

      // identify and consume the new token
      char nextChar = (char) m_strFunc.GetAt(iPos);
      if(nextChar == '.' || ::isdigit(nextChar))
      {
         // if first character of token is a decimal point or digit, then consume token as a numeric operand. The 
         // floating-point number can contain at most one '.' character. Scientific notation NOT supported.
         BOOL bGotDecimalPt = BOOL(nextChar == '.');
         strNum[0] = nextChar;
         int idx = 1;
         while(++iPos < iLen && bOk)
         {
            nextChar = (char) m_strFunc.GetAt(iPos);
            if(!(nextChar == '.' || ::isdigit(nextChar)))
               break;

            if(nextChar == '.')
            {
               if(bGotDecimalPt)
               {
                  m_parseErrorMsg.Format(_T("Numeric operand at index %d has multiple decimal points"), iTokenStart);
                  bOk = FALSE;
                  break;
               }
               bGotDecimalPt = TRUE;
            }

            if(idx == 49)  // need to leave room for terminating null char
            {
               m_parseErrorMsg.Format(_T("Numeric operand at index %d is too long"), iTokenStart);
               bOk = FALSE;
            }
            else strNum[idx++] = nextChar;
         }

         // the numeric token could be just the decimal point, which is obviously invalid
         if(bOk && (idx==1) && bGotDecimalPt)
         {
            m_parseErrorMsg.Format(_T("Bad numeric operand at index %d"), iTokenStart);
            bOk = FALSE;
         }

         // create the numeric operand token 
         if(bOk)
         {
            strNum[idx++] = '\0';

            pToken = new Token;
            pToken->type = NUMERIC;
            pToken->value = ::atof(&(strNum[0]));
            pToken->varIdx = -1;
         }
      }
      else if(nextChar == 'x')
      {
         // if first token character is 'x', then consume as a variable token: 'x0'.. 'x9'. If the second
         // character does not exist or is not a digit, then parsing fails
         ++iPos;
         bOk = (iPos < iLen) && ::isdigit(m_strFunc.GetAt(iPos));
         if(bOk)
         {
            strNum[0] = (char) m_strFunc.GetAt(iPos);
            strNum[1] = '\0';
            pToken = new Token;
            pToken->type = VARIABLE;
            pToken->value = 0;
            pToken->varIdx = ::atoi(&(strNum[0]));
            ++iPos;
         }
         else
         {
            m_parseErrorMsg.Format(_T("Invalid variable operand at index %d"), iTokenStart);
            bOk = FALSE;
         }
      }
      else
      {
         // otherwise, we must match one of the other token types
         // IMPORTANT: Because MINUS operator comes before NEGATE, all '-' tokens are mapped to MINUS. The token
         // type is corrected later, based on context.
         for(int i=PI; i<NUMTOKENTYPES; i++) if(iPos == m_strFunc.Find(TOKENATOMS[i], iPos))
         {
            pToken = new Token;
            pToken->type = (TokenType) i;
            pToken->value = (i==PI) ? cMath::PI : 0.0;
            pToken->varIdx = 0;
            iPos += static_cast<int>(::strlen((const char *) TOKENATOMS[i]));
            break;
         }
         if(pToken == NULL)
         {
            m_parseErrorMsg.Format(_T("Unrecognized token at index %d"), iTokenStart);
            bOk = FALSE;
         }
      }

      // if next token identified, validate it based on the identity of the previous token
      if(bOk)
      {
         Token* pPrev = (tokens.GetSize() == 0) ? NULL : tokens.GetAt(tokens.GetSize()-1);
         if(IsOperand(pToken) || IsFuncOperator(pToken))
         {
            bOk = (pPrev==NULL) || pPrev->type==LEFTPAREN || pPrev->type==NEGATE || pPrev->type==COMMA || 
               IsBinaryOperator(pPrev);
         }
         else if(IsBinaryOperator(pToken))
         {
            bOk = (pPrev != NULL) && (IsOperand(pPrev) || pPrev->type==RIGHTPAREN);

            // if not valid but the operator token is "-", then replace with unary negate operator if valid here
            if((!bOk) && pToken->type == MINUS)
            {
               bOk = (pPrev == NULL) || IsBinaryOperator(pPrev) || pPrev->type==LEFTPAREN || pPrev->type==COMMA;
               if(bOk) pToken->type = NEGATE;
            }
         }
         else if(pToken->type == LEFTPAREN)
         {
            bOk = (pPrev == NULL) || IsBinaryOperator(pPrev) || IsFuncOperator(pPrev) || pPrev->type==LEFTPAREN || 
               pPrev->type==NEGATE || pPrev->type==COMMA;
         }
         else if(pToken->type == RIGHTPAREN)
         {
            bOk = (pPrev != NULL) && (IsOperand(pPrev) || pPrev->type==RIGHTPAREN);
            if(bOk && !CheckFunctionArgCount(tokens))
            {
               bOk = FALSE;
               m_parseErrorMsg.Format(_T("Function op has wrong number of args (near index %d)"), iTokenStart);
            }
         }
         else if(pToken->type == COMMA)
         {
            bOk = (pPrev != NULL) && (IsOperand(pPrev) || pPrev->type==RIGHTPAREN);
            if(bOk && !IsInsideFunction(tokens))
            {
               bOk = FALSE;
               m_parseErrorMsg.Format(_T("Invalid comma at index %d"), iTokenStart);
            }
         }

         if((!bOk) && m_parseErrorMsg.IsEmpty()) 
            m_parseErrorMsg.Format(_T("Token not allowed at index %d"), iTokenStart);
      }

      // if new token has been validated, add it to the token array
      if(bOk) tokens.Add(pToken);
      else if(pToken != NULL) delete pToken;
   }

   // there must be at least one token, and last token must be an operand or right parenthesis
   if(bOk)
   {
      bOk = (tokens.GetSize() > 0);
      if(!bOk) m_parseErrorMsg = _T("No valid tokens found");
      else
      {
         Token* pLast = tokens.GetAt(tokens.GetSize()-1);
         if(!(IsOperand(pLast) || pLast->type == RIGHTPAREN))
         {
            bOk = FALSE;
            m_parseErrorMsg = _T("Must end with an operand or a ')'");
         }
      }
   }

   // if parsing failed, delete any tokens we had created and abort
   if(!bOk)
   {
      for(int i=0; i<tokens.GetSize(); i++) { delete tokens.GetAt(i); }
      tokens.RemoveAll();
      return;
   }

   // PHASE 2: Put tokens in postfix order, eliminating all grouping operators (parens and commas)
   CTypedPtrList<CPtrList, Token*> opStack;
   for(int i=0; i<tokens.GetSize(); i++)
   {
      Token* pToken = tokens.GetAt(i);
      if(IsOperand(pToken))
      {
         // all operands are added to the postfix token list in the order encountered
         m_postFixFunc.Add(pToken);
      }
      else if(pToken->type == LEFTPAREN)
      {
         // a left parenthesis is pushed onto operator stack when encountered. It is removed upon processing the 
         // matching right parenthesis
         opStack.AddHead(pToken);
      }
      else if(pToken->type == RIGHTPAREN || pToken->type == COMMA)
      {
         // the comma and right parenthesis are processed similarly. We pop items off the stack and add them to the
         // postfix token list until the first left parenthesis is found. If we're processing a comma, the left
         // parenthesis is left on the stack -- else the later right parenthesis will not have a match. Observe
         // that none of the grouping operators is added to the postfix representation of the function!
         BOOL foundLeft = FALSE;
         while(!(foundLeft || opStack.IsEmpty()))
         {
            Token* pPop = opStack.RemoveHead();
            if(pPop->type == LEFTPAREN)
            {
               foundLeft = TRUE;
               if(pToken->type == COMMA) opStack.AddHead(pPop);
            }
            else
               m_postFixFunc.Add(pPop);
         }
         ASSERT(foundLeft);  // we should ALWAYS find the left parenthesis!
      }
      else
      {
         // for all non-grouping operators, we need to pop any operators with precedence greater than or equal to the
         // current operator, then push the current operator onto the stack. Stop as soon as we encounter an operator
         // with a lesser precedence
         int currPrec = GetPrecedence(pToken);
         while(!opStack.IsEmpty())
         {
            Token* pHead = opStack.GetHead();
            if(GetPrecedence(pHead) >= currPrec) 
               m_postFixFunc.Add(opStack.RemoveHead());
            else
               break;
         }
         opStack.AddHead(pToken);
      }
   }

   // if there any operators left on the stack, pop them off and add them to the postfix list
   while(!opStack.IsEmpty()) m_postFixFunc.Add(opStack.RemoveHead());

   // clean up: Remember that no grouping operators are included in the final postfix list. So we need to delete
   // those tokens or we create a memory leak! Don't delete any other operators!
   for(int i=0; i<tokens.GetSize(); i++)
   {
      Token* pToken = tokens.GetAt(i);
      if(IsGroupingOperator(pToken)) { delete pToken; }
   }
   tokens.RemoveAll();
}



/**
 Check specified string for an unmatched left "(" or right ")" parenthesis. 
 @param s the function string to check.
 @returns -1 if not unmatched parenthesis found.
*/
int CFunctionParser::FindUnmatchedParen(const CString& s)
{
   int iPos = 0;
   int iLen = s.GetLength();
   CArray<int,int> unmatchedLefts;

   while(iPos < iLen)
   {
      int nextLeftParen = s.Find('(', iPos);
      if(nextLeftParen == -1) nextLeftParen = iLen;
      int nextRightParen = s.Find(')', iPos);
      if(nextRightParen == -1) nextRightParen = iLen + 1;

      if(nextRightParen < nextLeftParen)
      {
         if(unmatchedLefts.GetSize() == 0) return(nextRightParen);   // unmatched right parenthesis!
         unmatchedLefts.RemoveAt(unmatchedLefts.GetSize()-1);
         iPos = nextRightParen + 1;
      }
      else if(nextLeftParen < iLen)
      {
         unmatchedLefts.Add(nextLeftParen);
         iPos = nextLeftParen + 1;
      }
      else iPos = iLen;   // no parentheses left
   }

   // if there are any unmatched left parentheses, return index of the first such one
   return(unmatchedLefts.GetSize()==0 ? -1 : unmatchedLefts.GetAt(0));
}

/**
 If the next token in an incompletely parsed function definition is a ")", use this method to validate the number of
 function arguments WHEN the ")" and its matching "(" delimit the argument list of a function operator. The method
 searches backward through the provided token list for the first unmatched "(", incrementing the arg count (starting
 at 1) each time a comma operator is encountered at zero depth. Then, IF the token preceding that left parenthesis is
 a function operator, the method checks the observed argument count against that function's required # of arguments.
 @param tokens [in] List of operand/operator tokens representing an incompletely parsed function definition; the list
 is not modified by this method.
 @return FALSE if the observed function argument count is incorrect. Returns TRUE if the argument count is correct, if
 the operator preceding the left parenthesis is not a function operator, if that left parenthesis was not found.
*/
BOOL CFunctionParser::CheckFunctionArgCount(const CTokenArray& tokens)
{
   int depth = 0;
   int nArgs = 0;
   INT_PTR pos = tokens.GetSize() - 1;
   while(pos >= 0)
   {
      Token* pToken = tokens.GetAt(pos);
      if(pToken->type == RIGHTPAREN) --depth;
      else if(pToken->type == COMMA && depth == 0) ++nArgs;
      else if(pToken->type == LEFTPAREN)
      {
         if(depth == 0)
         {
            // found unmatched left parenthesis. Move to token preceding it, which might be a function operator
            --pos;
            break;
         }
         else ++depth;
      }
      else if(nArgs == 0) nArgs = 1;

      --pos;
   }


   // found the unmatched left parenthesis. Now check operator before it. Return TRUE if it's not a function 
   // operator, or if it is AND the argument count is correct. 
   if(pos >= 0)
   {
      Token* pToken = tokens.GetAt(pos);
      return((!IsFuncOperator(pToken)) || (pToken->type==POW && nArgs == 2) ||
         (nArgs==1 && (pToken->type==SIN || pToken->type==COS)));
   }

   // did not find an unmatched left parenthesis (this should never happen, BTW)
   return(TRUE);
}

/**
 Use this method to test whether or not a list of parsed tokens represents a state such that the next token appended to
 the list will be inside the parentheses that bracket the argument list of a function operator. The method searches
 backward through the provided token list for the first unmatched left parenthesis and checks whether or not the token
 preceding that parenthesis is a function operator.
 @param tokens [in] List of operand/operator tokens representing an incompletely parsed function definition; the list
 is not modified by this method.
 @return TRUE if the next token appended to the token list would be inside the argument list of a function operator.
*/
BOOL CFunctionParser::IsInsideFunction(const CTokenArray& tokens)
{
   int depth = 0;
   INT_PTR pos = tokens.GetSize() - 1;
   while(pos >= 0)
   {
      Token* pToken = tokens.GetAt(pos);
      if(pToken->type == RIGHTPAREN) --depth;
      else if(pToken->type == LEFTPAREN)
      {
         if(depth == 0)
         {
            // found unmatched left parenthesis. Move to token preceding it, which is what we need to check.
            --pos;
            break;
         }
         else ++depth;
      }

      --pos;
   }
   return((pos >= 0) && IsFuncOperator(tokens.GetAt(pos)));
}

/**
 Get the precedence assigned to the specified operator token: 0 for grouping operators; 1 for the add/subtract 
 operators; 2 for the multiply/divide operators; 4 for the negate operator; and 5 for any function operator.

 @param pToken [in] The token.
 @return The operator's precedence as listed above. If the argument is NULL or represents an operand rather than an
 operator, method returns 0.
*/
int CFunctionParser::GetPrecedence(Token* t)
{
   int prec = 0;
   if(t==NULL || IsOperand(t) || IsGroupingOperator(t)) prec = 0;
   else if(t->type==MINUS || t->type==PLUS) prec = 1;
   else if(t->type==TIMES || t->type==DIVIDE) prec = 2;
   else if(t->type==NEGATE) prec = 4;
   else if(IsFuncOperator(t)) prec = 5;

   return(prec);
}
