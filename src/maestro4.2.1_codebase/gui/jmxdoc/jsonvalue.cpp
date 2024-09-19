//=====================================================================================================================
//
// jsonvalue.cpp : Implementation ofJSONValue class, providing simplified support for reading JSON-formatted content.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// JSONValue only supports reading JSON-formatted content that is US-ASCII and does not include any escaped Unicode
// character sequences in any string values within the JSON content.
//
// CREDITS: Adapted from....
// 
//   JSONValue.cpp, JSON.cpp
//   Copyright (C) 2010 Mike Anchor <mikea@mjpa.co.uk>
// 
//   Part of the MJPA JSON Library - http://mjpa.co.uk/blog/view/A-simple-C-JSON-library/
//
//   License: http://mjpa.co.uk/licenses/GPLv2/
// 
//   This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public 
//   License as published by  the Free Software Foundation; either version 2 of the License, or (at your option) any 
//   later version.
//
//   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more 
//   details.
//
//   You should have received a copy of the GNU General Public License along with this program; if not, write to the 
//   Free Software  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//
// REVISION HISTORY:
// 15apr2010-- Began development.
// 05sep2018-- Fixed bug in JSONTextSource::ExtractNumber() -- did not properly handle parsing a number like "14.0036",
// which would be converted to 14.36 instead.
//=====================================================================================================================

#include <stdafx.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "jsonvalue.h"


/* JSONTextSource implementation ==================================================================================== */

int JSONTextSource::BUFSZ = 16384;
int JSONTextSource::LOOKBACKSZ = 80;

/**
 * Construct a JSON text source that reads JSON-encoded content from the file specified. The file is not opened until
 * the first character is requested from the source.
 * @param filePath Full pathname to the source file.
 */
JSONTextSource::JSONTextSource(LPCTSTR filePath)
{
   m_filePath = filePath;
   m_pFile = NULL;
   
   m_pBuffer = new char[BUFSZ];
   
   m_last = -1;
   m_curr = 0;
   m_strEndOfPrevBuf = "";
   
   m_totalLen = 0L;
   m_lineIndex = 0L;
   m_charIndex = -1L;
   
   m_failureReason = (m_pBuffer == NULL) ? "Unable to allocate memory buffer for reading source file!" : "";
}

/** Destroy this JSON text source object. The read buffer is deleted and the source file is closed if necessary. */ 
JSONTextSource::~JSONTextSource()
{
   try { if(m_pFile != NULL && m_pFile->m_hFile != CFile::hFileNull) m_pFile->Close(); }
   catch(CFileException* e) { e->Delete(); }
   
   if(m_pFile != NULL)
   {
      delete m_pFile;
      m_pFile = NULL;
   }
   
   if(m_pBuffer != NULL)
   {
      delete[] m_pBuffer;
      m_pBuffer = NULL;
   }
}

/**
 * Get the character at the current index in this JSON-formatted text content source, then increment the index.
 * @return The next character. Returns 0 if source has already been consumed or if an IO error has disabled it.
 */
char JSONTextSource::GetNextCharAndAdvance()
{
   if((m_charIndex >= m_totalLen) || (m_failureReason.GetLength() > 0)) return(0);
   if(!GetNextBufferFromFileIfNecessary()) return(0);
   
   // get current character and advance indices. Increment line index if it's a linefeed!
   char c = m_pBuffer[m_curr];
   if(c == '\n') ++m_lineIndex;
   ++m_curr;
   ++m_charIndex;
   
   return(c);
}

/**
 * Get the character at the current index in this JSON-formatted text content source, but DO NOT increment the index.
 * @return The next character. Returns 0 if source has already been consumed or if an IO error has disabled it.
 */
char JSONTextSource::LookAtNextChar()
{
   if((m_charIndex >= m_totalLen) || (m_failureReason.GetLength() > 0)) return(0);
   if(!GetNextBufferFromFileIfNecessary()) return(0);
   
   return(m_pBuffer[m_curr]);
}

/**
 * Retrieve a text fragment from this JSON-formatted text content source, advancing the index to the character 
 * immediately after the fragment retrieved.
 * @param len Length of fragment to be retrieved.
 * @param str The text fragment is stored here, if operation is successful.
 * @return True if successful, false if number of characters remaining in source is less than length requested. Also
 * returns false if the source is disabled by an IO error.
 */
bool JSONTextSource::GetFragment(int len, CString& str)
{
   if((m_charIndex + len > m_totalLen) || (m_failureReason.GetLength() > 0)) return(false);
   
   str = "";
   
   long end = m_charIndex + len;
   while(m_charIndex < end)
   {
      if(!GetNextBufferFromFileIfNecessary()) return(false);
      
      int avail = m_last - m_curr + 1;
      if(end-m_charIndex < (long) avail) avail = (int) end-m_charIndex;
      
      int fragLen = str.GetLength();
      LPTSTR pFrag = str.GetBuffer(fragLen + avail);
      for(int i=0; i<avail; i++) 
      {
         char c = m_pBuffer[m_curr+i];
         pFrag[fragLen + i] = c;
         if(c == '\n') ++m_lineIndex;
      }
      str.ReleaseBuffer(fragLen + avail);
      
      m_curr += avail;
      m_charIndex += avail;
   }

   return(true);
}

/** 
 * Skips over any whitespace characters (' ', '\t', '\r' or '\n') starting at the current character index in this
 * JSON-formatted text content source.
 * @return True if text content remains; false if the end of source was reached or an IO error disabled the source.
 */
bool JSONTextSource::SkipWhitespace()
{
   if((m_charIndex >= m_totalLen) || (m_failureReason.GetLength() > 0)) return(0);
   
   while(m_charIndex < m_totalLen)
   {
      if(!GetNextBufferFromFileIfNecessary()) return(false);
      
      bool gotWhitespace = true;
      while(gotWhitespace && m_curr <= m_last)
      {
         char c = m_pBuffer[m_curr];
         if(c == ' ' || c == '\t' || c == '\r' || c == '\n')
         {
            if(c == '\n') ++m_lineIndex;
            ++m_curr;
            ++m_charIndex;
         }
         else
            gotWhitespace = false;
      }
      
      if(!gotWhitespace) break;
   }
   
   return(m_charIndex < m_totalLen);
}

/** 
 * Extracts a string value starting at the current character index in this JSON text source. The current character MUST
 * be the double-quote (") that marks the start of a JSON string value. The string will include all characters up to
 * but excluding the next double-quote encountered. Any escaped characters are swapped out for their unescaped values.
 * Afterwards, the character index will point to the character immediately after the closing double-quote.
 *
 * NOTE that this method supports a small subset of the specification for a JSON string value. All characters must be 
 * US-ASCII 0x20-0x7e, or one of the allowed escape sequences EXCEPT for '/uHHHH'. Unicode characers are NOT supported.
 *
 * @param str Reference to a CString to receive the extracted string value.
 * @return True if successful. Returns false if: (1) the token starting at the current character index is not a valid 
 * JSON string containing only US-ASCII 0x20-0x7e or an allowed escape sequence, (2) if the end of source was reached 
 * before encountering the closing double-quote, or (3) if source was disabled by an IO error.
 */
bool JSONTextSource::ExtractString(CString& str)
{
   if((m_charIndex >= m_totalLen) || (m_failureReason.GetLength() > 0)) return(false);
   if(!GetNextBufferFromFileIfNecessary()) return(false);
   
   str = "";
	
   if(m_pBuffer[m_curr] != '"') return(false);
   ++m_curr;
   ++m_charIndex;
   
   while(m_charIndex < m_totalLen)
   {
      if(m_curr > m_last)
      {
         if(!GetNextBufferFromFileIfNecessary()) return(false);
      }
      
      // save the char so we can change it if need be
      char nextChar = m_pBuffer[m_curr];
      ++m_curr;
      ++m_charIndex;
      
      // escaped character?
      if(nextChar == '\\')
      {
         // if nothing follows the backslash, then text content is invalid.
         if(m_charIndex == m_totalLen) return(false);

         // it's possible that the '\' is the last character in the current read buffer!
         if(m_curr > m_last)
         {
            if(!GetNextBufferFromFileIfNecessary()) return(false);
         }
         
         // deal with the escaped char
         char escapedChar = m_pBuffer[m_curr];
         ++m_curr;
         ++m_charIndex;

         switch(escapedChar)
         {
            case '"': nextChar = '"'; break;
            case '\\': nextChar = '\\'; break;
            case '/': nextChar = '/'; break;
            case 'b': nextChar = '\b'; break;
            case 'f': nextChar = '\f'; break;
            case 'n': nextChar = '\n'; break;
            case 'r': nextChar = '\r'; break;
            case 't': nextChar = '\t'; break;
            case 'u':
            default:
               // Unicode characters not supported in this implementation
               return(false);
         }
      }

      // end of the string?
      else if(nextChar == '"')
      {
         return(true);
      }

      // disallowed char?
      else if ((nextChar < ' ' || nextChar > 0x7e) && nextChar != '\t')
      {
         // SPEC Violation: Allow tabs due to real world cases
         return(false);
      }
      
      // add the next char
      str += nextChar;
   }

   // text content ended before encountering the closing double-quote!
   return(false);
}

/**
 * Extracts a number token starting at the current character index in this JSON text source. The current character must
 * be a negative sign ('-') or an ASCII digit ('0' to '9'); else the method fails. If successful, the character index is
 * moved to the character immediately after the last character comprising the number token.
 *
 * @param pNum Pointer to a double to receive the numerical value parsed from the text source.
 * @return True if successful.  Returns false if: (1) the JSON number token is invalid, (2) if the end of source was 
 * reached before parsing a valid number token, or (3) if source was disabled by an IO error.
 */
bool JSONTextSource::ExtractNumber(double* pNum)
{
   if((m_charIndex >= m_totalLen) || (m_failureReason.GetLength() > 0)) return(false);
   if(!GetNextBufferFromFileIfNecessary()) return(false);
   
   char nextChar = m_pBuffer[m_curr];
   bool neg = (nextChar == '-');
   if(!(neg || (nextChar >= '0' && nextChar <= '9'))) return(false);

   // move past negative sign
   if(neg) 
   {
      ++m_curr;
      ++m_charIndex;
      if(m_charIndex >= m_totalLen) return(false);
      if(!GetNextBufferFromFileIfNecessary()) return(false);
      nextChar = m_pBuffer[m_curr];
   }

   // parse the whole part of the number. If it starts with '0', the next character cannot be another digit!
   double theNumber = 0.0;
   if(nextChar == '0')
   {
      ++m_curr;
      ++m_charIndex;
      if(m_charIndex >= m_totalLen) 
         nextChar = 0;
      else
      {
         if(!GetNextBufferFromFileIfNecessary()) return(false);
         nextChar = m_pBuffer[m_curr];
      }
      
      if(nextChar >= '0' && nextChar <= '9') return(false);
   }
   else if(nextChar >= '1' && nextChar <= '9')
   {
      int wholePart = 0;
      while(nextChar >= '0' && nextChar <= '9')
      {
         wholePart = wholePart * 10 + ((int) (nextChar - '0'));
         ++m_curr;
         ++m_charIndex;
         if(m_charIndex >= m_totalLen) 
            nextChar = 0;
         else
         {
            if(!GetNextBufferFromFileIfNecessary()) return(false);
            nextChar = m_pBuffer[m_curr];
         }
      }
      theNumber = (double) wholePart;
   }
   else
      return(false);
   
   // could be a decimal now...
   if(nextChar == '.')
   {
      // must be at least one digit after the decimal place
      ++m_curr;
      ++m_charIndex;
      if(m_charIndex >= m_totalLen) 
         nextChar = 0;
      else
      {
         if(!GetNextBufferFromFileIfNecessary()) return(false);
         nextChar = m_pBuffer[m_curr];
      }
      if(!(nextChar >= '0' && nextChar <= '9')) return(false);

      // extract decimal part as an integer, then convert
      int decPart = 0;
      int nPlaces = 0;
      while(nextChar >= '0' && nextChar <= '9')
      {
         decPart = decPart * 10 + ((int) (nextChar - '0'));
         ++m_curr;
         ++m_charIndex;
         if(m_charIndex >= m_totalLen) 
            nextChar = 0;
         else
         {
            if(!GetNextBufferFromFileIfNecessary()) return(false);
            nextChar = m_pBuffer[m_curr];
         }
         ++nPlaces;
      }

      // BUG FIX: This code did not correctly parse a number with one or more zeros after the decimal point.
      // E.g., "14.036" was converted to 14.36!
      // double decimal = (double) decPart;
      // while(((int)decimal) > 0) decimal /= 10.0;
      double decimal = (double) decPart;
      while(nPlaces > 0) { decimal /= 10.0; --nPlaces; }

      // add decimal part to whole part
      theNumber += decimal;
   }

   // could be an exponent now...
   if(nextChar == 'E' || nextChar == 'e')
   {
      // move on to next character. There must be at least one digit, but it could be preceded by a '+' or '-'
      ++m_curr;
      ++m_charIndex;
      if(m_charIndex >= m_totalLen) 
         return(false);
      else
      {
         if(!GetNextBufferFromFileIfNecessary()) return(false);
         nextChar = m_pBuffer[m_curr];
      }

      // if sign of exponent present, get it and move on
      bool neg_expo = false;
      if(nextChar == '-' || nextChar == '+')
      {
         neg_expo = (nextChar == '-');
         ++m_curr;
         ++m_charIndex;
         if(m_charIndex >= m_totalLen) 
            return(false);
         else
         {
            if(!GetNextBufferFromFileIfNecessary()) return(false);
            nextChar = m_pBuffer[m_curr];
         }
      }

      // must be at least one digit!
      if(!(nextChar >= '0' && nextChar <= '9')) return(false);

      // extract the exponent (an integer), then raise number to the power indicated (base 10)
      int expo = 0;
      while(nextChar >= '0' && nextChar <= '9')
      {
         expo = expo * 10 + ((int) (nextChar - '0'));
         ++m_curr;
         ++m_charIndex;
         if(m_charIndex >= m_totalLen) 
            nextChar = 0;
         else
         {
            if(!GetNextBufferFromFileIfNecessary()) return(false);
            nextChar = m_pBuffer[m_curr];
         }
      }

      for(int i = 0; i < expo; i++)
         theNumber = neg_expo ? (theNumber / 10.0) : (theNumber * 10.0);
   }

   // success; account for negative sign
   *pNum = neg ? -theNumber : theNumber;
   return(true);
}

/**
 * Get the (zero-based) index of the current character in this JSON-formatted text source.
 * @return The current character index.
 */
long JSONTextSource::GetCharIndex() { return(m_charIndex); }

/**
 * Get the (zero-based) index of the current line in this JSON-formatted text source. Each time a linefeed character
 * ('\n') is encountered in the source text, the line index is incremented. If the source contains no whitespace, this 
 * method will return 0 always.
 * @return The current line index.
 */
long JSONTextSource::GetLineIndex() { return(m_lineIndex); }

/**
 * Get the text fragment up to and including the current character in this JSON-formatted text source.
 * @param len Length of fragment to be retrieved. Range-restricted to [1..80].
 * @return The (read-only) text fragment. It may be shorter than the length requested. An empty string is returned if an 
 * IO error has disabled the source, or if no characters have been read from the source thus far.
 */
const CString& JSONTextSource::GetTextUpToCurrChar(int len)
{
   m_lookback = "";
   if(m_charIndex <= 0 || (m_failureReason.GetLength() > 0)) return(m_lookback);
   
   len = (len < 1) ? 1 : (len > LOOKBACKSZ ? LOOKBACKSZ : len);
   
   int end = (m_curr > m_last) ? m_last : m_curr;
   int start = end - len + 1;
   if(start < 0) start = 0;
   
   int lenFromPrevBuf = 0;
   if(end - start + 1 < len) lenFromPrevBuf = len - (end-start+1);
   
   if(lenFromPrevBuf > 0 && m_strEndOfPrevBuf.GetLength() > 0)
   {
      if(lenFromPrevBuf > m_strEndOfPrevBuf.GetLength()) m_lookback = m_strEndOfPrevBuf;
      else m_lookback = m_strEndOfPrevBuf.Right(lenFromPrevBuf);
   }
   
   for(int i=start; i<=end; i++) m_lookback += m_pBuffer[i];
   return(m_lookback);
}

/**
 * Has this JSON text source been completely consumed?
 * @return True iff source has been successfully opened and all of its content read and consumed.
 */
bool JSONTextSource::IsDone() { return(m_charIndex >= m_totalLen); }

/**
 * Has this JSON text source failed because of an IO error? If so, call GetFailureReason() for an error description.
 * @return True if source has been disabled by a previous IO error; false otherwise. 
 */
bool JSONTextSource::HasFailed() { return(m_failureReason.GetLength() > 0); }

/**
 * Get a brief description of the IO error that has disabled this JSON text source.
 * @return The (read-only) error description -- an empty string if no such error has occurred. 
 */
const CString& JSONTextSource::GetFailureReason() { return(m_failureReason); }

/**
 * Check the read buffer. If there are still some characters available to consume, do nothing. Otherwise, load the next
 * BUFSZ bytes from the file into the buffer. Fewer bytes may be read in if EOF is reached. The first time this method
 * is called, the source file is opened. When the last of the file's contents have been read, the file is closed.
 *
 * Whenever the read buffer is reloaded, the last LOOKBACKSZ characters are copied into an internal buffer so that
 * JSONTextSource can always provide a substring of up to LOOKBACKSZ characters preceding the current character
 * index position.
 *
 * @return True if there's still some characters available in buffer, or buffer was successfully reloaded from file;
 * false if an IO error occurred while reading the file, or if we've already reached EOF.
 */
bool JSONTextSource::GetNextBufferFromFileIfNecessary()
{
   if((m_charIndex >= m_totalLen) || (m_failureReason.GetLength() > 0)) return(false);
   
   // if there are still some available characters in the read buffer, do nothing.
   if(m_last > -1 && m_curr <= m_last) return(true);
   
   // open the file if it has not yet been opened
   if(m_pFile == NULL)
   {
      if(m_filePath.GetLength() == 0)
      {
         m_failureReason = "No source filename specified!";
         return(false);
      }
      
      m_pFile = new CFile();
      CFileException openExc;
      if(!m_pFile->Open(m_filePath, CFile::modeRead, &openExc))
      {
         LPTSTR pErrBuf = m_failureReason.GetBuffer(256);
         openExc.GetErrorMessage(pErrBuf, 256);
         m_failureReason.ReleaseBuffer();
      
         delete m_pFile;
         m_pFile = NULL;
         return(false);
      }
      
      m_totalLen = (long) m_pFile->GetLength();
      m_charIndex = 0;
      m_lineIndex = 0;
   }
   
   // copy last LOOKBACKSZ characters in m_pBuffer to m_strEndOfPrevBuf
   if(m_last > -1)
   {
      int start = m_last - LOOKBACKSZ + 1;
      if(start < 0) start = 0;
      int count = m_last - start + 1;
      m_strEndOfPrevBuf = "";
      LPTSTR pBuf = m_strEndOfPrevBuf.GetBuffer(count+1);
      for(int i=0; i<count; i++) pBuf[i] = m_pBuffer[start+i];
      pBuf[count] = '\0';
      m_strEndOfPrevBuf.ReleaseBuffer();
   }
   
   // read in next BUFSZ characters (or less if we're near EOF)
   int nRead = BUFSZ;
   if(m_totalLen - m_charIndex < BUFSZ) nRead = (int) (m_totalLen - m_charIndex);
   bool ok = false;
   try
   {
      int nActual = m_pFile->Read(m_pBuffer, (UINT) nRead);
      if(nActual != nRead)
         m_failureReason.Format("Unexpected read error: Read only %d of next %d bytes from file", nActual, nRead);
      else
      {
         m_curr = 0;
         m_last = nRead-1;
         ok = true;
      }
   }
   catch(CFileException* fileExc)
   {
      LPTSTR pErrBuf = m_failureReason.GetBuffer(256);
      fileExc->GetErrorMessage(pErrBuf, 256);
      m_failureReason.ReleaseBuffer();

      fileExc->Delete();
   }

   // close file if an error occurred or if we've reached EOF
   if(m_charIndex + nRead >= m_totalLen || !ok)
   {
      try { m_pFile->Close(); }
      catch(CFileException* e) { e->Delete(); }
      delete m_pFile;
      m_pFile = NULL;
   }
   
   return(ok);
}


/* JSONValue implementation ========================================================================================= */

/**
 * Parse the entire contents of the JSON text source, which should contain a complete, self-contained JSON entity --
 * either a JSON object or a JSON array.
 * @param pSrc The JSON-formatted text content source from which the JSON entity is to be parsed.
 * @param errMsg On failure, this contains an error message; else, it is left unchanged.
 * @return Pointer to the JSON value parsed from the source, which must be a JSON object or JSON array; NULL on failure.
 */
JSONValue* JSONValue::ParseComplete(JSONTextSource* pSrc, CString& errMsg)
{
   // Skip any preceding whitespace, end of data = no JSON = fail
   if(!pSrc->SkipWhitespace())
   {
      errMsg = "Invalid JSON source: No valid JSON entity found.";
      return(NULL);
   }

   // parse the JSON value immediately after any whitespace found. It must be a JSON object or array!
   JSONValue* pValue = JSONValue::Parse(pSrc, errMsg);
   if(pValue == NULL) return(NULL);
   else if(!(pValue->IsArray() || pValue->IsObject()))
   {
      delete pValue;
      errMsg = "Invalid JSON source: JSON content parsed, but is neither an object nor an array.";
      return(NULL);
   }

   // only whitespace can follow the JSON entity, skip over it and we should reach end of source text
   if(pSrc->SkipWhitespace())
   {
      delete pValue;
      errMsg = "Invalid JSON source: Found additional non-whitespace content after parsed JSON entity.";
      return(NULL);
   }

   // success!
   return(pValue);
}


/** Construct a JSON "null" value. */
JSONValue::JSONValue(/*NULL*/)
{
   m_type = JSONType_Null;
   m_string = "";
   m_boolean = false;
   m_number = 0.0;
   m_pArray = NULL;
   m_pObject = NULL;
}

/**
 * Construct a JSON value of the specified type. This is primarily intended for creating an empty JSON object or JSON
 * array value. Protected methods are used to populate the contents of the object or array. If used to create a JSON
 * string, number or boolean value, its value will be set to "", 0.0, or false, respectively.
 */
JSONValue::JSONValue(JSONType type)
{
   m_type = type;
   m_string = "";
   m_boolean = false;
   m_number = 0.0;
   m_pArray = (m_type == JSONType_Array) ? new JSONArray() : NULL;
   m_pObject = (m_type == JSONType_Object) ? new JSONObject(10) : NULL;
}

/** 
 * Construct a JSON string value.
 * @param str The string value.
 */
JSONValue::JSONValue(LPCTSTR str)
{
   m_type = JSONType_String;
   m_string = (str == NULL) ? "" : str;
   m_boolean = false;
   m_number = 0.0;
   m_pArray = NULL;
   m_pObject = NULL;
}

/** 
 * Construct a JSON boolean value.
 * @param b The boolean value assigned.
 */
JSONValue::JSONValue(bool b)
{
   m_type = JSONType_Bool;
   m_string = "";
   m_boolean = b;
   m_number = 0.0;
   m_pArray = NULL;
   m_pObject = NULL;
}

/** 
 * Construct a JSON number value.
 * @param num The number to use as the value.
 */
JSONValue::JSONValue(double num)
{
   m_type = JSONType_Number;
   m_string = "";
   m_boolean = false;
   m_number = num;
   m_pArray = NULL;
   m_pObject = NULL;
}

/** 
 * Destroy this JSON value. If the value is a JSON array or JSON object, the JSON values within the array or object
 * are deleted here.
 */
JSONValue::~JSONValue()
{
   if(m_type == JSONType_Array && m_pArray != NULL)
   {
      int i = 0;
      while(i < m_pArray->GetSize())
      {
         delete m_pArray->GetAt(i++);
      }
      m_pArray->RemoveAll();
      delete m_pArray;
      m_pArray = NULL;
   }
   else if(m_type == JSONType_Object && m_pObject != NULL)
   {
      POSITION pos = m_pObject->GetStartPosition();
      while(pos != NULL)
      {
         JSONValue* pValue;
         CString key;
         m_pObject->GetNextAssoc(pos, key, pValue);
         delete pValue;
      }
      m_pObject->RemoveAll();
      delete m_pObject;
      m_pObject = NULL;
   }
}

/** 
 * Checks if the value is a JSON "null" value.
 * @return True if it is a JSON null, false otherwise.
 */
bool JSONValue::IsNull() { return(m_type == JSONType_Null); }

/** 
 * Checks if the value is a JSON string.
 * @return True if it is a JSON string, false otherwise.
 */
bool JSONValue::IsString() { return(m_type == JSONType_String); }

/** 
 * Checks if the value is a JSON boolean ("true" or "false").
 * @return True if it is a JSON boolean, false otherwise.
 */
bool JSONValue::IsBool() { return(m_type == JSONType_Bool); }

/** 
 * Checks if the value is a JSON number value.
 * @return True if it is a JSON number, false otherwise.
 */
bool JSONValue::IsNumber() { return(m_type == JSONType_Number); }

/** 
 * Checks if the value is a JSON array.
 * @return True if it is a JSON array, false otherwise.
 */
bool JSONValue::IsArray() { return(m_type == JSONType_Array); }

/** 
 * Checks if the value is a JSON object.
 * @return True if it is a JSON object, false otherwise.
 */
bool JSONValue::IsObject() { return(m_type == JSONType_Object); }

/** 
 * Retrieves the JSONValue's string value.
 * @return The string value. Returns an empty string if this JSONValue is not a string.
 */
CString JSONValue::AsString() { return(m_string); }

/** 
 * Retrieves the JSONValue's boolean value.
 * @return The boolean value. Returns false if this JSONValue is not a boolean
 */
bool JSONValue::AsBool() { return(m_boolean); }

/** 
 * Retrieves the JSONValue's number value.
 * @return The number value. Returns 0.0 if this JSONValue is not a number.
 */
double JSONValue::AsNumber() { return(m_number); }

/** 
 * Retrieves the JSONValue's array value.
 * @return Pointer to the JSON array. Changes made via the pointer WILL affect this JSON value! Returns NULL if this
 * JSONValue is not an array.
 */
JSONArray* JSONValue::AsArray() { return(m_pArray); }

/** 
 * Retrieves the JSONValue's object value.
 * @return Pointer to the JSON object. Changes made via the pointer WILL affect this JSON value! Returns NULL if this
 * JSONValue is not an object.
 */
JSONObject* JSONValue::AsObject() { return(m_pObject); }

/**
 * Add a JSON value to this JSON array. No action taken if this JSONValue is not an array.
 * @param Pointer to the JSONValue to be added to the array. No action taken if this is NULL
 */
void JSONValue::AddToArray(JSONValue* pValue)
{
   if(m_type == JSONType_Array && pValue != NULL)
      m_pArray->Add(pValue);
}

/**
 * Add a (key, value)-pair to this JSON object. No action taken if this JSONValue is not an object.
 * @param key Name under which value is stored. No action taken if NULL. If key is already present in the object, then
 * the corresponding value is deleted and replaced by the value provided.
 * @param Pointer to the JSONValue to be added to be stored in the object. No action taken if NULL.
 */
void JSONValue::AddToObject(LPCTSTR key, JSONValue* pValue)
{
   if(key == NULL || pValue == NULL || m_type != JSONType_Object) return;
   
   // if key exists, first delete the mapped value and remove the key
   JSONValue *pOldValue;
   if(m_pObject->Lookup(key, pOldValue))
   {
      delete pOldValue;
      m_pObject->RemoveKey(key);
   }
   
   m_pObject->SetAt(key, pValue);
}


/**
 * Parses a JSON-encoded value from the JSON text source provided, starting at the current position in the source.
 * After a successful parse, the source position is at the character immediately after the text fragment encoding the
 * JSON value returned.
 *
 * @param pSrc The JSON-formatted text content source from which value is to be parsed.
 * @param errMsg On failure, this contains an error message; else, it is left unchanged.
 * @return Pointer to the JSON value parsed from the source, or NULL on failure.
 */
JSONValue* JSONValue::Parse(JSONTextSource* pSrc, CString& errMsg)
{
   // determine nature of the value token by its first character (we assume we are at the start of that token!)
   char firstChar = pSrc->LookAtNextChar();

   // a string token delineated by double-quotes: "string"
   if(firstChar == '"')
   {
      CString str;
      if(!pSrc->ExtractString(str))
      {
         errMsg.Format("Invalid JSON string near index=%d : %s", pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
         return(NULL);
      }
      else
         return(new JSONValue(str));
   }

   // one of the unquoted tokens: true, false, or null
   else if(firstChar == 't' || firstChar == 'f' || firstChar == 'n')
   {
      CString frag;
      if(pSrc->GetFragment((firstChar == 'f') ? 5 : 4, frag))
      {
         if(frag.Compare("true") == 0) return(new JSONValue(true));
         else if(frag.Compare("false") == 0) return(new JSONValue(false));
         else if(frag.Compare("null") == 0) return(new JSONValue());
      }
      
      errMsg.Format("Invalid JSON token near index=%d : %s", pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
      return(NULL);
   }

   // a number token: starts with a negative sign or digit
   else if(firstChar == '-' || (firstChar >= '0' && firstChar <= '9'))
   {
      double number = 0.0;
      if(!pSrc->ExtractNumber(&number))
      {
         errMsg.Format("Invalid JSON number near index=%d : %s", pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
         return(NULL);
      }
      
      return(new JSONValue(number));
   }

   // a JSON object: starts with a '{'
   else if(firstChar == '{')
   {
      JSONValue* pObj = new JSONValue(JSONType_Object);
      bool isEmptyObj = true;
      
      firstChar = pSrc->GetNextCharAndAdvance();
      while(firstChar != 0)
      {
         // parse each key/value pair in object:  <w>"key"<w>:<w>value<w>, where <w>=whitepace
         
         // skip over any initial whitespace; we should never reach the end of the source while doing so!
         if(!pSrc->SkipWhitespace())
         {
            delete pObj;
            errMsg.Format("Reached end of source inside a JSON object entity near index=%d : %s",
                  pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
            return(NULL);
         }
         firstChar = pSrc->LookAtNextChar();

         // special case - empty object
         if(isEmptyObj && (firstChar == '}'))
         {
            pSrc->GetNextCharAndAdvance();
            return(pObj);
         }
         
         // next token must be a JSON string token defining the "key" in a "key":value pair
         CString key;
         if(!pSrc->ExtractString(key))
         {
            delete pObj;
            errMsg.Format("Could not parse key string in key:value pair in JSON object near index=%d : %s",
                  pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
            return(NULL);
         }
         
         // skip over any whitespace after the key string token; again, we should not reach end of source here.
         if(!pSrc->SkipWhitespace())
         {
            delete pObj;
            errMsg.Format("Reached end of source inside a JSON object entity near index=%d : %s",
                  pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
            return(NULL);
         }
         firstChar = pSrc->GetNextCharAndAdvance();
         
         // expect the ":" separating key string and JSON value
         if(firstChar != ':')
         {
            delete pObj;
            errMsg.Format("Missing colon in key:value pair in JSON object near index=%d : %s",
                  pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
            return(NULL);
         }

         // skip over any whitespace after the ':'
         if(!pSrc->SkipWhitespace())
         {
            delete pObj;
            errMsg.Format("Reached end of source inside a JSON object entity near index=%d : %s",
                  pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
            return(NULL);
         }
         
         // now parse the value in the key:value pair -- NOTE: recursive!!	
         JSONValue* pValue = Parse(pSrc, errMsg);
         if(pValue == NULL)
         {
            delete pObj;
            return(NULL);
         }
         
         // add the key:value pair to the JSON object
         pObj->AddToObject(key, pValue);
         isEmptyObj = false;

         // skip over any whitespace after the key:value pair
         if(!pSrc->SkipWhitespace())
         {
            delete pObj;
            errMsg.Format("Reached end of source inside a JSON object entity near index=%d : %s",
                  pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
            return(NULL);
         }
         
         // get next character and advance. This character must be the closing bracket '}' marking the end of the object
         // definition, or a comma separating the just-parsed key:value pair from the next pair. If not, then the object
         // entity is invalid. 
         firstChar = pSrc->GetNextCharAndAdvance();
         if(firstChar == '}')
            return(pObj);
         else if(firstChar != ',')
         {
            delete pObj;
            errMsg.Format("Missing comma after key:value pair in JSON object near index=%d : %s",
                  pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
            return(NULL);
         }
         
         firstChar = pSrc->LookAtNextChar();
      }
      
      // if we get here, then we reached end of text source in the middle of an object entity
      delete pObj;
      errMsg.Format("Reached end of source inside a JSON object entity near index=%d : %s",
                  pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
      return(NULL);
   }

   // a JSON array: starts with a '['
   else if(firstChar == '[')
   {
      JSONValue* pArray = new JSONValue(JSONType_Array);
      bool isEmptyArray = true;
      
      firstChar = pSrc->GetNextCharAndAdvance();
      while(firstChar != 0)
      {
         // parse each value in array:  <w>value<w>, where <w>=whitepace
         
         // skip over any initial whitespace; we should never reach the end of the source while doing so!
         if(!pSrc->SkipWhitespace())
         {
            delete pArray;
            errMsg.Format("Reached end of source inside a JSON array entity near index=%d : %s",
                  pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
            return(NULL);
         }
         firstChar = pSrc->LookAtNextChar();

         // special case - empty array
         if(isEmptyArray && (firstChar == ']'))
         {
            pSrc->GetNextCharAndAdvance();
            return(pArray);
         }
         
         // now parse the value -- NOTE: recursive!!	
         JSONValue* pValue = Parse(pSrc, errMsg);
         if(pValue == NULL)
         {
            delete pArray;
            return(NULL);
         }
         
         // append value to the JSON array
         pArray->AddToArray(pValue);
         isEmptyArray = false;

         // skip over any whitespace after the value; again, we should not reach end of source while doing so.
         if(!pSrc->SkipWhitespace())
         {
            delete pArray;
            errMsg.Format("Reached end of source inside a JSON array entity near index=%d : %s",
                  pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
            return(NULL);
         }
         
         // get next character and advance. This character must be the closing bracket ']' marking the end of the array
         // definition, or a comma separating the just-parsed value token from the next. If not, then the array entity
         // is invalid. 
         firstChar = pSrc->GetNextCharAndAdvance();
         if(firstChar == ']')
            return(pArray);
         else if(firstChar != ',')
         {
            delete pArray;
            errMsg.Format("Missing comma after a value token in JSON array near index=%d : %s",
                  pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
            return(NULL);
         }
         
         firstChar = pSrc->LookAtNextChar();
      }
      
      // if we get here, then we reached end of text source in the middle of an object entity
      delete pArray;
      errMsg.Format("Reached end of source inside a JSON array entity near index=%d : %s",
                  pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
      return(NULL);
   }

   // otherwise, first character from source does not mark the start of a valid JSON token of any kind!
   else
   {
      errMsg.Format("Invalid token starting at index=%d : %s", pSrc->GetCharIndex(), pSrc->GetTextUpToCurrChar(20));
      return(NULL);
   }
}
