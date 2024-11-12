//=====================================================================================================================
//
// jmxdocimporter.cpp : Implementation of utility class JMXDocImporter.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// JMXDocImporter handles the task of reading in the contents of a JSON-formatted Maestro experiment (JMX) document
// file prepared in Matlab (via the maestrodoc() M-function with supporting JAR) and importing it into Maestro's single
// active experiment document, encapsulated by the CCxDoc class. 
//
// To use JMXDocImporter, construct an instance and call DoImport(), supplying the full pathname of the JMX file and a
// pointer to the experiment document. The previous contents of that document are deleted, so be sure to save the
// document before invoking DoImport(). 
//
// REVISION HISTORY:
// 20apr2010-- Began development.
// 02feb2011-- Added new special op 'search' to STR_JMXSPECIALOPS.
// 21sep2011-- ImportTrial() modified so that it no longer recognizes predefined targets "Fiber1", "Fiber2", "REDLED1",
// and "REDLED2". As of Maestro 3.0, these targets are no longer supported.
// 17may2012-- NOTE on channel IDs in STR_JMXCHANNELIDS : The channel IDs "htpos", "vtpos", "htpos2" and "vtpos2" have 
// no meaning in Maestro 3, since Fiber1/2 are no longer supported. However, we still use them here; instead, 
// maestrodoc() has been extended to accept either the use-specific IDs or the more generic "aiN". In the latter case, 
// maestrodoc() maps the generic ID to the use-specific ID in STR_JMXCHANNELIDS. That way the JMX document generated
// can be imported by Maestro 2.x or Maestro 3 (backwards compatibility).
// 02dec2014-- ImportTrialSets() modified to support importing trial sets containing trial "subsets", a feature 
// introduced in Maestro v3.1.2. The Matlab maestrodoc() function was modified to allow creation of such subsets.
// 26sep2016-- Modified IAW method prototype changes in CCxTrial. These were made while introducing the notion of
// random variables. However, at least for now, JMXDocImporter will NOT support defining random variables in a trial
// object.
// 11oct2016-- Modified to support new RMVideo target type, RMV_IMAGE.
// 31jan2017—- Modified to recognize ‘rgbcon’ parameter for RMV_RANDOMDOTS target type, in order to specify a two-color
// contrast dot patch (which was introduced in Maestro v3.1.2, Dec 2014).
// 06sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017. Also, replaced old class
// CMapStringToWord with CMap<CString, LPCTSTR, WORD, WORD>.
// 02oct2018-- Modified to support new RMVideo "vertical sync spot flash" feature. New RMVideo display settings (in 
// "settings.rmv") govern the spot size in mm and the flash durationn # of video frames s. A new optional trial segment
// header parameter, "rmvsync", is a flag that enables the flash during the first frame marking the start of that
// segment on the display (nonzero = enable; default = 0, disabled).
// 07may2019-- Modified to support new "flicker" feature available for all RMVideo target types. Three new parameters
// govern the flicker ON duration, OFF duration, and initial delay (to first ON phase). All parameters are specified 
// in # of video frames and all default to 0, which disables the feature. See ImportRMVTarget().
// 15may2019-- Modified to support new "random reward withholding variable ratio" feature in a trial. The feature is
// specified by the parameter name "rewWHVR" and the parameter value is an array of 4 integers [N1 D1 N2 D2] setting 
// the VR's numerator N and denominator D for reward pulse 1 and 2.
// 14aug2019-- Modified to handle importing the per-segment, per-target trajectory flag controlling whether or not
// target "snaps to eye" at the segment start when VStab is on. See ImportTrial().
// 14aug2019-- Added an 8th numeric parameter to the field "settings.other" that specifies the sliding average window
// length in ms for the VStab feature. To avoid breaking any existing JMX documents, this parameter defaults to 1 if
// it is missing.
// 18oct2024-- Modified to ignore all XYScope-related content -- display settings, targets, and any trials that use
// XYScope targets. See ImportTargetSets().
// 11nov2024-- maestrodoc() v1.2.2 adds support for defining and using trial random variables using new fields
// trial.rvs and trial.rvuse. See ImportTrial() for details.
//          -- maestrodoc() v1.2.2 no longer produces JMX docs with XYScope-related content. While JMXDocImporter
// has already been updated to ignore such content, had to update ImportTargetSets() to treat the target "isxy" field
// as optional rather than required.
//=====================================================================================================================

#include <stdafx.h>                          // standard MFC stuff

#include "cxsettings.h"                      // CCxSettings -- application settings
#include "cxpert.h"                          // CCxPert -- the CNTRLX perturbation data class
#include "cxchannel.h"                       // CCxChannel -- the CNTRLX channel configuration data class
#include "cxtarget.h"                        // CCxTarget -- the CNTRLX target data class
#include "cxtrial.h"                         // CCxTrial -- the CNTRLX trial data class
#include "util.h"                            // for cMath functions

#include "jmxdocimporter.h"


/**
 * Import the contents of a JSON-formatted Maestro experiment (JMX) document file into the active Maestro experiment
 * document object, replacing all existing content (except for application-defined objects, of course). If the import 
 * fails for whatever reason, the experiment document is reset to its "new document" state.
 *
 * @param filePath Full pathname of the JMX document file.
 * @param pDoc Pointer to the Maestro experiment document.
 * @param errMsg Reference to a CString in which an error description is stored should the import fail.
 * @return TRUE if successful; FALSE otherwise.
 */
BOOL JMXDocImporter::DoImport(LPCTSTR filePath, CCxDoc* pDoc, CString& errMsg)
{
   ASSERT(filePath != NULL && pDoc != NULL);
   
   // reset internals, just in case we're reusing this importer
   m_chanCfgsMap.RemoveAll();
   m_pertsMap.RemoveAll();
   m_targetsMap.RemoveAll();
   m_xyTgtsSkipped.RemoveAll();

   
   // reset the experiment document now. If unable to do so, abort
   if(!pDoc->OnNewDocument())
   {
      errMsg = "Import aborted. Unable to reset the experiment document.";
      return(FALSE);
   }
   
   // attempt to parse the file specified as a JMX document
   JSONTextSource* pSrc = new JSONTextSource(filePath);
   JSONValue* pJMXDoc = JSONValue::ParseComplete(pSrc, errMsg);
   delete pSrc;
   if(pJMXDoc == NULL) return(FALSE);
   
   // we parsed a valid JSON entity from the file. Begin importing its contents into experiment document. As we do so,
   // if we encounter any invalid content, we abort the import and reset the document.
   BOOL ok = pJMXDoc->IsObject();
   if(!ok) errMsg = "Root entity in JMX file is not a JSON object!";
   
   if(ok) ok = ImportSettings(pJMXDoc->AsObject(), pDoc, errMsg);
   if(ok) ok = ImportChanCfgs(pJMXDoc->AsObject(), pDoc, errMsg);
   if(ok) ok = ImportPerts(pJMXDoc->AsObject(), pDoc, errMsg);
   if(ok) ok = ImportTargetSets(pJMXDoc->AsObject(), pDoc, errMsg);
   if(ok) ok = ImportTrialSets(pJMXDoc->AsObject(), pDoc, errMsg);
   
   // if there were XYScope targets in the JMXDoc, any trials using them would be skipped over, resulting perhaps
   // in some empty trial sets -- which we remove.
   if(ok && !m_xyTgtsSkipped.IsEmpty())
      pDoc->RemoveEmptyTrialSets();

   delete pJMXDoc;
   
   if(!ok) pDoc->OnNewDocument();
   return(ok);
}

/**
 * Helper method for DoImport(). Validates the content of the "settings" field in the JMX document object and imports
 * those settings into the Maestro experiment document (CCxDoc).
 * 
 * Migration considerations:
 * 1) If document version < 3, then settings.rmv = [W H D BKGC]; else = [W H D BKGC SZ DUR]. Support for RMVideo VSync 
 * spot flash feature, with spot size SZ and flash duration DUR, was added in Maestro 4.0.0.
 * 2) As of Maestro 4.1.1, the VStab sliding window length (ms) is persisted in the Maestro experiment document and can
 * be included in a JMX document. To avoid breaking existing JMX documents, it is stored as the 8th parameter in the
 * field settings.other = [D P1 P2 OVRIDE? VARATIO AUDIOREW BEEP? VSTABWIN]. If only 7 parameters are found, then the
 * VStab window length is left unchanged.
 * 3) As of Maestro 5.0, the XYScope platform -- unsupported since V4.0 -- is dropped entirely. SO the "settings.xy"
 * field in the JMX doc, which lists the XYScope display parameters, is ignored.
 *
 * @param pJMX Pointer to the JMX document object (a JSON object).
 * @param pDoc Pointer to the Maestro experiment document.
 * @param errMsg Reference to a CString in which an error description is stored should the import fail.
 * @return TRUE if successful; FALSE otherwise.
 */
BOOL JMXDocImporter::ImportSettings(JSONObject* pJMX, CCxDoc* pDoc, CString& errMsg)
{
   int i;
   JSONValue* pValue;

   // need document version # to migrate settings.rmv for V<3
   int version = 0;
   BOOL ok = pJMX->Lookup("version", pValue) && pValue->IsNumber();
   if(!ok)
   {
      errMsg = "Unable to read document version for migration purposes";
      return(FALSE);
   }
   version = (int)pValue->AsNumber();

   ok = pJMX->Lookup("settings", pValue) && pValue->IsObject();
   if(!ok)
   {
      errMsg = "Missing or invalid field in JMX document object: 'settings'";
      return(FALSE);
   }
   
   JSONObject* pJMXSettings = pValue->AsObject();
   CCxSettings* pSettings = pDoc->GetSettings();
   
   // settings.rmv = [w h d bkg] if V<3; else [w h d bkg sz dur]; all integer values
   int nEl = (version >= 3) ? 6 : 4;
   ok = pJMXSettings->Lookup("rmv", pValue) && pValue->IsArray() && (pValue->AsArray()->GetSize() == nEl);
   for(i=0; ok && i<nEl; i++) ok = pValue->AsArray()->ElementAt(i)->IsNumber();
   if(!ok)
   {
      errMsg = "Missing or invalid field: 'settings.rmv'";
      return(FALSE);
   }
   
   pSettings->SetFBWidth((int) pValue->AsArray()->ElementAt(0)->AsNumber());
   pSettings->SetFBHeight((int) pValue->AsArray()->ElementAt(1)->AsNumber());
   pSettings->SetFBDistToEye((int) pValue->AsArray()->ElementAt(2)->AsNumber());
   int rgb = (int) pValue->AsArray()->ElementAt(3)->AsNumber();
   pSettings->SetFBBkgRed((rgb >> 16) & 0x00ff);
   pSettings->SetFBBkgGrn((rgb >> 8) & 0x00ff);
   pSettings->SetFBBkgBlu(rgb & 0x00ff);

   if(version < 3)
   {
      pSettings->SetRMVSyncFlashSize(0);
      pSettings->SetRMVSyncFlashDuration(1);
   }
   else
   {
      pSettings->SetRMVSyncFlashSize((int)pValue->AsArray()->ElementAt(4)->AsNumber());
      pSettings->SetRMVSyncFlashDuration((int)pValue->AsArray()->ElementAt(5)->AsNumber());
   }

   // settings.fix = [hFixAcc vFixAcc], both floating-pt
   ok = pJMXSettings->Lookup("fix", pValue) && pValue->IsArray() && (pValue->AsArray()->GetSize() == 2);
   for(i=0; ok && i<2; i++) ok = pValue->AsArray()->ElementAt(i)->IsNumber();
   if(!ok)
   {
      errMsg = "Missing or invalid field: 'settings.fix'";
      return(FALSE);
   }
   
   pSettings->SetFixAccH((float) pValue->AsArray()->ElementAt(0)->AsNumber());
   pSettings->SetFixAccV((float) pValue->AsArray()->ElementAt(1)->AsNumber());

   // settings.other = [d p1 p2 ovride? varatio audiorew beep? vstabwin], all integer values. The vstabwin
   // parameter may be missing, in which case the VStab window length is unchanged (this was done because
   // the parameter was added later, and I didn't want to break existing JMX documents, if any)
   ok = pJMXSettings->Lookup("other", pValue) && pValue->IsArray() && (pValue->AsArray()->GetSize() >= 7)
         && (pValue->AsArray()->GetSize() <= 8);

   for(i=0; ok && i<pValue->AsArray()->GetSize(); i++) ok = pValue->AsArray()->ElementAt(i)->IsNumber();
   if(!ok)
   {
      errMsg = "Missing or invalid field: 'settings.other'";
      return(FALSE);
   }
   
   pSettings->SetFixDuration((int) pValue->AsArray()->ElementAt(0)->AsNumber());
   pSettings->SetRewardLen1((int) pValue->AsArray()->ElementAt(1)->AsNumber());
   pSettings->SetRewardLen2((int) pValue->AsArray()->ElementAt(2)->AsNumber());
   pSettings->SetTrialRewLenOverride(BOOL(pValue->AsArray()->ElementAt(3)->AsNumber() != 0));
   pSettings->SetVariableRatio((int) pValue->AsArray()->ElementAt(4)->AsNumber());
   pSettings->SetAudioRewardLen((int) pValue->AsArray()->ElementAt(5)->AsNumber());
   pSettings->SetRewardBeepEnabled(BOOL(pValue->AsArray()->ElementAt(6)->AsNumber() != 0));
   if(pValue->AsArray()->GetSize() == 8)
      pSettings->SetVStabWinLen((int)pValue->AsArray()->ElementAt(7)->AsNumber());

   return(TRUE);
}

/**
 * Helper method for DoImport(). Validates the content of the "chancfgs" field in the JMX document object and imports
 * all defined channel configurations into the Maestro experiment document (CCxDoc).
 *
 * @param pJMX Pointer to the JMX document object (a JSON object).
 * @param pDoc Pointer to the Maestro experiment document.
 * @param errMsg Reference to a CString in which an error description is stored should the import fail.
 * @return TRUE if successful; FALSE otherwise.
 */
BOOL JMXDocImporter::ImportChanCfgs(JSONObject* pJMX, CCxDoc* pDoc, CString& errMsg)
{
   int i, j, k;
   JSONValue* pValue;

   BOOL ok = pJMX->Lookup("chancfgs", pValue) && pValue->IsArray();
   if(!ok)
   {
      errMsg = "Missing or invalid field in JMX document object: 'chancfgs'";
      return(FALSE);
   }
   
   // all imported channel configurations are inserted under this object
   WORD wBase = pDoc->GetBaseObj(CX_CHANBASE); 
   ASSERT(wBase != CX_NULLOBJ_KEY);

   // import the channel configurations one at a time...
   JSONArray* pChanCfgs = pValue->AsArray();
   for(i=0; i<pChanCfgs->GetSize(); i++)
   {
      CString name;
      JSONArray* pChannels = NULL;
      WORD wKey;
      
      ok = pChanCfgs->ElementAt(i)->IsObject();
      if(ok)
      {
         JSONObject* pObj = pChanCfgs->ElementAt(i)->AsObject();
         ok = pObj->Lookup("name", pValue) && pValue->IsString();
         if(ok) name = pValue->AsString();
         if(ok) ok = pObj->Lookup("channels", pValue) && pValue->IsArray();
         if(ok) pChannels = pValue->AsArray();
      }
      if(!ok)
      {
         errMsg.Format("%d-th channel configuration is invalid in field 'chancfgs'", i);
         return(FALSE);
      }
      
      // ignore duplicates
      if(m_chanCfgsMap.Lookup(name, wKey))
         continue;

      // append a new channel configuration object with the name specified.
      wKey = pDoc->InsertObj(wBase, CX_CHANCFG, name);
      if(wKey == CX_NULLOBJ_KEY)
      {
         errMsg.Format("Unable to import %d-th channel configuration: low memory or document full", i);
         return(FALSE);
      }
      
      // if the channel config object's name was modified during insertion, fail
      if(pDoc->GetObjName(wKey).Compare(name) != 0)
      {
         errMsg.Format("Invalid name for %d-th channel config: %s", i, name);
         return(FALSE);
      }

      // process each channel description {CH_ID REC? DSP? OFFSET GAIN COLOR_ID} in 'channels' array
      CCxChannel* pChan = (CCxChannel*) pDoc->GetObject(wKey);
      for(j=0; j<pChannels->GetSize(); j++)
      {
         JSONArray* pChanDesc = NULL;
         ok = pChannels->ElementAt(j)->IsArray() && (pChannels->ElementAt(j)->AsArray()->GetSize() == 6);
         if(ok)
         {
            pChanDesc = pChannels->ElementAt(j)->AsArray();
            ok = pChanDesc->ElementAt(0)->IsString() && pChanDesc->ElementAt(5)->IsString();
            for(k=1; ok && k<=4; k++) ok = pChanDesc->ElementAt(k)->IsNumber();
         }
         if(ok)
         {
            CString chID = pChanDesc->ElementAt(0)->AsString();
            int iPos = -1;
            for(k=0; k<38; k++) if(chID.Compare(STR_JMXCHANNELIDS[k]) == 0)
            {
               iPos = k;
               break;
            }
            
            CString colorStr = pChanDesc->ElementAt(5)->AsString();
            int iColor = -1;
            for(k=0; k<12; k++) if(colorStr.Compare(STR_JMXTRACECOLORNAMES[k]) == 0)
            {
               iColor = k;
               break;
            }

            ok = (iPos > -1 && iColor > -1);
            if(ok)
            {
               BOOL isRec = (pChanDesc->ElementAt(1)->AsNumber() != 0);
               if(isRec != pChan->IsRecorded(iPos)) pChan->ToggleRecord(iPos);
               BOOL isDsp = (pChanDesc->ElementAt(2)->AsNumber() != 0);
               if(isDsp != pChan->IsDisplayed(iPos)) pChan->ToggleDisplay(iPos);
               pChan->SetOffset(iPos, (int) pChanDesc->ElementAt(3)->AsNumber());
               
               // gain value in [-5..5] must be converted to 0-base index [0..10]
               int gainIdx = 5 + ((int) pChanDesc->ElementAt(4)->AsNumber());
               gainIdx = (gainIdx < 0) ? 0 : ((gainIdx > 10) ? 10 : gainIdx);
               pChan->SetGainIndex(iPos, gainIdx);
               
               pChan->SetColorIndex(iPos, iColor);
            }
         }
         
         if(!ok)
         {
            errMsg.Format("Bad channel description (n=%d) in %d-th channel configuration", j, i);
            return(FALSE);
         }
      }
      
      // successfully imported channel configuration. Add name-key pair to our internal map so we can find the channel
      // configuration object key when importing a trial.
      m_chanCfgsMap.SetAt(name, wKey);
   }
   
   return(TRUE);
}

/**
 * Helper method for DoImport(). Validates each perturbation waveform definition in the "perts" field in the JMX 
 * document object and imports each perturbation waveform into the Maestro experiment document (CCxDoc).
 *
 * @param pJMX Pointer to the JMX document object (a JSON object).
 * @param pDoc Pointer to the Maestro experiment document.
 * @param errMsg Reference to a CString in which an error description is stored should the import fail.
 * @return TRUE if successful; FALSE otherwise.
 */
BOOL JMXDocImporter::ImportPerts(JSONObject* pJMX, CCxDoc* pDoc, CString& errMsg)
{
   int i, j;
   JSONValue* pValue;
   BOOL ok = pJMX->Lookup("perts", pValue) && pValue->IsArray();
   if(!ok)
   {
      errMsg = "Missing or invalid field in JMX document object: 'perts'";
      return(FALSE);
   }
   
   // all imported perturbations are inserted under this object
   WORD wBase = pDoc->GetBaseObj(CX_PERTBASE);
   ASSERT(wBase != CX_NULLOBJ_KEY);

   // import the perturbation waveforms one at a time...
   JSONArray* pJSONPertArray = pValue->AsArray();
   for(i=0; i<pJSONPertArray->GetSize(); i++)
   {
      JSONArray* pJSONPert = NULL;
      WORD wKey;
      
      ok = pJSONPertArray->ElementAt(i)->IsArray();
      if(ok)
      {
         pJSONPert = pJSONPertArray->ElementAt(i)->AsArray();
         ok = (pJSONPert->GetSize() == 5) || (pJSONPert->GetSize() == 6);
         for(j=0; ok && j<2; j++) ok = pJSONPert->ElementAt(j)->IsString();
         for(j=2; ok && j<pJSONPert->GetSize(); j++) ok = pJSONPert->ElementAt(j)->IsNumber();
      }
      if(!ok)
      {
         errMsg.Format("%d-th perturbation waveform is invalid in field 'perts'", i);
         return(FALSE);
      }
      
      // ignore duplicates
      CString name = pJSONPert->ElementAt(0)->AsString();
      if(m_pertsMap.Lookup(name, wKey))
         continue;

      // append a new perturbation waveform object with the name specified. The name could be auto-corrected
      wKey = pDoc->InsertObj(wBase, CX_PERTURB, name);
      if(wKey == CX_NULLOBJ_KEY)
      {
         errMsg.Format("Unable to import %d-th perturbation waveform: low memory or document full", i);
         return(FALSE);
      }
      
      // if the perturbation object's name was modified during insertion, fail
      if(pDoc->GetObjName(wKey).Compare(name) != 0)
      {
         errMsg.Format("Invalid name for %d-th perturbation waveform: %s", i, name);
         return(FALSE);
      }

      // set the new perturbation waveform object's parameters as specified in the JSON array
      PERT pertInfo;
      CString type = pJSONPert->ElementAt(1)->AsString();
      if(type.Compare("sinusoid") == 0) pertInfo.iType = PERT_ISSINE;
      else if(type.Compare("pulse train") == 0) pertInfo.iType = PERT_ISTRAIN;
      else if(type.Compare("uniform noise") == 0) pertInfo.iType = PERT_ISNOISE;
      else if(type.Compare("gaussian noise") == 0) pertInfo.iType = PERT_ISGAUSS;
      else
      {
         errMsg.Format("Unable to import %d-th perturbation waveform: Unrecognized type = %s", i, type);
         return(FALSE);
      }
      
      if(pertInfo.iType != PERT_ISSINE && pJSONPert->GetSize() == 5)
      {
         errMsg.Format("Unable to import %d-th perturbation waveform: Not enough params", i);
         return(FALSE);
      }
      
      pertInfo.iDur = (int) pJSONPert->ElementAt(2)->AsNumber();
      
      if(pertInfo.iType == PERT_ISSINE)
      {
         pertInfo.sine.iPeriod = (int) pJSONPert->ElementAt(3)->AsNumber();
         pertInfo.sine.fPhase = (float) pJSONPert->ElementAt(4)->AsNumber();
      }
      else if(pertInfo.iType == PERT_ISTRAIN)
      {
         pertInfo.train.iRampDur = (int) pJSONPert->ElementAt(3)->AsNumber();
         pertInfo.train.iPulseDur = (int) pJSONPert->ElementAt(4)->AsNumber();
         pertInfo.train.iIntv = (int) pJSONPert->ElementAt(5)->AsNumber();
      }
      else
      {
         pertInfo.noise.iUpdIntv = (int) pJSONPert->ElementAt(3)->AsNumber();
         pertInfo.noise.fMean = (float) pJSONPert->ElementAt(4)->AsNumber();
         pertInfo.noise.iSeed = (int) pJSONPert->ElementAt(5)->AsNumber();
      }
      
      CCxPert* pPert = (CCxPert*) pDoc->GetObject(wKey);
      pPert->SetPertInfo(pertInfo);
            
      // successfully imported perturbation waveform. Add name-key pair to our internal map so we can find the
      // perturbation waveform object key when importing a trial.
      m_pertsMap.SetAt(name, wKey);
   }
   
   return(TRUE);
}

/**
 * Helper method for DoImport(). Consumes the "targetSets" field in the JMX document object, importing each target set
 * defined into the Maestro experiment document (CCxDoc), along with all the targets defined within a target set.
 * 
 * 18oct2024: XYScope support dropped. Keeps a list the names of all XYScope targets encountered, but does not read in
 * their definitions. The list of XYScope target names is used to ignore any trials that use them.
 * 
 * 12nov2024: maestrodoc() v1.2.2 and later do not generate XYScope-related content. Modified to treat the target
 * "isxy" field as optional rather than required.
 * 
 * @param pJMX Pointer to the JMX document object (a JSON object).
 * @param pDoc Pointer to the Maestro experiment document.
 * @param errMsg Reference to a CString in which an error description is stored should the import fail.
 * @return TRUE if successful; FALSE otherwise.
 */
BOOL JMXDocImporter::ImportTargetSets(JSONObject* pJMX, CCxDoc* pDoc, CString& errMsg)
{
   JSONValue* pValue;
   BOOL ok = pJMX->Lookup("targetSets", pValue) && pValue->IsArray();
   if(!ok)
   {
      errMsg = "Missing or invalid field in JMX document object: 'targetSets'";
      return(FALSE);
   }
   
   // all imported target sets are inserted under this object
   WORD wBase = pDoc->GetBaseObj(CX_TARGBASE);
   ASSERT(wBase != CX_NULLOBJ_KEY);

   // import the target sets one at a time...
   CMap<CString, LPCTSTR, WORD, WORD> importedSetsMap;
   JSONArray* pJSONTgSetArray = pValue->AsArray();
   for(int i=0; i<pJSONTgSetArray->GetSize(); i++)
   {
      JSONObject* pJSONTgSet = NULL;
      CString setName;
      JSONArray* pJSONTargets = NULL;
      WORD wTgSetKey = CX_NULLOBJ_KEY;
      
      ok = pJSONTgSetArray->ElementAt(i)->IsObject();
      if(ok)
      {
         pJSONTgSet = pJSONTgSetArray->ElementAt(i)->AsObject();
         ok = pJSONTgSet->Lookup("name", pValue) && pValue->IsString();
         if(ok) setName = pValue->AsString();
         if(ok) ok = pJSONTgSet->Lookup("targets", pValue) && pValue->IsArray();
         if(ok) pJSONTargets = pValue->AsArray();
      }
      if(!ok)
      {
         errMsg.Format("%d-th target set is invalid in field 'targetSets'", i);
         return(FALSE);
      }
      
      // ignore duplicates
      if(importedSetsMap.Lookup(setName, wTgSetKey))
         continue;

      // append a new target set object with the name specified.
      wTgSetKey = pDoc->InsertObj(wBase, CX_TARGSET, setName);
      if(wTgSetKey == CX_NULLOBJ_KEY)
      {
         errMsg.Format("Unable to import %d-th target set: low memory or document full", i);
         return(FALSE);
      }
      
      // if the target set name was modified during insertion, fail
      if(pDoc->GetObjName(wTgSetKey).Compare(setName) != 0)
      {
         errMsg.Format("Invalid name for %d-th target set: %s", i, setName);
         return(FALSE);
      }

      // remember name of target set so we don't import another with the same name!
      importedSetsMap.SetAt(setName, wTgSetKey);
      
      // import all the targets in the set
      BOOL bSetEmpty = TRUE;
      for(int iTgt = 0; iTgt < pJSONTargets->GetSize(); iTgt++)
      {
         JSONObject* pJSONTarget = NULL;
         CString tgtName;
         BOOL isXYScope = FALSE;
         CString type;
         JSONArray* pJSONParams = NULL;
         WORD wTgtKey = CX_NULLOBJ_KEY;

         ok = pJSONTargets->ElementAt(iTgt)->IsObject();
         if(ok)
         {
            pJSONTarget = pJSONTargets->ElementAt(iTgt)->AsObject();
            ok = pJSONTarget->Lookup("name", pValue) && pValue->IsString();
            if(ok) tgtName = pValue->AsString();

            // the "isxy" field is optional, since it is not found in JMX doc generated by maestrodoc v1.2.2 or later
            if(ok && pJSONTarget->Lookup("isxy", pValue))
            {
               ok = pValue->IsNumber();
               if(ok) isXYScope = BOOL(pValue->AsNumber() != 0);
            }

            if(ok) ok = pJSONTarget->Lookup("type", pValue) && pValue->IsString();
            if(ok) type = pValue->AsString();
            if(ok) ok = pJSONTarget->Lookup("params", pValue) && pValue->IsArray();
            if(ok) pJSONParams = pValue->AsArray();
            if(ok) ok = BOOL((pJSONParams->GetSize() % 2) == 0);
         }
         if(!ok)
         {
            errMsg.Format("%d-th target in set %s is invalid in field 'targetSets'", iTgt, setName);
            return(FALSE);
         }
         
         // ignore duplicates
         CString fullName = setName + "/" + tgtName;
         if(m_targetsMap.Lookup(fullName, wTgtKey))
            continue;

         // the XYScope platform is unsupported since Maestro 4.0, and is removed entirely in V5.0. We skip over every
         // XYScope target in the JMX Document, but keep track of them so we can skip every trial that uses one. 
         if(isXYScope)
         {
            m_xyTgtsSkipped.SetAt(fullName, (WORD) 0);
            continue;
         }

         // import the RMVideo target (a/o Maestro 5.0, it is the only valid user-defined target type!)
         wTgtKey = ImportRMVTarget(pDoc, wTgSetKey, tgtName, type, pJSONParams, errMsg);
         if(wTgtKey == CX_NULLOBJ_KEY) return(FALSE);
         
         // at least one target added, so the set is not empty.
         bSetEmpty = FALSE;

         // add target pathname ("set/tgt") and key to our internal map so we can find the target object key when 
         // importing a trial.
         m_targetsMap.SetAt(fullName, wTgtKey);
      }

      // if the set contained only XYScope targets, which are obsolete and no longer imported, then it will be empty.
      // In that case, remove it.
      if(bSetEmpty)
      {
         pDoc->RemoveObj(wTgSetKey);
         importedSetsMap.RemoveKey(setName);
      }
   }
   
   return(TRUE);
}

/**
 Helper method for ImportTargetSets(). It imports a single RMVideo target definition into the Maestro experiment 
 document (CCxDoc).

 The JSON array listing the target's defining parameters is a sequence of ('name', value) pairs, where 'name' is
 the parameter name. If a parameter is omitted from the array, then it is set to a default value. The recognized
 RMVideo target types are listed below, along with the names of applicable parameters. Default parameter values are
 listed in parentheses.
    'point'      : 'dotsize' (1), 'rgb' (0x00ffffff), 'flicker' ([0 0 0])
    'dotpatch'   : 'dotsize' (1), 'rgb' (0x00ffffff), 'rgbcon' (0x00000000), 'ndots' (100), 'aperture' ('rect'), 
                   'dim' ([10 10 5 5]), 'sigma' ([0 0]), 'seed' (0), 'pct' (100), 'dotlf' ([1 0]), 
                   'noise' ([0 0 100 0]), 'wrtscreen' (0), 'flicker' ([0 0 0])
    'flowfield'  : 'dotsize' (1), 'rgb' (0x00ffffff), 'ndots' (100), 'dim' ([30 0.5]), 'seed' (0), 'flicker' ([0 0 0])
    'bar'        : 'rgb' (0x00ffffff), 'dim' ([10 10 0]), 'flicker' ([0 0 0])
    'spot'       : 'rgb' (0x00ffffff), 'aperture' ('rect'), 'dim' ([10 10 5 5]), 'sigma' ([0 0]), 'flicker' ([0 0 0])
    'grating'    : 'aperture' ('rect'), 'dim' ([10 10]), 'sigma' ([0 0]), 'square' (0), 'oriadj' (0),
                   'grat1' ([0x00808080 0x00646464 1.0 0 0]), 'flicker' ([0 0 0])
    'plaid'      : 'aperture' ('rect'), 'dim' ([10 10]), 'sigma' ([0 0]), 'square' (0), 'oriadj' (0), 'indep' (0),
                   'grat1' ([0x00808080 0x00646464 1.0 0 0]), 'grat2' ([0x00808080 0x00646464 1.0 0 0]),
                   'flicker' ([0 0 0])
    'movie'      : 'folder' ('folderName'), 'file' ('fileName'), 'flags' ([0 0 0]), 'flicker' ([0 0 0])
    'image'      : 'folder' ('folderName'), 'file' ('fileName'), 'flicker' ([0 0 0])

 @param pDoc Pointer to the Maestro experiment document.
 @param wSet Key of the target set under which imported target should be stored.
 @param name The target's name.
 @param type The target type -- one of the strings listed above.
 @param pParams Pointer to JSON array of parameter (name, value) pairs defining the target. Method assumes this
 array contains an even number of elements.
 @param errMsg Reference to a CString in which an error description is stored should the import fail.
 @return The key of the imported target object; CX_NULLOBJ_KEY if import fails for whatever reason.
*/
WORD JMXDocImporter::ImportRMVTarget(
      CCxDoc* pDoc, WORD wSet, LPCTSTR name, LPCTSTR type, JSONArray* pParams, CString& errMsg)
{
   int i, j;

   // get name of target set (for use in building error message)
   CString setName = pDoc->GetObjName(wSet);
   
   // convert JMX target type name to Maestro target type integer ID
   CString strType = type;
   int iTgtType = -1;
   for(i=0; i<RMV_NUMTGTTYPES; i++) if(strType.Compare(STR_JMXTGTTYPES_RMV[i]) == 0)
   {
      iTgtType = i;
      break;
   }
   if(iTgtType == -1)
   {
      errMsg.Format("Cannot import RMVideo target %s in set %s: Bad target type.", name, setName);
      return(CX_NULLOBJ_KEY);
   }

   // insert new target object under the specified target set
   int wKey = pDoc->InsertObj(wSet, CX_RMVTARG, name);
   if(wKey == CX_NULLOBJ_KEY)
   {
      errMsg.Format("Cannot import RMVideo target %s in set %s: low memory or document full", name, setName);
      return(CX_NULLOBJ_KEY);
   }
   
   // if target name was changed upon insertion, fail -- assume original target name is invalid.
   if(pDoc->GetObjName(wKey).Compare(name) != 0)
   {
      errMsg.Format("Invalid/duplicate name for target in set %s: %s", setName, name);
      return(CX_NULLOBJ_KEY);
   }

   // prepare default parametric definition of target IAW type
   U_TGPARMS tgParms;
   CCxTarget* pTgt = (CCxTarget*) pDoc->GetObject(wKey);
   pTgt->GetParams(&tgParms);
   
   tgParms.rmv.iType = iTgtType;
   tgParms.rmv.iAperture = RMV_RECT;
   tgParms.rmv.iFlags = 0;
   tgParms.rmv.iRGBMean[0] = 0x00FFFFFF;
   tgParms.rmv.fOuterW = tgParms.rmv.fOuterH = 10.0f;
   tgParms.rmv.fInnerW = tgParms.rmv.fInnerH = 5.0f;
   tgParms.rmv.fSigma[0] = tgParms.rmv.fSigma[1] = 0.0f;
   tgParms.rmv.iFlickerOn = tgParms.rmv.iFlickerOff = tgParms.rmv.iFlickerDelay = 0;
   switch(iTgtType)
   {
      case RMV_POINT :
         tgParms.rmv.nDotSize = 1;
         break;
      case RMV_RANDOMDOTS :
         tgParms.rmv.nDots = 100;
         tgParms.rmv.nDotSize = 1;
         tgParms.rmv.iSeed = 0;
         tgParms.rmv.iPctCoherent = 100;
         tgParms.rmv.iNoiseUpdIntv = 0;
         tgParms.rmv.iNoiseLimit = 100;
         tgParms.rmv.iFlags = RMV_F_LIFEINMS;
         tgParms.rmv.fDotLife = 0.0f;
         tgParms.rmv.iRGBCon[0] = 0;
         break;
      case RMV_FLOWFIELD :
         tgParms.rmv.fOuterW = 30.0f;
         tgParms.rmv.fInnerW = 0.5f;
         tgParms.rmv.nDots = 100;
         tgParms.rmv.nDotSize = 1;
         tgParms.rmv.iSeed = 0;
         break;
      case RMV_BAR :
         tgParms.rmv.fDriftAxis[0] = 0.0f;
         break;
      case RMV_SPOT :
         break;
      case RMV_GRATING :
      case RMV_PLAID:
         for(i=0; i<2; i++)
         {
            tgParms.rmv.iRGBMean[i] = 0x00808080;
            tgParms.rmv.iRGBCon[i] = 0x00646464;
            tgParms.rmv.fSpatialFreq[i] = 1.0f;
            tgParms.rmv.fGratPhase[i] = 0.0f;
            tgParms.rmv.fDriftAxis[i] = 0.0f;
         }
         break;
      case RMV_MOVIE :
      case RMV_IMAGE :
         ::strcpy_s(tgParms.rmv.strFolder, "folderName");
         ::strcpy_s(tgParms.rmv.strFile, "fileName");
         break;
   }

   // examine name,value array and update target definition accordingly. We don't validate values, only
   // structure. We let SetParams() auto-correct any invalid parameters.
   for(i=0; i<pParams->GetSize(); i+=2)
   {
      CString paramName = pParams->ElementAt(i)->AsString();
      JSONValue* pValue = pParams->ElementAt(i+1);
      
      BOOL ok = FALSE;
      if(paramName.Compare("dotsize") == 0)
      {
         if(iTgtType > RMV_FLOWFIELD) continue;
         ok = pValue->IsNumber();
         if(ok) tgParms.rmv.nDotSize = (int) pValue->AsNumber();
      }
      else if(paramName.Compare("rgb") == 0)
      {
         if(iTgtType > RMV_SPOT) continue;
         ok = pValue->IsNumber();
         if(ok) tgParms.rmv.iRGBMean[0] = 0x00FFFFFF & ((int) pValue->AsNumber());
      }
      else if(paramName.Compare("rgbcon") == 0)
      {
         if(iTgtType != RMV_RANDOMDOTS) continue;
         ok = pValue->IsNumber();
         if(ok) tgParms.rmv.iRGBCon[0] = 0x00FFFFFF & ((int) pValue->AsNumber());
      }
      else if(paramName.Compare("ndots") == 0)
      {
         if(iTgtType != RMV_RANDOMDOTS && iTgtType != RMV_FLOWFIELD) continue;
         ok = pValue->IsNumber();
         if(ok) tgParms.rmv.nDots = (int) pValue->AsNumber();
      }
      else if(paramName.Compare("aperture") == 0)
      { 
         if(!(iTgtType==RMV_RANDOMDOTS || iTgtType==RMV_SPOT || iTgtType==RMV_GRATING || iTgtType==RMV_PLAID))
            continue;
         CString apType = pValue->AsString();
         ok = TRUE;
         if(apType.Compare("rect") == 0) tgParms.rmv.iAperture = RMV_RECT;
         else if(apType.Compare("oval") == 0) tgParms.rmv.iAperture = RMV_OVAL;
         else if(apType.Compare("rectannu") == 0) tgParms.rmv.iAperture = RMV_RECTANNU;
         else if(apType.Compare("ovalannu") == 0) tgParms.rmv.iAperture = RMV_OVALANNU;
         else ok = FALSE;
      }
      else if(paramName.Compare("dim") == 0)
      {
         if(iTgtType == RMV_POINT || iTgtType == RMV_MOVIE || iTgtType == RMV_IMAGE) continue;

         JSONArray* pArVals = pValue->AsArray();
         ok = pArVals != NULL;
         INT_PTR nVals = 0;
         if(ok)
         {
            nVals = pArVals->GetSize();
            ok = (nVals >= 2) && (nVals <= 4);
            for(j=0; ok && j<nVals; j++) ok = pArVals->ElementAt(j)->IsNumber();
         }
         
         if(ok)
         {
            if(iTgtType == RMV_FLOWFIELD)
            {
               tgParms.rmv.fOuterW = (float) pArVals->ElementAt(0)->AsNumber();
               tgParms.rmv.fInnerW = (float) pArVals->ElementAt(1)->AsNumber();
            }
            else
            {
               tgParms.rmv.fOuterW = (float) pArVals->ElementAt(0)->AsNumber();
               tgParms.rmv.fOuterH = (float) pArVals->ElementAt(1)->AsNumber();
               tgParms.rmv.fInnerW = (nVals >= 3) ? ((float) pArVals->ElementAt(2)->AsNumber()) : 0.01f;
               tgParms.rmv.fInnerH = (nVals == 4) ? ((float) pArVals->ElementAt(3)->AsNumber()) : 0.01f;
               if(iTgtType == RMV_BAR)
                  tgParms.rmv.fDriftAxis[0] = (nVals >=3) ? ((float) pArVals->ElementAt(2)->AsNumber()) : 0.0f;
            }
         }
      }
      else if(paramName.Compare("sigma") == 0)
      {
         if(!(iTgtType==RMV_RANDOMDOTS || iTgtType==RMV_SPOT || iTgtType==RMV_GRATING || iTgtType==RMV_PLAID))
            continue;
         
         JSONArray* pArVals = pValue->AsArray();
         ok = pArVals != NULL && (pArVals->GetSize() == 2);
         for(j=0; ok && j<2; j++)
         {
            ok = pArVals->ElementAt(j)->IsNumber();
            if(ok) tgParms.rmv.fSigma[j] = (float) pArVals->ElementAt(j)->AsNumber();
         }
      }
      else if(paramName.Compare("seed") == 0)
      {
         if(iTgtType != RMV_RANDOMDOTS && iTgtType != RMV_FLOWFIELD) continue;
         ok = pValue->IsNumber();
         if(ok) tgParms.rmv.iSeed = (int) pValue->AsNumber();
      }
      else if(paramName.Compare("pct") == 0)
      {
         if(iTgtType != RMV_RANDOMDOTS) continue;
         ok = pValue->IsNumber();
         if(ok) tgParms.rmv.iPctCoherent = (int) pValue->AsNumber();
      }
      else if(paramName.Compare("dotlf") == 0)
      {
         if(iTgtType != RMV_RANDOMDOTS) continue;
         
         JSONArray* pArVals = pValue->AsArray();
         ok = (pArVals != NULL) && (pArVals->GetSize() == 2);
         for(j=0; ok && j<2; j++) ok = pArVals->ElementAt(j)->IsNumber();
         
         if(ok)
         {
            if(pArVals->ElementAt(0)->AsNumber() != 0) tgParms.rmv.iFlags |= RMV_F_LIFEINMS;
            else tgParms.rmv.iFlags &= ~RMV_F_LIFEINMS;
            
            tgParms.rmv.fDotLife = (float) pArVals->ElementAt(1)->AsNumber();
         }
      }
      else if(paramName.Compare("noise") == 0)
      {
         if(iTgtType != RMV_RANDOMDOTS) continue;
         
         JSONArray* pArVals = pValue->AsArray();
         ok = (pArVals != NULL) && (pArVals->GetSize() == 4);
         for(j=0; ok && j<4; j++) ok = pArVals->ElementAt(j)->IsNumber();
         
         if(ok)
         {
            if(pArVals->ElementAt(0)->AsNumber() != 0) tgParms.rmv.iFlags |= RMV_F_DIRNOISE;
            else tgParms.rmv.iFlags &= ~RMV_F_DIRNOISE;
            if(pArVals->ElementAt(1)->AsNumber() != 0) tgParms.rmv.iFlags |= RMV_F_SPDLOG2;
            else tgParms.rmv.iFlags &= ~RMV_F_SPDLOG2;
            
            tgParms.rmv.iNoiseLimit = (int) pArVals->ElementAt(2)->AsNumber();
            tgParms.rmv.iNoiseUpdIntv = (int) pArVals->ElementAt(3)->AsNumber();
         }
      }
      else if(paramName.Compare("wrtscreen") == 0)
      {
         if(iTgtType != RMV_RANDOMDOTS) continue;
         ok = pValue->IsNumber();
         if(ok)
         {
            if(pValue->AsNumber() != 0) tgParms.rmv.iFlags |= RMV_F_WRTSCREEN;
            else tgParms.rmv.iFlags &= ~RMV_F_WRTSCREEN;
         }
      }
      else if(paramName.Compare("square") == 0)
      {
         if(iTgtType != RMV_GRATING && iTgtType != RMV_PLAID) continue;
         ok = pValue->IsNumber();
         if(ok)
         {
            if(pValue->AsNumber() != 0) tgParms.rmv.iFlags |= RMV_F_ISSQUARE;
            else tgParms.rmv.iFlags &= ~RMV_F_ISSQUARE;
         }
      }
      else if(paramName.Compare("oriadj") == 0)
      {
         if(iTgtType != RMV_GRATING && iTgtType != RMV_PLAID) continue;
         ok = pValue->IsNumber();
         if(ok)
         {
            if(pValue->AsNumber() != 0) tgParms.rmv.iFlags |= RMV_F_ORIENTADJ;
            else tgParms.rmv.iFlags &= ~RMV_F_ORIENTADJ;
         }
      }
      else if(paramName.Compare("indep") == 0)
      {
         if(iTgtType != RMV_PLAID) continue;
         ok = pValue->IsNumber();
         if(ok)
         {
            if(pValue->AsNumber() != 0) tgParms.rmv.iFlags |= RMV_F_INDEPGRATS;
            else tgParms.rmv.iFlags &= ~RMV_F_INDEPGRATS;
         }
      }
      else if(paramName.Compare("grat1") == 0 || paramName.Compare("grat2") == 0)
      {
         if(iTgtType != RMV_GRATING && iTgtType != RMV_PLAID) continue;
         if(paramName.Compare("grat2") == 0 && iTgtType == RMV_GRATING) continue;
         
         JSONArray* pArVals = pValue->AsArray();
         ok = (pArVals != NULL) && (pArVals->GetSize() == 5);
         for(j=0; ok && j<5; j++) ok = pArVals->ElementAt(j)->IsNumber();

         int grat = (paramName.Compare("grat1") == 0) ? 0 : 1;
         if(ok)
         {
            tgParms.rmv.iRGBMean[grat] = 0x00FFFFFF & ((int) pArVals->ElementAt(0)->AsNumber());
            tgParms.rmv.iRGBCon[grat] = 0x00FFFFFF & ((int) pArVals->ElementAt(1)->AsNumber());
            tgParms.rmv.fSpatialFreq[grat] = (float) pArVals->ElementAt(2)->AsNumber();
            tgParms.rmv.fGratPhase[grat] = (float) pArVals->ElementAt(3)->AsNumber();
            tgParms.rmv.fDriftAxis[grat] = (float) pArVals->ElementAt(4)->AsNumber();
         }
      }
      else if(paramName.Compare("folder") == 0 || paramName.Compare("file") == 0)
      {
         if(iTgtType != RMV_MOVIE && iTgtType != RMV_IMAGE) continue;
         CString str = pValue->AsString();
         ok = (str.GetLength() > 0) && (str.GetLength() <= RMV_MVF_LEN);
         if(ok)
         {
            if(paramName.Compare("folder") == 0) ::strcpy_s(tgParms.rmv.strFolder, (LPCTSTR) str);
            else ::strcpy_s(tgParms.rmv.strFile, (LPCTSTR) str);
         }
      }
      else if(paramName.Compare("flags") == 0)
      {
         if(iTgtType != RMV_MOVIE) continue;

         JSONArray* pArVals = pValue->AsArray();
         ok = (pArVals != NULL) && (pArVals->GetSize() == 3);
         for(j = 0; ok && j<3; j++) ok = pArVals->ElementAt(j)->IsNumber();

         if(ok)
         {
            if(pArVals->ElementAt(0)->AsNumber() != 0) tgParms.rmv.iFlags |= RMV_F_REPEAT;
            else tgParms.rmv.iFlags &= ~RMV_F_REPEAT;

            if(pArVals->ElementAt(1)->AsNumber() != 0) tgParms.rmv.iFlags |= RMV_F_PAUSEWHENOFF;
            else tgParms.rmv.iFlags &= ~RMV_F_PAUSEWHENOFF;

            if(pArVals->ElementAt(2)->AsNumber() != 0) tgParms.rmv.iFlags |= RMV_F_ATDISPRATE;
            else tgParms.rmv.iFlags &= ~RMV_F_ATDISPRATE;
         }
      }
      else if(paramName.Compare("flicker") == 0)
      {
         JSONArray* pArVals = pValue->AsArray();
         ok = (pArVals != NULL) && (pArVals->GetSize() == 3);
         for(j = 0; ok && j<3; j++) ok = pArVals->ElementAt(j)->IsNumber();

         if(ok)
         {
            tgParms.rmv.iFlickerOn = (int)pArVals->ElementAt(0)->AsNumber();
            tgParms.rmv.iFlickerOff = (int)pArVals->ElementAt(1)->AsNumber();
            tgParms.rmv.iFlickerDelay = (int)pArVals->ElementAt(2)->AsNumber();
         }
      }

      if(!ok)
      {
         errMsg.Format("Cannot import RMVideo target %s in set %s: Bad parameter (%s)", name, setName, paramName);
         return(CX_NULLOBJ_KEY);
      }
   }
   
   BOOL b;
   pTgt->SetParams(&tgParms, b);
   
   return(wKey);
}

/**
 * Helper method for DoImport(). Consumes the "trialSets" field in the JMX document object, importing each trial set
 * defined into the Maestro experiment document (CCxDoc), along with all the trials an trial subsets defined within 
 * that set.
 * 
 * 27sep2024: Any trial that uses the obsolete XYScope platform is skipped over (rather than aborting import).
 *
 * @param pJMX Pointer to the JMX document object (a JSON object).
 * @param pDoc Pointer to the Maestro experiment document.
 * @param errMsg Reference to a CString in which an error description is stored should the import fail.
 * @return TRUE if successful; FALSE otherwise.
 */
BOOL JMXDocImporter::ImportTrialSets(JSONObject* pJMX, CCxDoc* pDoc, CString& errMsg)
{
   JSONValue* pValue;
   BOOL ok = pJMX->Lookup("trialSets", pValue) && pValue->IsArray();
   if(!ok)
   {
      errMsg = "Missing or invalid field in JMX document object: 'trialSets'";
      return(FALSE);
   }
   
   // all imported trial sets are inserted under this object
   WORD wBase = pDoc->GetBaseObj(CX_TRIALBASE);
   ASSERT(wBase != CX_NULLOBJ_KEY);

   // import the trial sets one at a time...
   CMap<CString, LPCTSTR, WORD, WORD> importedSetsMap;
   CMap<CString, LPCTSTR, WORD, WORD> importedKidsMap;
   JSONArray* pJSONTrialSetArray = pValue->AsArray();
   for(int i=0; i<pJSONTrialSetArray->GetSize(); i++)
   {
      JSONObject* pJSONTrialSet = NULL;
      CString setName;
      JSONArray* pJSONKids = NULL;
      WORD wTrialSetKey = CX_NULLOBJ_KEY;
      
      ok = pJSONTrialSetArray->ElementAt(i)->IsObject();
      if(ok)
      {
         pJSONTrialSet = pJSONTrialSetArray->ElementAt(i)->AsObject();
         ok = pJSONTrialSet->Lookup("name", pValue) && pValue->IsString();
         if(ok) setName = pValue->AsString();
         if(ok) ok = pJSONTrialSet->Lookup("trials", pValue) && pValue->IsArray();
         if(ok) pJSONKids = pValue->AsArray();
      }
      if(!ok)
      {
         errMsg.Format("%d-th trial set is invalid in field 'trialSets'", i);
         return(FALSE);
      }
      
      // ignore duplicates
      if(importedSetsMap.Lookup(setName, wTrialSetKey))
         continue;

      // append a new trial set object with the name specified.
      wTrialSetKey = pDoc->InsertObj(wBase, CX_TRIALSET, setName);
      if(wTrialSetKey == CX_NULLOBJ_KEY)
      {
         errMsg.Format("Unable to import %d-th trial set: low memory or document full", i);
         return(FALSE);
      }
      
      // if the trial set name was modified during insertion, fail
      if(pDoc->GetObjName(wTrialSetKey).Compare(setName) != 0)
      {
         errMsg.Format("Invalid name for %d-th trial set: %s", i, setName);
         return(FALSE);
      }

      // remember name of trial set so we don't import another with the same name!
      importedSetsMap.SetAt(setName, wTrialSetKey);
      
      // import all the trials and/or trial subsets in the trial set
      importedKidsMap.RemoveAll();
      for(int iKid = 0; iKid < pJSONKids->GetSize(); iKid++)
      {
         JSONObject* pJSONKid = NULL;
         CString kidName;
         WORD wKidKey = CX_NULLOBJ_KEY;
         BOOL bIsSubset = FALSE;

         ok = pJSONKids->ElementAt(iKid)->IsObject();
         if(ok)
         {
            pJSONKid = pJSONKids->ElementAt(iKid)->AsObject();
            ok = pJSONKid->Lookup("name", pValue) && pValue->IsString();
            if(ok)
            {
               kidName = pValue->AsString();
               bIsSubset = FALSE;
            }
            else
            {
               ok = pJSONKid->Lookup("subset", pValue) && pValue->IsString();
               if(ok)
               {
                  kidName = pValue->AsString();
                  bIsSubset = TRUE;
               }
            }
         }
         if(!ok)
         {
            errMsg.Format("%d-th object in set %s is invalid in field 'trialSets'", iKid, setName);
            return(FALSE);
         }
         
         // ignore duplicates
         if(importedKidsMap.Lookup(kidName, wKidKey))
            continue;

         // import the trial or trial set
         CString strErr;
         if(bIsSubset)
            wKidKey = ImportTrialSubset(pDoc, wTrialSetKey, pJSONKid, strErr);
         else
            wKidKey = ImportTrial(pDoc, wTrialSetKey, pJSONKid, strErr);
         if(wKidKey == CX_NULLOBJ_KEY)
         {
            // skip over trials that aren't imported because they use old XYScope platform
            if(strErr.IsEmpty())
               continue;
            errMsg.Format("Failed to import %s %s in set %s from field 'trialSets': %s", 
               bIsSubset ? "subset" : "trial", kidName, setName, strErr);
            return(FALSE);
         }
         
         // keep track of children added to set so that we ignore duplicate child names
         importedKidsMap.SetAt(kidName, wKidKey);
      }
   }
   
   return(TRUE);
}

/**
 Helper method for ImportTrialSets(). It imports a single JSON object defining a Maestro trial subset into the Maestro
 experiment document (CCxDoc). The JSON object encapsulating a trial subset is very much like that defining a trial
 set, except that the subset name is in a field called 'subset' instead of 'name', and the 'trials' array can contain
 only trial objects, no trial subsets. It is assumed that the caller has already validated the subset's name and 
 verified that it is unique among the children of the parent trial set. The method simply iterates over the trial
 objects in the 'trials' array and imports them into the subset via ImportTrial().
 
 @param pDoc Pointer to the Maestro experiment document.
 @param wSet Key of the parent trial set under which imported subset should be stored.
 @param pJSONSubset The JSON object defining the trial subset, as extracted from the JMX document.
 @param errMsg Reference to a CString in which an error description is stored should the import fail.
 @return The key of the imported trial subset; CX_NULLOBJ_KEY if import fails for whatever reason.
*/
WORD JMXDocImporter::ImportTrialSubset(CCxDoc* pDoc, WORD wSet, JSONObject* pJSONSubset, CString& errMsg)
{
   // verify that JSON subset has two fields 'subset' and 'trials', then append empty trial subset to parent set
   JSONValue* pValue = NULL;
   CString subsetName;
   JSONArray* pJSONTrials = NULL;
   BOOL ok = pJSONSubset->Lookup("subset", pValue) && pValue->IsString();
   if(ok)
   {
      subsetName = pValue->AsString();
      ok = pJSONSubset->Lookup("trials", pValue) && pValue->IsArray();
      if(ok) pJSONTrials = pValue->AsArray();
   }
   if(!ok)
   {
      errMsg = "Missing or invalid field -- 'subset' or 'trials'";
      return(CX_NULLOBJ_KEY);
   }
   
   WORD wSubsetKey = pDoc->InsertObj(wSet, CX_TRIALSUBSET, subsetName);
   if(wSubsetKey == CX_NULLOBJ_KEY)
   {
      errMsg = "Unable to import trial subset: low memory or document full";
      return(CX_NULLOBJ_KEY);
   }
   if(pDoc->GetObjName(wSubsetKey).Compare(subsetName) != 0)
   {
      errMsg.Format("Invalid name for trial subset: %s", subsetName);
      return(CX_NULLOBJ_KEY);
   }

   // import the trials into the subset one at a time...
   CMap<CString, LPCTSTR, WORD, WORD> importedTrialsMap;
   for(int i=0; i<pJSONTrials->GetSize(); i++)
   {
      JSONObject* pJSONTrial = NULL;
      CString trialName;
      WORD wTrialKey = CX_NULLOBJ_KEY;
      
      ok = pJSONTrials->ElementAt(i)->IsObject();
      if(ok)
      {
         pJSONTrial = pJSONTrials->ElementAt(i)->AsObject();
         ok = pJSONTrial->Lookup("name", pValue) && pValue->IsString();
         if(ok) trialName = pValue->AsString();
      }
      if(!ok)
      {
         errMsg.Format("%d-th trial in subset is invalid", i);
         return(CX_NULLOBJ_KEY);
      }
      
      // ignore duplicates
      if(importedTrialsMap.Lookup(trialName, wTrialKey))
         continue;

      // import the trial
      CString strErr;
      wTrialKey = ImportTrial(pDoc, wSubsetKey, pJSONTrial, strErr);
      if(wTrialKey == CX_NULLOBJ_KEY)
      {
         // skip over trials that aren't imported because they use old XYScope platform
         if(strErr.IsEmpty())
            continue;
         errMsg.Format("Failed to import %d-th trial %s in subset: %s", i, trialName, strErr);
         return(CX_NULLOBJ_KEY);
      }
         
      // keep track of trials added to subset so that we ignore duplicate trial names
      importedTrialsMap.SetAt(trialName, wTrialKey);
   }
   
   return(wSubsetKey);
}

/**
 * Helper method for ImportTrialSets(). It imports a single JSON object defining a Maestro trial into the Maestro 
 * experiment document (CCxDoc). The JSON object TRIAL encapsulating a trial's definition is quite complex, with nine
 * different fields.
 * 
 * NOTE: All parameter values that are segment or target indices are zero-based in Maestro and one-based in JMX. Thus,
 * during import, such parameters are decremented by 1!
 *
 * TRIAL.NAME: The trial name. This must be accepted without alteration. It is assumed that caller has already verified
 * that the name supplied is unique among the trials within the parent trial set.
 *
 * TRIAL.PARAMS: General trial parameters -- JSON array holding a sequence of zero or more ('param-name', param-value)
 * pairs. Any parameter not included in the array is set to its default value, as listed below. Recognized parameters:
 * 
 * 'chancfg': Name of the channel configuration applicable to this trial. If specified, it must name an existing 
 *    configuration already imported into the CCxDoc. Default = 'default'. The 'default' configuration always exists.
 * 'wt': Trial weight governing frequency of presentation in a "Randomized" sequence. An integer range-restricted
 *    to [0..255]. Default = 1.
 * 'keep': Save (nonzero) or discard (zero) data recorded during trial. Default = 1.
 * 'startseg': Turn on data recording at the beginning of this segment. Integer in [0..#segs]. If 0, the entire 
 *    trial is recorded. Default = 0.  
 * 'failsafeseg': If trial cut short because subject broke fixation, data is still saved trial reached the start of
 *    this segment. Integer in [0..#segs], where 0 => trial must finish. Default = 0.
 * 'specialop': Special feature. Recognized values: 'none', 'skip', 'selbyfix', 'selbyfix2', 'switchfix', 'rpdistro', 
 *    'choosefix1', 'choosefix2', 'search', 'selectDur'. See Maestro User's Guide for a full description. 
 *    Default = 'none'. 
 * 'specialseg': Index of segment during which special feature operation occurs. Ignored if 'specialop'=='none'.
 *    Integer in [1..#segs]. Default = 1.
 * 'saccvt': Saccade threshold velocity in deg/sec for saccade-triggered special features. Integer range-restricted to 
 *    [0..999]. Default = 100.
 * 'marksegs': Display marker segments, [M1 M2]. If either element is a valid segment index in [1..#segs], a marker is 
 *    drawn in the Maestro data trace window at the starting time for that segment. Default = [0 0].
 * 'mtr': Mid-trial reward feature. Parameter value is a three-element array [M L D], where M = mode (0 = periodic, 
 *    delivered at regular intervals; nonzero = delivered at the end of each segment for which mid-trial rewards are 
 *    enabled), L = reward pulse length in milliseconds (integer, range [1..999], and D = reward pulse interval in 
 *    milliseconds, for periodic mode only (integer, range [100..9999]). Default = [0 10 100].
 * 'xydotseedalt': XYScope is obsolete. This field, if present, is simply ignored (a/o Maestro 5.0).
 * 'xyinterleave': XYScope is obsolete. This field, if present, is simply ignored (a/o Maestro 5.0).
 * 'rewpulses': Lengths of end-of-trial reward pulses, [P1 P2]. The second reward pulse only applies to the special
 *    operations that involve the subject selecting one of two fixation targets. Each pulse length is an integer
 *    range-restricted to [1..999]. Default = [10 10].
 * 'rewWHVR': Random reward withholding variable ratio N/D for the two reward pulses: [N1 D1 N2 D2]. Here, the
 *    numerator N is range-restricted to [0..99] and the denominator to [1..100]. Default = [0 1 0 1], which disables
 *    random reward withholding for both reward pulses.
 * 'stair': Staircase sequencing parameters [N S I]. N = the staircase number, an integer in [0..5], where 0 means
 *    that trial is NOT part of a staircase sequence. S = the staircase strength value assigned to the trial, a 
 *    floating-point value restricted to [0..1000). I = the correct-response input channel (zero = AI12 and nonzero = 
 *    AI13). Default = [0 1.0 0] (NOT a staircase trial).
 *
 * TRIAL.PSGM: Control parameters for a pulse train sequence delivered by Maestro's pulse stimulus generator module
 * (PSGM) during the trial. Value should be an empty array [] if the PSGM is not used in trial. Otherwise, it must be 
 * an 11-element vector [MODE SEG EXTRIG PA1 PA2 PW1 PW2 IPI ITI NP NT], where:
 *
 * MODE = Operational mode. An integer in [0..5], where: 0 => single pulse; 1 => dual pulse (two distinct pulses
 *    separated by the interpulse interval); 2 => biphasic pulse (same as dual pulse, but no interpulse interval);
 *    3 => pulse train (sequence of identical pulses in one or more pulse trains), 4 => biphasic pulse train; 
 *    5 => none (PSGM not used).
 * SEG = Index of trial segment during which PSGM sequence begins. Integer in [1..#segs].
 * EXTRIG: Scalar flag. If nonzero, PSGM sequence does not start until module detects an external trigger pulse
 *    during the designated segment. If zero, PSGM sequence starts at the beginning of that segment.
 * PA1, PA2: Amplitudes of first and second pulses, in millivolts. Each is an integer restricted to [-10240..10160] in 
 *    80mV increments.
 * PW1, PW2: Widths of first and second pulses, in microseconds. Each is an integer restricted to [50..2500] in 10us
 *    increments. 
 * IPI: Interpulse interval in ms. Integer restricted to [1..250] in 1ms increments.
 * ITI: Intertrain interval in ms. Integer restricted to [10..2500] in 10ms increments.
 * NP: #Pulses per train. Integer restricted to [1..250].
 * NT: #Trains in stimulus sequence. Integer restricted to [1..250].
 *
 * TRIAL.PERTS: List of perturbations participating in trial, with control parameters. A JSON array of up to four 
 * elements, each of which is a JSON array defining a trial perturabation {NAME, A, S, T, C}, where:
 *
 * NAME = The name of the perturbation waveform. It must already be imported into the CCxDoc, or the operation fails.
 * A = Perturbation amplitude, range-restricted to +/-999.99.
 * S = Index of segment at which perturbation starts. Must be a valid segment index in [1..#segs].
 * T = Index of affected target. Must be a valid index into the trial's participating target list, [1..#tgts].
 * C = Affected trajectory component. Must be one of: 'winH', 'winV', 'patH', 'patV', 'winDir', 'patDir', 'winSpd',
 *    'patSpd', 'speed', or 'direc'. 
 *
 * TRIAL.TGTS: Trial target list, a NON-EMPTY JSON array of strings, each identifying a target participating in the 
 * trial. The targets will appear in the trial segment table in the order listed. Each entry in the array must have
 * the form 'setName/tgtName', where 'setName' is the name of an EXISTING target set in the CCxDoc and 'tgtName' is
 * the name of an EXISTING target within that set.
 * 
 * There is one parameter-less target in Maestro that may be specified without the containing target set: 'CHAIR'. 
 * As of Maestro 3.0, the parameter-less targets 'FIBER1', 'FIBER2', 'REDLED1', and 'REDLED2' are no longer 
 * supported. If any of these four are encountered, the import fails.
 *
 * TRIAL.TAGS: List of tagged sections in the trial's segment table. A "tagged section" attaches a label to a single
 * segment in the trial, or a contiguous span of segments. It is characterized by a label string and the indices of
 * the first and last segments in the section. TRIAL.TAGS is a JSON array (possibly empty), each element of which is a 
 * JSON array of the form {LABEL, START, END}, where:
 *    LABEL = The section tag. It must contain 1-17 characters. No restriction on character content.
 *    START = Index of the first segment in the section. Must be a valid segment index in [1..#segs].
 *    END = Index of last segment in the section. Must be a valid segment index >= START.
 * No two tagged sections can have the same label, and the defined sections cannot overlap. If either of these rules 
 * are violated, the operation fails.
 *
 * TRIAL.SEGS: The trial's segment table. This is a NON-EMPTY JSON array of JSON objects, one per trial segment. Each
 * JSON object SEG has fields "hdr" and "traj", as described below. 
 *
 * SEG.HDR: The segment's header, which is the list of parameters shown in the top six rows of the segment table in
 * Maestro. The header is specified as a JSON array of zero or more ('param-name', param-value) pairs. Any parameter
 * not listed in the array is set to the default value specified below.
 *    'dur': The segment duration range in milliseconds, [D1 D2], where integers 0 <= D1 <= D2. When D1 < D2, the 
 *       actual segment duration is randomized within the specified range. Default = [1000 1000].
 *    'xyframe': XYScope obsolete. This field, if present, is simply ignored (a/o Maestro 5.0).
 *    'rmvsync': Enable RMVideo VSync spot flash during first video frame at seg start. Nonzero = enable. Default = 0.
 *    'fix1': Zero-based index of the first fixation target for this segment. Must be 0 ("NONE") or a valid index into 
 *       the list of participating targets, [1..#tgts]. Default = 0.
 *    'fix2': Similarly for the second fixation target. Default = 0.
 *    'fixacc': Horizontal and vertical fixation accuracy [H V] in degrees. Each must be >=0.1. Default= [5.0 5.0].
 *    'grace': Grace period in ms for this segment. Integer >= 0. Default = 0.
 *    'mtrena': Enable mid-trial reward feature for this segment? Nonzero = enable. Default = 0 (disabled).
 *    'chkrsp': Enable checking for correct/incorrect response from subject during this segment. Applicable to
 *       staircase trials only. Nonzero = enable. Default = 0 (disabled).
 *    'marker': Digital output channel number for a marker pulse delivered at the start of this segment. Integer value 
 *       restricted to [0..10]; 0 = no marker, 1..10 selects DO1 - DO10, respectively. Default = 0.
 *
 * SEG.TRAJ: A JSON array defining the trajectories of all trial targets during this segment of the trial. Note that 
 * the entry SEG.TRAJ(T) defines the trajectory variables for target T as listed in TRIAL.TGTS. Each entry is a JSON
 * array of ('param-name', param-value) pairs. Any trajectory parameter not included in the array is set to its default
 * value, as specified below.
 *    'on': Target ON (nonzero) or OFF (zero). Default = 0 (OFF).
 *    'abs': Target position specified ABSolutely (nonzero) or RELative to last position (zero). Default = 0 (REL).
 *    'vstab': Target velocity stabilization mode: 'none', 'h', 'v', 'hv'. Default = 'none'.
 *    'snap': Velocity stabilization "snap to eye" flag: Enabled (nonzero) or disabled (0). Default = 0.
 *    'pos': Target horizontal and vertical position in deg, [H V]. Default = [0 0].
 *    'vel': Target window vector velocity specified as [MAG DIR], where MAG is the vector magniture and DIR is the 
 *       vector direction in deg counterclockwise from +X-axis. Note that this representation is different from what 
 *       appears in Maestro -- the horizontal and vertical components of the vector velocity. Maestro handles the 
 *       conversion when the JMX document is imported. If MAG=0, DIR=0. Default = [0 0].
 *    'acc': Target window vector acceleration specified in the same manner as 'vel'. Default = [0 0].
 *    'patvel': Target pattern vector velocity specified in the same manner as 'vel'. Default = [0 0].
 *    'patacc': Target pattern vector acceleration specified in the same manner as 'vel'. Default = [0 0].
 *
 * There are a couple special cases in which the 'patvel' and 'patacc' variables must be treated differently.
 *    (1) If the target is an optic flow field (XYScope or RMVideo), there is no pattern velocity velocity -- there 
 *       is just a "flow velocity" (positive or negative). In this case, DIR is ignored and MAG is taken as the flow 
 *       velocity (for 'patacc', DIR is ignored and MAG is taken as the flow acceleration).
 *    (2) For an RMVideo 'grating' target with the 'oriadj' flag cleared, DIR is ignored and MAG is taken as the
 *       grating's drift velocity (for 'patvel') or drift acceleration (for 'patacc').
 *    (3) For an RMVideo 'plaid' target with the 'indep' flag set, the component gratings move independently, not as
 *       a cohesive pattern. In this case, DIR is taken as the drift velocity (for 'patvel') or acceleration (for 
 *       'patacc') of the first component grating, while MAG is taken as the drift velocity or acceleration of the 
 *       second component.
 *
 * Support for defining and using trial random variables was added in maestrodoc() v1.2.2. There are two additional fields
 * in the JSON object TRIAL (both are optional; if not present, then no RVs are defined in the trial):
 * 
 * TRIAL.RVS: A JSON array of 0 to 10 JSON arrays, where the i-th array defines the i-th random variable, which are
 * labelled "x0" to "x9" in Maestro. Each array must have one of the following forms:
 * 
 *    {'uniform', seed, A, B} : A uniform distribution over the interval [A, B], where A < B.
 *    {'normal', seed, M, D, S} : A normal distribution with mean M, standard deviation D > 0, and a maximum spread
 *       S >= 3*D. During trial sequencing, any time a generated value falls outside [M-S, M+S], that value is rejected 
 *       and another generated in its place.
 *    {'exponential', seed, L, S} : An exponential distribution with rate L > 0 and maximum spread S >= 3/L.
 *    {'gamma', seed, K, T, S} : A gamma distribution with shape parameter K > 0, scale parameter T > 0, and a maximum 
 *       spread S. The mean is KT and variance KT^2. The spread S must be at least 3 standard deviations beyond the 
 *       mean, ie, S >= T*(K + 3*sqrt(K)).
 *    For all of the above distributions, the nonnegative integer seed parameter initializes the random number
 *    generator each time trial sequencing begins. If 0, then a different seed is chosen for each trial sequence.
 *    {'function', formula} : An RV expressed as a function of one or more other RVs. An RV is referenced in the formula
 *       string by its variable name, "x0" to "x9" ("x0" corresponds to the 1st JSON array in TRIAL.RVS, etc). In 
 *       addition to these variables, the formula can contain integer or floating-point numeric constants; the named 
 *       constant "pi"; the four standard arithmetic binary operators -, +, *, /; the unary - operator (as in "-2*x1");
 *       left and right parentheses for grouping; and three named mathematical functions - sin(a), cos(a), and pow(a,b).
 *       Note that the pow() function includes a comma operator to separate its two arguments. Standard operator
 *       precedence rules are observed. It is an ERROR for a function RV to depend on itself, on another function RV, or
 *       on any RV that was not defined in TRIAL.RVs. It is also an ERROR if the formula string is not valid.
 *
 * TRIAL.RVUSE : A JSON array, possibly empty, indicating what trial segment parameters are governed by the trial random
 * variables. Each element of the array must have the form {rvIdx, 'paramName', segIdx, tgIdx}, where rvIdx is the 
 * 1-based index of a random variable defined in TRIAL.RVS, segIdx is the 1-based index of the affected segment, tgIdx
 * is the 1-based index of the affected target trajectory parameter, and 'paramName' identifies the affected parameter:
 *    'mindur', 'maxdur' : Minimum or maximum segment duration. (tgIdx ignored in this case)
 *    'hpos', 'vpos' : Horizontal or vertical target position.
 *    'hvel', 'vvel', 'hacc', 'vacc': Horizontal or vertical target velocity or acceleration.
 *    'hpatvel', 'vpatvel': Horizontal or vertical target pattern valocity.
 *    'hpatacc', 'vpatacc': Horizontal or vertical target pattern acceleration.
 * Note that any defined RV can control the value of more than one segment parameter.
 *
 * 
 * 27sep2024: A JMX document may still contain and use XYScope targets, which have been unsupported since Maestro 4.0
 * and are removed entirely in Maestro 5.0. JMXDocImporter now "skips over" all XYScope targets defined in a JMXDoc,
 * as well as any trials that use an XYScope target.
 *
 * @param pDoc Pointer to the Maestro experiment document.
 * @param wSet Key of the trial set under which imported trial should be stored.
 * @param pJSONTrial The trial definition as extracted from the JMX document.
 * @param errMsg Reference to a CString in which an error description is stored should the import fail.
 * @return The key of the imported trial object; CX_NULLOBJ_KEY if import fails for whatever reason. SPECIAL CASE: If
 * CX_NULLOBJ_KEY is returned by errMsg is an empty string, then the trial was skipped over because it uses an
 * XYScope target. In this case, the import process should continue.
 */
WORD JMXDocImporter::ImportTrial(CCxDoc* pDoc, WORD wSet, JSONObject* pJSONTrial, CString& errMsg)
{
   int i, j, iTgt, iSeg;
   JSONValue* pValue;

   errMsg = _T("");

   // STEP 0: Check for a trial that uses an XYScope target, which we no longer permit. In this case the trial is
   // skipped over but an empty error string is returned -- indicating that the import should continue.
   JSONArray* pArTgts = NULL;
   BOOL ok = pJSONTrial->Lookup("tgts", pValue) && pValue->IsArray();
   if(ok)
   {
      pArTgts = pValue->AsArray();
      ok = pArTgts->GetSize() > 0 && pArTgts->GetSize() <= MAX_TRIALTARGS;
   }
   if(!ok)
   {
      errMsg = "Missing or invalid field 'tgts', or number of targets is invalid";
      return(CX_NULLOBJ_KEY);
   }
   if(!m_xyTgtsSkipped.IsEmpty())
   {
      for(i = 0; i < pArTgts->GetSize(); i++)
      {
         CString tgtPath = pArTgts->ElementAt(i)->AsString();
         WORD wKey;
         if(m_xyTgtsSkipped.Lookup(tgtPath, wKey))
            return(CX_NULLOBJ_KEY);
      }
   }

   // STEP 1: get the trial name and create a trial with that name under the parent set. Abort if unable to create the
   // trial or if the trial's name was modified during insertion.
   CString name;
   if(pJSONTrial->Lookup("name", pValue) && pValue->IsString()) 
      name = pValue->AsString();
   else
   {
      errMsg = "Missing or invalid field -- 'name'";
      return(CX_NULLOBJ_KEY);
   }
   
   WORD wKey = pDoc->InsertObj(wSet, CX_TRIAL, name);
   if(wKey == CX_NULLOBJ_KEY)
   {
      errMsg.Format("Unable to import trial %s -- low memory or document full", name);
      return(CX_NULLOBJ_KEY);
   }

   if(pDoc->GetObjName(wKey).Compare(name) != 0)
   {
      errMsg.Format("Invalid name for trial -- %s", name);
      return(CX_NULLOBJ_KEY);
   }

   CCxTrial* pTrial = (CCxTrial*) pDoc->GetObject(wKey);

   // STEP 2: Insert all participating targets in the order listed in the 'tgts' field of the JMX trial object. Each
   // named target ('setname/tgtname') must have already been imported, or the operation fails. Note that we retrieved
   // the participating target names in STEP 0...
   for(i=0; i<pArTgts->GetSize(); i++)
   {
      CString tgtPath = pArTgts->ElementAt(i)->AsString();
      WORD tgtKey = CX_NULLOBJ_KEY;
      if(tgtPath.Find('/') < 0)
      {
         // as of Maestro 3.0, the only supported "predefined" target is "CHAIR".
         if(tgtPath.Compare("CHAIR") == 0) tgtKey = pDoc->GetChairTarget();
         else 
         {
            errMsg.Format("A target specified in 'tgts' array does not exist: %s", tgtPath);
            return(CX_NULLOBJ_KEY);
         }
      }
      else if(!m_targetsMap.Lookup(tgtPath, tgtKey))
      {
         errMsg.Format("A target specified in 'tgts' array does not exist: %s", tgtPath);
         return(CX_NULLOBJ_KEY);
      }
      if(!pTrial->InsertTarget(-1, tgtKey))
      {
         errMsg.Format("Unexpected error while inserting target: %s", tgtPath);
         return(CX_NULLOBJ_KEY);
      }
   }

   // STEP 3: Insert all segments into trial table in the order listed in the 'segs' field of the JMX trial object.
   JSONArray* pArSegs = NULL;
   ok = pJSONTrial->Lookup("segs", pValue) && pValue->IsArray();
   if(ok)
   {
      pArSegs = pValue->AsArray();
      ok = pArSegs->GetSize() > 0 && pArSegs->GetSize() <= MAX_SEGMENTS;
      for(i=0; ok && i<pArSegs->GetSize(); i++) ok = pArSegs->ElementAt(i)->IsObject();
   }
   if(!ok)
   {
      errMsg = "Missing or invalid field 'segs', or number of segments is invalid";
      return(CX_NULLOBJ_KEY);
   }

   for(iSeg=0; iSeg<pArSegs->GetSize(); iSeg++)
   {
      // insert the segment
      if(pTrial->InsertSeg(-1) < 0)
      {
         errMsg.Format("Unexpected error while appending segment# %d", iSeg);
         return(CX_NULLOBJ_KEY);
      }
      
      // set default segment header and trajectories IAW the defaults assumed by JMX document. These are the same as
      // the defaults assigned when the segment is created, with two exceptions: grace period = 0, and all targets are
      // turned OFF.
      pTrial->SetGracePeriod(iSeg, 0);
      for(iTgt=0; iTgt<pTrial->TargCount(); iTgt++) pTrial->SetTgtOn(iSeg, iTgt, FALSE);
      
      // for each JMX segment object, extract JSON arrays defining segment header and trajectory list and make sure
      // they're reasonable.
      JSONObject* pSegObj = pArSegs->ElementAt(iSeg)->AsObject();
      JSONArray* pArSegHdr = NULL;
      JSONArray* pArTrajs = NULL;

      ok = pSegObj->Lookup("hdr", pValue) && pValue->IsArray();
      if(ok)
      {
         pArSegHdr = pValue->AsArray();
         ok = ((pArSegHdr->GetSize() % 2) == 0);
      }
      if(!ok)
      {
         errMsg.Format("Invalid or missing segment header field: segs(%d).hdr", iSeg);
         return(CX_NULLOBJ_KEY);
      }
      
      ok = pSegObj->Lookup("traj", pValue) && pValue->IsArray();
      if(!ok)
      {
         errMsg.Format("Invalid or missing segment trajectories list: segs(%d).traj", iSeg);
         return(CX_NULLOBJ_KEY);
      }
      else
      {
         pArTrajs = pValue->AsArray();
         if(pArTrajs->GetSize() != pTrial->TargCount())
         {
            errMsg.Format("Length of segment trajectory list != #trial targets: segs(%d).traj", iSeg);
            return(CX_NULLOBJ_KEY);
         }
         for(iTgt=0; iTgt<pArTrajs->GetSize(); iTgt++)
         {
            ok = pArTrajs->ElementAt(iTgt)->IsArray();
            if(ok) ok = ((pArTrajs->ElementAt(iTgt)->AsArray()->GetSize() % 2) == 0);
            if(!ok)
            {
               errMsg.Format("Bad segment trajectory: segs(%d).traj(%d)", iSeg, iTgt);
               return(CX_NULLOBJ_KEY);
            }
         }
      }

      // process JMX segment header in segs(iSeg).hdr
      for(i=0; i<pArSegHdr->GetSize(); i+=2)
      {
         CString paramName = pArSegHdr->ElementAt(i)->AsString();
         JSONValue* pValue = pArSegHdr->ElementAt(i+1);
      
         ok = FALSE;
         if(paramName.Compare("dur") == 0)
         {
            ok = pValue->IsArray() && (pValue->AsArray()->GetSize() == 2) && 
                  pValue->AsArray()->ElementAt(0)->IsNumber() && pValue->AsArray()->ElementAt(1)->IsNumber();
            if(ok)
            {
               int minDur = (int) pValue->AsArray()->ElementAt(0)->AsNumber();
               int maxDur = (int) pValue->AsArray()->ElementAt(1)->AsNumber();
               ok = (0 <= minDur) && (minDur <= maxDur);
               if(ok) 
               {
                  pTrial->SetSegParam(iSeg, -1, CCxTrial::MINDURATION, minDur);
                  pTrial->SetSegParam(iSeg, -1, CCxTrial::MAXDURATION, maxDur);
               }
            }
         }
         else if(paramName.Compare("xyframe") == 0)
         {
            // 27sep2024: XYScope platform dropped a/o Maestro 5. We simply ignore this parameter.
            ok = TRUE;
         }
         else if(paramName.Compare("rmvsync") == 0)
         {
            ok = pValue->IsNumber();
            if(ok) pTrial->SetRMVSyncFlashOn(iSeg, BOOL(pValue->AsNumber() != 0));
         }
         else if(paramName.Compare("fix1") == 0)
         {
            ok = pValue->IsNumber();
            if(ok) pTrial->SetFixTarg1Pos(iSeg, ((int) pValue->AsNumber()) - 1);
         }
         else if(paramName.Compare("fix2") == 0)
         {
            ok = pValue->IsNumber();
            if(ok) pTrial->SetFixTarg2Pos(iSeg, ((int) pValue->AsNumber()) - 1);
         }
         else if(paramName.Compare("fixacc") == 0)
         {
            ok = pValue->IsArray() && (pValue->AsArray()->GetSize() == 2) && 
                  pValue->AsArray()->ElementAt(0)->IsNumber() && pValue->AsArray()->ElementAt(1)->IsNumber();
            if(ok)
            {
               pTrial->SetFixAccH(iSeg, pValue->AsArray()->ElementAt(0)->AsNumber());
               pTrial->SetFixAccV(iSeg, pValue->AsArray()->ElementAt(1)->AsNumber());
            }
         }
         else if(paramName.Compare("grace") == 0)
         {
            ok = pValue->IsNumber();
            if(ok) pTrial->SetGracePeriod(iSeg, (int) pValue->AsNumber());
         }
         else if(paramName.Compare("mtrena") == 0)
         {
            ok = pValue->IsNumber();
            if(ok) pTrial->SetMidTrialRewEnable(iSeg, BOOL(pValue->AsNumber() != 0));
         }
         else if(paramName.Compare("chkrsp") == 0)
         {
            ok = pValue->IsNumber();
            if(ok) pTrial->SetResponseChecked(iSeg, BOOL(pValue->AsNumber() != 0));
         }
         else if(paramName.Compare("marker") == 0)
         {
            ok = pValue->IsNumber();
            if(ok) pTrial->SetMarker(iSeg, (int) pValue->AsNumber());
         }
         
         if(!ok)
         {
            errMsg.Format("Unrecognized param name or bad param value in segs(%d).hdr: %s", iSeg, paramName);
            return(CX_NULLOBJ_KEY);
         }
      }
      
      // process JMX segment trajectories in segs(iSeg).traj(0:nTgts-1)
      for(iTgt = 0; iTgt<pTrial->TargCount(); iTgt++)
      {
         // for certain targets, the 'patvel' and 'patacc' trajectory variables are not interpreted as [MAG DIR]
         BOOL ignoreDir = FALSE;
         BOOL isIndepPlaid = FALSE;
         CCxTarget* pTgt = (CCxTarget*) pDoc->GetObject(pTrial->GetTarget(iTgt));
         ASSERT(pTgt != NULL);
         U_TGPARMS tgParms;
         pTgt->GetParams(&tgParms);
         if(pTgt->DataType() == CX_RMVTARG)
         {
            if(tgParms.rmv.iType == RMV_FLOWFIELD) 
               ignoreDir = TRUE;
            else if(tgParms.rmv.iType == RMV_GRATING && ((tgParms.rmv.iFlags & RMV_F_ORIENTADJ) == 0))
               ignoreDir = TRUE;
            else if(tgParms.rmv.iType == RMV_PLAID && ((tgParms.rmv.iFlags & RMV_F_INDEPGRATS) == RMV_F_INDEPGRATS))
               isIndepPlaid = TRUE;
         }
         
         JSONArray* pTraj = pArTrajs->ElementAt(iTgt)->AsArray();
         for(i=0; i<pTraj->GetSize(); i+=2)
         {
            CString paramName = pTraj->ElementAt(i)->AsString();
            JSONValue* pValue = pTraj->ElementAt(i+1);
      
            ok = FALSE;
            if(paramName.Compare("on") == 0)
            {
               ok = pValue->IsNumber();
               if(ok) pTrial->SetTgtOn(iSeg, iTgt, BOOL(pValue->AsNumber() != 0));
            }
            else if(paramName.Compare("abs") == 0)
            {
               ok = pValue->IsNumber();
               if(ok) pTrial->SetAbsolutePos(iSeg, iTgt, BOOL(pValue->AsNumber() != 0));
            }
            else if(paramName.Compare("vstab") == 0)
            {
               CString vstab = pValue->AsString();
               int mode = -1;
               if(vstab.Compare("none") == 0) mode = SGTJ_VSTABOFF;
               else if(vstab.Compare("h") == 0) mode = SGTJ_VSTABHONLY;
               else if(vstab.Compare("v") == 0) mode = SGTJ_VSTABVONLY;
               else if(vstab.Compare("hv") == 0) mode = SGTJ_VSTABBOTH;

               ok = BOOL(mode > -1);
               if(ok) pTrial->SetTgtVStabMode(iSeg, iTgt, mode);
            }
            else if(paramName.Compare("snap") == 0)
            {
               ok = pValue->IsNumber();
               if(ok) pTrial->SetTgtVStabSnapToEye(iSeg, iTgt, BOOL(pValue->AsNumber() != 0));
            }
            else if(paramName.Compare("pos") == 0)
            {
               ok = pValue->IsArray() && (pValue->AsArray()->GetSize() == 2) && 
                     pValue->AsArray()->ElementAt(0)->IsNumber() && pValue->AsArray()->ElementAt(1)->IsNumber();
               if(ok)
               {
                  pTrial->SetSegParam(iSeg, iTgt, CCxTrial::TGTHPOS, pValue->AsArray()->ElementAt(0)->AsNumber());
                  pTrial->SetSegParam(iSeg, iTgt, CCxTrial::TGTVPOS, pValue->AsArray()->ElementAt(1)->AsNumber());
               }
            }
            else if(paramName.Compare("vel") == 0)
            {
               ok = pValue->IsArray() && (pValue->AsArray()->GetSize() == 2) && 
                     pValue->AsArray()->ElementAt(0)->IsNumber() && pValue->AsArray()->ElementAt(1)->IsNumber();
               if(ok)
               {
                  double mag = pValue->AsArray()->ElementAt(0)->AsNumber();
                  double dir = pValue->AsArray()->ElementAt(1)->AsNumber();
                  pTrial->SetSegParam(iSeg, iTgt, CCxTrial::TGTHVEL, mag*cMath::cosDeg(dir));
                  pTrial->SetSegParam(iSeg, iTgt, CCxTrial::TGTVVEL, mag*cMath::sinDeg(dir));
               }
            }
            else if(paramName.Compare("acc") == 0)
            {
               ok = pValue->IsArray() && (pValue->AsArray()->GetSize() == 2) && 
                     pValue->AsArray()->ElementAt(0)->IsNumber() && pValue->AsArray()->ElementAt(1)->IsNumber();
               if(ok)
               {
                  double mag = pValue->AsArray()->ElementAt(0)->AsNumber();
                  double dir = pValue->AsArray()->ElementAt(1)->AsNumber();
                  pTrial->SetSegParam(iSeg, iTgt, CCxTrial::TGTHACC, mag*cMath::cosDeg(dir));
                  pTrial->SetSegParam(iSeg, iTgt, CCxTrial::TGTVACC, mag*cMath::sinDeg(dir));
               }
            }
            else if(paramName.Compare("patvel") == 0)
            {
               ok = pValue->IsArray() && (pValue->AsArray()->GetSize() == 2) && 
                     pValue->AsArray()->ElementAt(0)->IsNumber() && pValue->AsArray()->ElementAt(1)->IsNumber();
               if(ok)
               {
                  double mag = pValue->AsArray()->ElementAt(0)->AsNumber();
                  double dir = pValue->AsArray()->ElementAt(1)->AsNumber();

                  double h = (ignoreDir || isIndepPlaid) ? mag : mag*cMath::cosDeg(dir);
                  double v = ignoreDir ? 0.0 : (isIndepPlaid ? dir : mag*cMath::sinDeg(dir));

                  pTrial->SetSegParam(iSeg, iTgt, CCxTrial::PATHVEL, h);
                  pTrial->SetSegParam(iSeg, iTgt, CCxTrial::PATVVEL, v);
               }
            }
            else if(paramName.Compare("patacc") == 0)
            {
               ok = pValue->IsArray() && (pValue->AsArray()->GetSize() == 2) && 
                     pValue->AsArray()->ElementAt(0)->IsNumber() && pValue->AsArray()->ElementAt(1)->IsNumber();
               if(ok)
               {
                  double mag = pValue->AsArray()->ElementAt(0)->AsNumber();
                  double dir = pValue->AsArray()->ElementAt(1)->AsNumber();

                  double h = (ignoreDir || isIndepPlaid) ? mag : mag*cMath::cosDeg(dir);
                  double v = ignoreDir ? 0.0 : (isIndepPlaid ? dir : mag*cMath::sinDeg(dir));

                  pTrial->SetSegParam(iSeg, iTgt, CCxTrial::PATHACC, h);
                  pTrial->SetSegParam(iSeg, iTgt, CCxTrial::PATVACC, v);
               }
            }

            if(!ok)
            {
               errMsg.Format("Bad param name or value in segs(%d).traj(%d): %s", iSeg, iTgt, paramName);
               return(CX_NULLOBJ_KEY);
            }
         }
      }
   }

   // STEP 4: Set trial header IAW contents of 'params' and 'psgm' fields in the JSON trial object.
   JSONArray* pArParams = NULL;
   if(pJSONTrial->Lookup("params", pValue) && pValue->IsArray())
   {
      pArParams = pValue->AsArray();
      if(pArParams->GetSize() % 2 != 0)
      {
         errMsg = "Array 'params' does not have an even number of elements";
         return(CX_NULLOBJ_KEY);
      }
   }
   else
   {
      errMsg = "Missing or invalid field -- 'params'";
      return(CX_NULLOBJ_KEY);
   }

   JSONArray* pArPSGM;
   if(pJSONTrial->Lookup("psgm", pValue) && pValue->IsArray()) 
   {
      pArPSGM = pValue->AsArray();
      if(pArPSGM->GetSize() != 0 && pArPSGM->GetSize() != 11)
      {
         errMsg = "Length of 'psgm' is neither 0 nor 11";
         return(CX_NULLOBJ_KEY);
      }
      for(i=0; i<pArPSGM->GetSize(); i++) if(!pArPSGM->ElementAt(i)->IsNumber())
      {
         errMsg = "At least one element of 'psgm' array is not a number";
         return(CX_NULLOBJ_KEY);
      }
   }
   else
   {
      errMsg = "Missing or invalid field -- 'psgm'";
      return(CX_NULLOBJ_KEY);
   }

   // the header parameter defaults are assigned when the trial object is created -- with one exception: we need
   // to set the channel configuration to the predefined default channel configuration object.
   TRLHDR hdr;
   pTrial->GetHeader(hdr);
   hdr.wChanKey = pDoc->GetDefaultChannelConfig();

   // set all header parameters that are explicitly specified in the 'params' array. We only validate the structure
   // of the 'params' array as a sequence of (name,value) pairs and verify that the nature of each value is what is
   // expected for the named parameter. We leave it to CCxTrial::SetHeader() to auto-correct out-of-range values.
   // NOTE: We decrement parameter values that are segment indices to convert from 1-based (JMX) to 0-based (CCxDoc).
   for(i=0; i<pArParams->GetSize(); i+=2)
   {
      CString paramName = pArParams->ElementAt(i)->AsString();
      JSONValue* pValue = pArParams->ElementAt(i+1);
      
      ok = FALSE;
      if(paramName.Compare("chancfg") == 0)
      {
         CString cfgName = pValue->AsString();
         WORD cfgKey = 0;
         ok = m_chanCfgsMap.Lookup(cfgName, cfgKey);
         if(ok) hdr.wChanKey = cfgKey;
         if(!ok)
         {
            errMsg.Format("Could not find imported channel config: %s; key = %d", cfgName, cfgKey);
            return(CX_NULLOBJ_KEY);
         }
      }
      else if(paramName.Compare("wt") == 0)
      {
         ok = pValue->IsNumber();
         if(ok) hdr.iWeight = (int) pValue->AsNumber();
      }
      else if(paramName.Compare("keep") == 0)
      {
         ok = pValue->IsNumber();
         if(ok)
         {
            if(pValue->AsNumber() != 0) hdr.dwFlags |= THF_KEEP;
            else hdr.dwFlags &= ~THF_KEEP;
         }
      }
      else if(paramName.Compare("startseg") == 0)
      {
         ok = pValue->IsNumber();
         if(ok) hdr.iStartSeg = ((int) pValue->AsNumber()) - 1; 
      }
      else if(paramName.Compare("failsafeseg") == 0)
      {
         ok = pValue->IsNumber();
         if(ok) hdr.iFailsafeSeg = ((int) pValue->AsNumber()) - 1; 
      }
      else if(paramName.Compare("specialop") == 0)
      {
         CString opType = pValue->AsString();
         for(j=0; j<TH_NUMSPECOPS; j++) if(opType.Compare(STR_JMXSPECIALOPS[j]) == 0)
         {
            hdr.iSpecialOp = j;
            ok = TRUE;
            break;
         }
      }
      else if(paramName.Compare("specialseg") == 0)
      {
         ok = pValue->IsNumber();
         if(ok) hdr.iSpecialSeg = ((int) pValue->AsNumber()) - 1; 
      }
      else if(paramName.Compare("saccvt") == 0)
      {
         ok = pValue->IsNumber();
         if(ok) hdr.iSaccVt = (int) pValue->AsNumber(); 
      }
      else if(paramName.Compare("marksegs") == 0)
      {
         ok = pValue->IsArray() && (pValue->AsArray()->GetSize() == 2) && 
               pValue->AsArray()->ElementAt(0)->IsNumber() && pValue->AsArray()->ElementAt(1)->IsNumber();
         if(ok)
         {
            hdr.iMarkSeg1 = ((int) pValue->AsArray()->ElementAt(0)->AsNumber()) - 1;
            hdr.iMarkSeg2 = ((int) pValue->AsArray()->ElementAt(1)->AsNumber()) - 1;
         }
      }
      else if(paramName.Compare("mtr") == 0)
      {
         ok = pValue->IsArray() && (pValue->AsArray()->GetSize() == 3) && pValue->AsArray()->ElementAt(0)->IsNumber() &&
               pValue->AsArray()->ElementAt(1)->IsNumber() && pValue->AsArray()->ElementAt(2)->IsNumber();
         if(ok)
         {
            if(pValue->AsArray()->ElementAt(0)->AsNumber() != 0) hdr.dwFlags |= THF_MTRMODE;
            else hdr.dwFlags &= ~THF_MTRMODE;

            hdr.iMTRLen = (int) pValue->AsArray()->ElementAt(1)->AsNumber();
            hdr.iMTRIntv = (int) pValue->AsArray()->ElementAt(2)->AsNumber();
         }
      }
      else if(paramName.Compare("xydotseedalt") == 0)
      {
         // 27sep2024: XYScope platform dropped a/o Maestro 5. We simply ignore this parameter.
         ok = TRUE;
      }
      else if(paramName.Compare("xyinterleave") == 0)
      {
         // 27sep2024: XYScope platform dropped a/o Maestro 5. We simply ignore this parameter.
         ok = TRUE;
      }
      else if(paramName.Compare("rewpulses") == 0)
      {
         ok = pValue->IsArray() && (pValue->AsArray()->GetSize() == 2) && 
               pValue->AsArray()->ElementAt(0)->IsNumber() && pValue->AsArray()->ElementAt(1)->IsNumber();
         if(ok)
         {
            hdr.reward1[0] = (int) (pValue->AsArray()->ElementAt(0)->AsNumber());
            hdr.reward2[0] = (int) (pValue->AsArray()->ElementAt(1)->AsNumber());
         }
      }
      else if(paramName.Compare("rewWHVR") == 0)
      {
         ok = pValue->IsArray() && (pValue->AsArray()->GetSize() == 4);
         for(int k = 0; ok && k < 4; k++) ok = pValue->AsArray()->ElementAt(k)->IsNumber();
         if(ok)
         {
            hdr.reward1[1] = (int)(pValue->AsArray()->ElementAt(0)->AsNumber());
            hdr.reward1[2] = (int)(pValue->AsArray()->ElementAt(1)->AsNumber());
            hdr.reward2[1] = (int)(pValue->AsArray()->ElementAt(2)->AsNumber());
            hdr.reward2[2] = (int)(pValue->AsArray()->ElementAt(3)->AsNumber());
         }
      }
      else if(paramName.Compare("stair") == 0)
      {
         ok = pValue->IsArray() && (pValue->AsArray()->GetSize() == 3) && pValue->AsArray()->ElementAt(0)->IsNumber() &&
               pValue->AsArray()->ElementAt(1)->IsNumber() && pValue->AsArray()->ElementAt(2)->IsNumber();
         if(ok)
         {
            hdr.iStairNum = (int) pValue->AsArray()->ElementAt(0)->AsNumber();
            hdr.fStairStrength = (float) pValue->AsArray()->ElementAt(1)->AsNumber();
            
            if(pValue->AsArray()->ElementAt(2)->AsNumber() != 0) hdr.dwFlags |= THF_STAIRRESP;
            else hdr.dwFlags &= ~THF_STAIRRESP;
         }
      }

      if(!ok)
      {
         errMsg.Format("Unrecognized header parameter or bad parameter value: %s", paramName);
         return(CX_NULLOBJ_KEY);
      }
   }
   
   // set PSGM parameters IAW contents of 'psgm' array = [MODE SEG EXTRIG PA1 PA2 PW1 PW2 IPI ITI NP NT]. If array is
   // empty, then PSGM is not used, which is how the default header was set up.
   if(pArPSGM->GetSize() > 0)
   {
      hdr.sgm.iOpMode = (int) pArPSGM->ElementAt(0)->AsNumber();
      hdr.iSGMSeg = ((int) pArPSGM->ElementAt(1)->AsNumber()) - 1;
      hdr.sgm.bExtTrig = BOOL( pArPSGM->ElementAt(2)->AsNumber() != 0);
      hdr.sgm.iAmp1 = (int) pArPSGM->ElementAt(3)->AsNumber();
      hdr.sgm.iAmp2 = (int) pArPSGM->ElementAt(4)->AsNumber();
      hdr.sgm.iPW1 = (int) pArPSGM->ElementAt(5)->AsNumber();
      hdr.sgm.iPW2 = (int) pArPSGM->ElementAt(6)->AsNumber();
      hdr.sgm.iPulseIntv = (int) pArPSGM->ElementAt(7)->AsNumber();
      hdr.sgm.iTrainIntv = (int) pArPSGM->ElementAt(8)->AsNumber();
      hdr.sgm.nPulses = (int) pArPSGM->ElementAt(9)->AsNumber();
      hdr.sgm.nTrains = (int) pArPSGM->ElementAt(10)->AsNumber();
   }
   
   // update trial's header parameters. NOTE that we allow auto-corrections -- the assumption being that the JMX trial
   // parameters are all valid.
   BOOL b;
   pTrial->SetHeader(hdr, b);
   
   
   // STEP 5: Set trial's perturbations IAW contents of the 'perts' field in the JMX trial object.
   JSONArray* pArPerts;
   if(pJSONTrial->Lookup("perts", pValue) && pValue->IsArray())
   {
      pArPerts = pValue->AsArray();
      if(pArPerts->GetSize() > MAX_TRIALPERTS)
      {
         errMsg = "Too many entries in array 'perts'";
         return(CX_NULLOBJ_KEY);
      }
   }
   else
   {
      errMsg = "Missing or invalid field -- 'perts'";
      return(CX_NULLOBJ_KEY);
   }

   for(i=0; i<pArPerts->GetSize(); i++)
   {
      JSONArray* pPert = NULL;
      ok = pArPerts->ElementAt(i)->IsArray();
      if(ok) pPert = pArPerts->ElementAt(i)->AsArray();
      if(ok) ok = (pPert->GetSize() == 5) && pPert->ElementAt(0)->IsString() && pPert->ElementAt(1)->IsNumber() &&
         pPert->ElementAt(2)->IsNumber() && pPert->ElementAt(3)->IsNumber() && pPert->ElementAt(4)->IsString();
      if(!ok)
      {
         errMsg.Format("Invalid entry at index %d in 'perts' field", i);
         return(CX_NULLOBJ_KEY);
      }
      
      CString pertName = pPert->ElementAt(0)->AsString();
      WORD pertKey;
      if(!m_pertsMap.Lookup(pertName, pertKey))
      {
         errMsg.Format("Perturbation object (%s) in perts(%d) has not been imported", pertName, i);
         return(CX_NULLOBJ_KEY);
      }
      
      float fAmp = (float) pPert->ElementAt(1)->AsNumber();
      int affectedSeg = ((int) pPert->ElementAt(2)->AsNumber()) - 1;
      int affectedTgt = ((int) pPert->ElementAt(3)->AsNumber()) - 1;
      
      CString pertCmpt = pPert->ElementAt(4)->AsString();
      int idCmpt = -1;
      for(j=0; j<PERT_NCMPTS; j++) if(pertCmpt.Compare(STR_JMXPERTCMPTS[j]) == 0)
      {
         idCmpt = j;
         break;
      }
      if(idCmpt < 0)
      {
         errMsg.Format("Invalid perturbation component (%s) in perts(%d)", pertCmpt, i);
         return(CX_NULLOBJ_KEY);
      }
      
      if(!(pTrial->AppendPert(pertKey) && pTrial->SetPert(i, pertKey, fAmp, affectedSeg, affectedTgt, idCmpt)))
      {
         errMsg.Format("Failed to append trial perturbation in perts(%d)", i);
         return(CX_NULLOBJ_KEY);
      }
   }

   // STEP 6: Define any tagged sections IAW content of the 'tags' field in the JMX trial object.
   JSONArray* pArTagSects;
   if(!(pJSONTrial->Lookup("tags", pValue) && pValue->IsArray()))
   {
      errMsg = "Missing or invalid field -- 'tags'";
      return(CX_NULLOBJ_KEY);
   }
   pArTagSects = pValue->AsArray();
   
   for(i=0; i<pArTagSects->GetSize(); i++)
   {
      JSONArray* pSect = pArTagSects->ElementAt(i)->AsArray();
      ok = (pSect != NULL) && (pSect->GetSize() == 3) && pSect->ElementAt(0)->IsString() && 
         pSect->ElementAt(1)->IsNumber() && pSect->ElementAt(2)->IsNumber();
      if(ok)
      {
         int start = ((int) pSect->ElementAt(1)->AsNumber()) - 1;
         int end = ((int) pSect->ElementAt(2)->AsNumber()) - 1;
         ok = pTrial->CreateTaggedSection(start, end, pSect->ElementAt(0)->AsString());
      }
      
      if(!ok)
      {
         errMsg.Format("Found invalid tagged section definition in tags(%d)", i);
         return(CX_NULLOBJ_KEY);
      }
   }

   // STEP 7: Define any random variables IAW content of the OPTIONAL 'rvs' field in the JMX trial object.
   JSONArray* pArRandomVars = NULL;
   int nRVs = 0;
   if(pJSONTrial->Lookup("rvs", pValue) && pValue->IsArray())
   {
      pArRandomVars = pValue->AsArray();
      nRVs = (int) pArRandomVars->GetSize();
   }
   if(nRVs > 10)
   {
      errMsg = "Field 'rvs' -- More than 10 random variables defined in field";
      return(CX_NULLOBJ_KEY);
   }
   else if(nRVs > 0)
   {
      for(i = 0; i < nRVs; i++)
      {
         JSONArray* pRV = pArRandomVars->ElementAt(i)->IsArray() ? pArRandomVars->ElementAt(i)->AsArray() : NULL;
         ok = (pRV != NULL) && (2 <= pRV->GetSize()) && (pRV->GetSize() <= 5) && pRV->ElementAt(0)->IsString();
         if(ok) 
         {
            CCxTrial::CRVEntry rvEntry{};
            CString rvType = pRV->ElementAt(0)->AsString();
            if(rvType.Compare("uniform") == 0 || rvType.Compare("exponential") == 0)
            {
               ok = ((pRV->GetSize() == 4) && pRV->ElementAt(1)->IsNumber() && pRV->ElementAt(2)->IsNumber() &&
                  pRV->ElementAt(3)->IsNumber());
               if(ok)
               {
                  rvEntry.iType = (rvType.Compare("uniform") == 0) ? RV_UNIFORM : RV_EXPON;
                  rvEntry.iSeed = (int)pRV->ElementAt(1)->AsNumber();
                  rvEntry.dParams[0] = pRV->ElementAt(2)->AsNumber();
                  rvEntry.dParams[1] = pRV->ElementAt(3)->AsNumber();
               }
            }
            else if(rvType.Compare("normal") == 0 || rvType.Compare("gamma") == 0)
            {
               ok = ((pRV->GetSize() == 5) && pRV->ElementAt(1)->IsNumber() && pRV->ElementAt(2)->IsNumber() &&
                  pRV->ElementAt(3)->IsNumber() && pRV->ElementAt(4)->IsNumber());
               if(ok)
               {
                  rvEntry.iType = (rvType.Compare("normal") == 0) ? RV_NORMAL : RV_GAMMA;
                  rvEntry.iSeed = (int)pRV->ElementAt(1)->AsNumber();
                  rvEntry.dParams[0] = pRV->ElementAt(2)->AsNumber();
                  rvEntry.dParams[1] = pRV->ElementAt(3)->AsNumber();
                  rvEntry.dParams[2] = pRV->ElementAt(4)->AsNumber();
               }
            }
            else if(rvType.Compare("function") == 0)
            {
               ok = ((pRV->GetSize() == 2) && pRV->ElementAt(1)->IsString());
               if(ok)
               {
                  rvEntry.iType = RV_FUNCTION;
                  rvEntry.strFunc = pRV->ElementAt(1)->AsString();
               }
            }
            else
               ok = FALSE;

            // checks validity of RV parameters. When setting the last RV, also verifies that no defined
            // function RV depends on another function RV or an unused RV.
            if(ok) ok = pTrial->SetRV(i, rvEntry, BOOL(i == nRVs - 1));
         }
         if(!ok)
         {
            errMsg.Format("Field 'rvs' -- Invalid random variable definition at index %d", i + 1);
            return(CX_NULLOBJ_KEY);
         }
      }
   }

   // STEP 8: Apply random variable to parameters in trial segment table IAW content of OPTIONAL 'rvuse' field.
   JSONArray* pArRVUses = (pJSONTrial->Lookup("rvuse", pValue) && pValue->IsArray()) ? pValue->AsArray() : NULL;
   if((pArRVUses != NULL) && (pArRVUses->GetSize() > 0))
   {
      if(nRVs == 0)
      {
         errMsg = "Field 'rvuse' not empty, but no random variables defined in 'rv'!";
         return(CX_NULLOBJ_KEY);
      }
      
      for(i = 0; i < pArRVUses->GetSize(); i++)
      {
         JSONArray* pUsage = pArRVUses->ElementAt(i)->IsArray() ? pArRVUses->ElementAt(i)->AsArray() : NULL;
         ok = (pUsage != NULL) && (pUsage->GetSize() == 4) && pUsage->ElementAt(0)->IsNumber() && 
            pUsage->ElementAt(1)->IsString() && pUsage->ElementAt(2)->IsNumber() && pUsage->ElementAt(3)->IsNumber();
         if(ok)
         {
            int rvIdx = ((int)pUsage->ElementAt(0)->AsNumber()) - 1;   // in JMX doc, RV/seg/tgt indices start at 1!!
            int segIdx = ((int)pUsage->ElementAt(2)->AsNumber()) - 1;
            int tgtIdx = ((int)pUsage->ElementAt(3)->AsNumber()) - 1;

            // map param name string to param ID
            CCxTrial::ParamID paramID = CCxTrial::NOTAPARAM;
            CString paramName = pUsage->ElementAt(1)->AsString();
            if(paramName.Compare("mindur") == 0) paramID = CCxTrial::MINDURATION;
            else if(paramName.Compare("maxdur") == 0) paramID = CCxTrial::MAXDURATION;
            else if(paramName.Compare("hpos") == 0) paramID = CCxTrial::TGTHPOS; 
            else if(paramName.Compare("vpos") == 0) paramID = CCxTrial::TGTVPOS; 
            else if(paramName.Compare("hvel") == 0) paramID = CCxTrial::TGTHVEL;
            else if(paramName.Compare("vvel") == 0) paramID = CCxTrial::TGTVVEL;
            else if(paramName.Compare("hacc") == 0) paramID = CCxTrial::TGTHACC;
            else if(paramName.Compare("vacc") == 0) paramID = CCxTrial::TGTVACC;
            else if(paramName.Compare("hpatvel") == 0) paramID = CCxTrial::PATHVEL;
            else if(paramName.Compare("vpatvel") == 0) paramID = CCxTrial::PATVVEL;
            else if(paramName.Compare("hpatacc") == 0) paramID = CCxTrial::PATHACC;
            else if(paramName.Compare("vpatacc") == 0) paramID = CCxTrial::PATVACC;

            ok = (0 <= rvIdx) && (rvIdx < nRVs) && (paramID != CCxTrial::NOTAPARAM);
            if(ok) ok = pTrial->SetSegParam(segIdx, tgtIdx, paramID, rvIdx, TRUE);
         }
         if(!ok)
         {
            errMsg.Format("Field rvuse: Bad RV assignment at index %d", i + 1);
            return(CX_NULLOBJ_KEY);
         }
      }
   }

   // success!
   return(wKey);
}


/** Maps JMX channel ID token to corresponding index position in CCxChannel. */
LPCTSTR JMXDocImporter::STR_JMXCHANNELIDS[] =  
{
   "hgpos", "vepos", "hevel", "vevel", "htpos", "vtpos", "hhvel", "hhpos", 
   "hdvel", "htpos2", "vtpos2", "vepos2", "ai12", "ai13", "hgpos2", "spwav",
   "fix1_hvel", "fix1_vvel", "fix2_hvel", "fix2_vvel", "fix1_hpos", "fix1_vpos",
   "di0", "di1", "di2", "di3", "di4", "di5", "di6", "di7", "di8", "di9", "di10", "di11", "di12", "di13", "di14", "di15"
};

/** Maps JMX trace color name token to corresponding integer index in CCxChannel. */
LPCTSTR JMXDocImporter::STR_JMXTRACECOLORNAMES[] =  
{
   "white", "red", "green", "blue", "yellow", "magenta", "cyan", "dk green", "orange", "purple", "pink", "med gray"
};

/** Maps JMX RMVideo target type token to corresponding integer type (0-based index) as required by CCxTarget. */
LPCTSTR JMXDocImporter::STR_JMXTGTTYPES_RMV[] =
{
   "point", "dotpatch", "flowfield", "bar", "spot", "grating", "plaid", "movie", "image"
};

/** Maps JMX trial special operation token to corresponding integer type (0-based index) as required by CCxTrial. */
LPCTSTR JMXDocImporter::STR_JMXSPECIALOPS[] =
{
   "none", "skip", "selbyfix", "selbyfix2", "switchfix", "rpdistro", "choosefix1", "choosefix2", "search", "selectDur"
};

/** Maps JMX trial perturbation component token to corresponding integer type (0-based index) required by CCxTrial. */
LPCTSTR JMXDocImporter::STR_JMXPERTCMPTS[] =
{
   "winH", "winV", "patH", "patV", "winDir", "patDir", "winSpd", "patSpd", "direc", "speed"
};
