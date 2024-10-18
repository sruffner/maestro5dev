//===================================================================================================================== 
//
// jmxdocimporter.h : Declaration of utility class JMXDocImporter.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(JMXDOCIMPORTER_H__INCLUDED_)
#define JMXDOCIMPORTER_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "jsonvalue.h"                     // for JSONValue and JSONTextSource
#include "cxdoc.h"                         // for CCxDoc

class JMXDocImporter
{
public:
   JMXDocImporter() {}
   ~JMXDocImporter() {}
   BOOL DoImport(LPCTSTR filePath, CCxDoc* pDoc, CString& errMsg);

private:
   JMXDocImporter(const JMXDocImporter& src);                // copy constructor is NOT defined
   JMXDocImporter& operator=(const JMXDocImporter& src);     // assignment op is NOT defined
   
   BOOL ImportSettings(JSONObject* pJMX, CCxDoc* pDoc, CString& errMsg);
   BOOL ImportChanCfgs(JSONObject* pJMX, CCxDoc* pDoc, CString& errMsg);
   BOOL ImportPerts(JSONObject* pJMX, CCxDoc* pDoc, CString& errMsg);
   BOOL ImportTargetSets(JSONObject* pJMX, CCxDoc* pDoc, CString& errMsg);
   WORD ImportRMVTarget(CCxDoc* pDoc, WORD wSet, LPCTSTR name, LPCTSTR type, JSONArray* pParams, CString& errMsg);
   BOOL ImportTrialSets(JSONObject* pJMX, CCxDoc* pDoc, CString& errMsg);
   WORD ImportTrialSubset(CCxDoc* pDoc, WORD wSet, JSONObject* pJSONSubset, CString& errMsg);
   WORD ImportTrial(CCxDoc* pDoc, WORD wSet, JSONObject* pJSONTrial, CString& errMsg);
   
   static LPCTSTR STR_JMXCHANNELIDS[38];
   static LPCTSTR STR_JMXTRACECOLORNAMES[12];
   static LPCTSTR STR_JMXTGTTYPES_RMV[RMV_NUMTGTTYPES];
   static LPCTSTR STR_JMXSPECIALOPS[TH_NUMSPECOPS];
   static LPCTSTR STR_JMXPERTCMPTS[PERT_NCMPTS];
   
   /** Maps name of each imported JMX document channel configuration to corresponding document object key. */
   CMap<CString, LPCTSTR, WORD, WORD> m_chanCfgsMap;
   /** Maps name of each imported JMX document perturbation to corresponding document object key. */
   CMap<CString, LPCTSTR, WORD, WORD> m_pertsMap;
   /** Maps "setName/tgtName" of each imported JMX document target to corresponding document object key. */
   CMap<CString, LPCTSTR, WORD, WORD> m_targetsMap;
   /** 
   Map contains "setName/tgtName" for every XYScope target that was skipped during import. Any trial that depends on an
   XYScope target is also silently skipped (instead of aborting the import). 
   */
   CMap<CString, LPCTSTR, WORD, WORD> m_xyTgtsSkipped;
};

#endif  // !defined(JMXDOCIMPORTER_H__INCLUDED_)

