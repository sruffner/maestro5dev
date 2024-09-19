//===================================================================================================================== 
//
// jsonvalue.h : Declaration of JSONValue class, providing simplified support for reading JSON-formatted content.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
// Adapted from:
//   JSONValue.h
//   Copyright (C) 2010 Mike Anchor <mikea@mjpa.co.uk>
//
//   Part of the MJPA JSON Library - http://mjpa.co.uk/blog/view/A-simple-C-JSON-library/
//
//   License: http://mjpa.co.uk/licenses/GPLv2/
//
//   This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public 
//   License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any 
//   later version.
//
//   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
//   details.
// 
//   You should have received a copy of the GNU General Public License along with this program; if not, write to the 
//   Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//===================================================================================================================== 


#if !defined(JSONVALUE_H__INCLUDED_)
#define JSONVALUE_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// Win32 incompatibilities
#ifdef WIN32
   static inline bool isnan(double x) { return x != x; }
   static inline bool isinf(double x) { return !isnan(x) && isnan(x - x); }
#endif

#include "afxtempl.h"                     // for CMap template 

class JSONTextSource
{
public:
   JSONTextSource(LPCTSTR filePath);
   ~JSONTextSource();
   
   char GetNextCharAndAdvance();
   char LookAtNextChar();
   bool GetFragment(int len, CString& str);
   bool SkipWhitespace();
   bool ExtractString(CString& str);
   bool ExtractNumber(double* pNum);
   
   long GetCharIndex();
   long GetLineIndex();
   const CString& GetTextUpToCurrChar(int len);
   bool IsDone();
   bool HasFailed();
   const CString& GetFailureReason();
   
private:
   bool GetNextBufferFromFileIfNecessary();
   
   /** Size of allocated buffer for reading in text from source file. */
   static int BUFSZ;
   /** Number of characters retained preceding the current chracter index. */
   static int LOOKBACKSZ;
   
   /** Full pathname to the text file that sources the JSON-encoded text. */
   CString m_filePath;
   /** The source file object. */
   CFile* m_pFile;
   
   /** Buffer for reading text from the source file. */
   char* m_pBuffer;
   /** Index of last valid character in the text buffer. -1 if invalid. */
   int m_last;
   /** Index of current character in the text buffer; -1 if invalid. */
   int m_curr;
   /** Last LOOKBACKSZ characters in the previous buffer's worth of text. */
   CString m_strEndOfPrevBuf;
   
   /** Used to prepare the lookback text for return as a function output.  Not thread-safe, of course. */
   CString m_lookback;
   
   /** Total number of 8-bit characters in the source file. */
   long m_totalLen;
   /** Current line index (determined by counting linefeed characters in the source text). */
   long m_lineIndex;
   /** Current character index in the file. */
   long m_charIndex;
   
   /** Description of why text source failed (usually an IO error); empty string otherwise. */
   CString m_failureReason;
};

enum JSONType { JSONType_Null, JSONType_String, JSONType_Bool, JSONType_Number, JSONType_Array, JSONType_Object };

class JSONValue;
typedef CArray<JSONValue*, JSONValue*> JSONArray;
typedef CMap<CString, LPCTSTR, JSONValue*, JSONValue*> JSONObject;

class JSONValue
{
public:
   static JSONValue* ParseComplete(JSONTextSource* pSrc, CString& errMsg);
   
   JSONValue(/*NULL*/);
   JSONValue(JSONType type);
   JSONValue(LPCTSTR str);
   JSONValue(bool b);
   JSONValue(double num);
   ~JSONValue();

   bool IsNull();
   bool IsString();
   bool IsBool();
   bool IsNumber();
   bool IsArray();
   bool IsObject();

   CString AsString();
   bool AsBool();
   double AsNumber();
   JSONArray* AsArray();
   JSONObject* AsObject();

private:
   void AddToArray(JSONValue* pValue);
   void AddToObject(LPCTSTR key, JSONValue* pValue);
   
   static JSONValue* Parse(JSONTextSource* pSrc, CString& errMsg);
   
   JSONType m_type;
   CString m_string;
   bool m_boolean;
   double m_number;
   JSONArray* m_pArray;
   JSONObject* m_pObject;
};

#endif
