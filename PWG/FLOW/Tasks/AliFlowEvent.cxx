/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/*****************************************************************
  AliFlowEvent: Event container for flow analysis

  origin:   Mikolaj Krzewicki  (mikolaj.krzewicki@cern.ch)
  mods:     Redmer A. Bertens (rbertens@cern.ch)
*****************************************************************/

#include "Riostream.h"
#include "TFile.h"
#include "TList.h"
#include "TH1.h"
#include "TH2F.h"
#include "TProfile.h"
#include "AliMCEvent.h"
#include "AliMCParticle.h"
#include "AliCFManager.h"
#include "AliESDtrack.h"
#include "AliESDPmdTrack.h"
#include "AliESDEvent.h"
#include "AliAODEvent.h"
#include "AliOADBContainer.h"
#include "AliGenCocktailEventHeader.h"
#include "AliGenEposEventHeader.h"
#include "AliGenHijingEventHeader.h"
#include "AliGenGeVSimEventHeader.h"
#include "AliCollisionGeometry.h"
#include "AliMultiplicity.h"
#include "AliFlowTrackCuts.h"
#include "AliFlowEventSimple.h"
#include "AliFlowTrack.h"
#include "AliFlowVector.h"
#include "AliFlowEvent.h"
#include "AliLog.h"

using std::endl;
using std::cout;
ClassImp(AliFlowEvent)

//-----------------------------------------------------------------------
AliFlowEvent::AliFlowEvent():
  AliFlowEventSimple(), fApplyRecentering(kFALSE), fCachedRun(-1), fCurrentCentrality(-1)
{
    // constructor
    for(Int_t i(0); i < 9; i++) {
        for(Int_t j(0); j < 2; j++) {
            for(Int_t k(0); k < 2; k++) {
                fMeanQ[i][j][k] = 0.; 
                fWidthQ[i][j][k] = 0.;  
                fMeanQv3[i][j][k] = 0.; 
                fWidthQv3[i][j][k] = 0.;
            }
        }
    }

  //ctor
  cout << "AliFlowEvent: Default constructor to be used only by root for io" << endl;
}

//-----------------------------------------------------------------------
AliFlowEvent::AliFlowEvent(Int_t n):
  AliFlowEventSimple(n), fApplyRecentering(kFALSE), fCachedRun(-1), fCurrentCentrality(-1)
{
    // constructor
    for(Int_t i(0); i < 9; i++) {
        for(Int_t j(0); j < 2; j++) {
            for(Int_t k(0); k < 2; k++) {
                fMeanQ[i][j][k] = 0.; 
                fWidthQ[i][j][k] = 0.;  
                fMeanQv3[i][j][k] = 0.; 
                fWidthQv3[i][j][k] = 0.;
            }
        }
    }
}

//-----------------------------------------------------------------------
AliFlowEvent::AliFlowEvent(const AliFlowEvent& event):
  AliFlowEventSimple(event), fApplyRecentering(event.fApplyRecentering), fCachedRun(-1), fCurrentCentrality(-1)
{
    // copy constructor 
    for(Int_t i(0); i < 9; i++) {
        for(Int_t j(0); j < 2; j++) {
            for(Int_t k(0); k < 2; k++) {
                fMeanQ[i][j][k] = 0.; 
                fWidthQ[i][j][k] = 0.;  
                fMeanQv3[i][j][k] = 0.; 
                fWidthQv3[i][j][k] = 0.;
            }
        }
    }
}

//-----------------------------------------------------------------------
AliFlowEvent& AliFlowEvent::operator=(const AliFlowEvent& event)
{

  //assignment operator
  if (&event==this) return *this;       // check self-assignment

  fApplyRecentering = event.fApplyRecentering; 
  fCachedRun = event.fCachedRun; 
  fCurrentCentrality = event.fCurrentCentrality;
  for(Int_t i(0); i < 9; i++) {
      for(Int_t j(0); j < 2; j++) {
          for(Int_t k(0); k < 2; k++) {
              fMeanQ[i][j][k] = event.fMeanQ[i][j][k]; 
              fWidthQ[i][j][k] = fWidthQ[i][j][k];  
              fMeanQv3[i][j][k] = fMeanQv3[i][j][k]; 
              fWidthQv3[i][j][k] = fWidthQv3[i][j][k];
          }
      }
  }
  AliFlowEventSimple::operator=(event);
  return *this;
}

//-----------------------------------------------------------------------
AliFlowTrack* AliFlowEvent::GetTrack(Int_t i)
{
  //get track i from collection
  if (i>=fNumberOfTracks) return NULL;
  AliFlowTrack* pTrack = static_cast<AliFlowTrack*>(fTrackCollection->At(i)) ;
  return pTrack;
}

//-----------------------------------------------------------------------
void AliFlowEvent::SetMCReactionPlaneAngle(const AliMCEvent* mcEvent)
{
  //sets the event plane angle from the proper header in the MC

  //COCKTAIL with HIJING
  if (!strcmp(mcEvent-> GenEventHeader()->GetName(),"Cocktail Header"))   //returns 0 if matches
  {
    AliGenCocktailEventHeader *headerC = dynamic_cast<AliGenCocktailEventHeader *> (mcEvent-> GenEventHeader());
    if (headerC)
    {
      TList *lhd = headerC->GetHeaders();
      if (lhd)
      {
        AliGenHijingEventHeader *hdh = dynamic_cast<AliGenHijingEventHeader *> (lhd->At(0));
        if (hdh) AliFlowEventSimple::SetMCReactionPlaneAngle( hdh->ReactionPlaneAngle() );
      }
    }
  }
  //THERMINATOR
  else if (!strcmp(mcEvent-> GenEventHeader()->GetName(),"Therminator"))   //returns 0 if matches
  {
    AliGenHijingEventHeader* headerH = dynamic_cast<AliGenHijingEventHeader*>(mcEvent->GenEventHeader());
    if (headerH) AliFlowEventSimple::SetMCReactionPlaneAngle( headerH->ReactionPlaneAngle() );
  }
  //GEVSIM
  else if (!strcmp(mcEvent-> GenEventHeader()->GetName(),"GeVSim header"))   //returns 0 if matches
  {
    AliGenGeVSimEventHeader* headerG = dynamic_cast<AliGenGeVSimEventHeader*>(mcEvent->GenEventHeader());
    if (headerG) AliFlowEventSimple::SetMCReactionPlaneAngle( headerG->GetEventPlane() );
  }
  //HIJING
  else if (!strcmp(mcEvent-> GenEventHeader()->GetName(),"Hijing"))   //returns 0 if matches
  {
    AliGenHijingEventHeader* headerH = dynamic_cast<AliGenHijingEventHeader*>(mcEvent->GenEventHeader());
    if (headerH) AliFlowEventSimple::SetMCReactionPlaneAngle( headerH->ReactionPlaneAngle() );
  }
  //AMPT
  else if (!strcmp(mcEvent-> GenEventHeader()->GetName(),"Ampt"))   //returns 0 if matches
  {
    AliGenHijingEventHeader* headerH = dynamic_cast<AliGenHijingEventHeader*>(mcEvent->GenEventHeader());
    if (headerH) AliFlowEventSimple::SetMCReactionPlaneAngle( headerH->ReactionPlaneAngle() );
  }
  //EPOS
  else if (!strcmp(mcEvent->GenEventHeader()->GetName(),"EPOS"))
  {
    AliGenEposEventHeader* headerE = dynamic_cast<AliGenEposEventHeader*>(mcEvent->GenEventHeader());
    if (headerE) AliFlowEventSimple::SetMCReactionPlaneAngle( headerE->ReactionPlaneAngle() );
  }
  //Hydjet
  else
  {
    AliCollisionGeometry* header = dynamic_cast<AliCollisionGeometry*>(mcEvent->GenEventHeader());
    if (header) AliFlowEventSimple::SetMCReactionPlaneAngle( header->ReactionPlaneAngle() );
  }
}

//-----------------------------------------------------------------------
AliFlowEvent::AliFlowEvent( const AliMCEvent* anInput,
                            const AliCFManager* rpCFManager,
                            const AliCFManager* poiCFManager):
  AliFlowEventSimple(20), fApplyRecentering(kFALSE), fCachedRun(-1), fCurrentCentrality(-1)
{
    // constructor
    for(Int_t i(0); i < 9; i++) {
        for(Int_t j(0); j < 2; j++) {
            for(Int_t k(0); k < 2; k++) {
                fMeanQ[i][j][k] = 0.; 
                fWidthQ[i][j][k] = 0.;  
                fMeanQv3[i][j][k] = 0.; 
                fWidthQv3[i][j][k] = 0.;
            }
        }
    }

  //Fills the event from the MC kinematic information

  Int_t iNumberOfInputTracks = anInput->GetNumberOfTracks() ;

  //loop over tracks
  for (Int_t itrkN=0; itrkN<iNumberOfInputTracks; itrkN++)
  {
    //get input particle
    AliMCParticle* pParticle = dynamic_cast<AliMCParticle*>(anInput->GetTrack(itrkN));
    if (!pParticle) continue;

    //check if pParticle passes the cuts
    Bool_t rpOK = kTRUE;
    Bool_t poiOK = kTRUE;
    if (rpCFManager && poiCFManager)
    {
      rpOK = rpCFManager->CheckParticleCuts(AliCFManager::kPartGenCuts,pParticle);
      poiOK = poiCFManager->CheckParticleCuts(AliCFManager::kPartGenCuts,pParticle);
    }
    if (!(rpOK||poiOK)) continue;

    AliFlowTrack* pTrack = new AliFlowTrack(pParticle);
    pTrack->SetSource(AliFlowTrack::kFromMC);

    if (rpOK && rpCFManager)
    {
      pTrack->SetForRPSelection(kTRUE);
      fNumberOfRPs++;
    }
    if (poiOK && poiCFManager)
    {
      pTrack->SetForPOISelection(kTRUE);
      fNumberOfPOIs++;
    }

    AddTrack(pTrack) ;
  }//for all tracks
  SetMCReactionPlaneAngle(anInput);
}

//-----------------------------------------------------------------------
AliFlowEvent::AliFlowEvent( const AliESDEvent* anInput,
                            const AliCFManager* rpCFManager,
                            const AliCFManager* poiCFManager ):
  AliFlowEventSimple(20),  fApplyRecentering(kFALSE), fCachedRun(-1), fCurrentCentrality(-1)
{
    // constructor
    for(Int_t i(0); i < 9; i++) {
        for(Int_t j(0); j < 2; j++) {
            for(Int_t k(0); k < 2; k++) {
                fMeanQ[i][j][k] = 0.; 
                fWidthQ[i][j][k] = 0.;  
                fMeanQv3[i][j][k] = 0.; 
                fWidthQv3[i][j][k] = 0.;
            }
        }
    }
   
  //Fills the event from the ESD

  Int_t iNumberOfInputTracks = anInput->GetNumberOfTracks() ;

  //loop over tracks
  for (Int_t itrkN=0; itrkN<iNumberOfInputTracks; itrkN++)
  {
    AliESDtrack* pParticle = anInput->GetTrack(itrkN);   //get input particle

    //check if pParticle passes the cuts
    Bool_t rpOK = kTRUE;
    Bool_t poiOK = kTRUE;
    if (rpCFManager && poiCFManager)
    {
      rpOK = ( rpCFManager->CheckParticleCuts(AliCFManager::kPartRecCuts,pParticle) &&
               rpCFManager->CheckParticleCuts(AliCFManager::kPartSelCuts,pParticle));
      poiOK = ( poiCFManager->CheckParticleCuts(AliCFManager::kPartRecCuts,pParticle) &&
                poiCFManager->CheckParticleCuts(AliCFManager::kPartSelCuts,pParticle));
    }
    if (!(rpOK || poiOK)) continue;

    //make new AliFLowTrack
    AliFlowTrack* pTrack = new AliFlowTrack(pParticle);
    pTrack->SetSource(AliFlowTrack::kFromESD);

    //marking the particles used for int. flow:
    if(rpOK && rpCFManager)
    {
      pTrack->SetForRPSelection(kTRUE);
      fNumberOfRPs++;
    }
    //marking the particles used for diff. flow:
    if(poiOK && poiCFManager)
    {
      pTrack->SetForPOISelection(kTRUE);
      fNumberOfPOIs++;
    }

    AddTrack(pTrack);
  }//end of while (itrkN < iNumberOfInputTracks)
}

//-----------------------------------------------------------------------
AliFlowEvent::AliFlowEvent( const AliAODEvent* anInput,
                            const AliCFManager* rpCFManager,
                            const AliCFManager* poiCFManager):
  AliFlowEventSimple(20), fApplyRecentering(kFALSE), fCachedRun(-1), fCurrentCentrality(-1)
{
    // constructor
    for(Int_t i(0); i < 9; i++) {
        for(Int_t j(0); j < 2; j++) {
            for(Int_t k(0); k < 2; k++) {
                fMeanQ[i][j][k] = 0.; 
                fWidthQ[i][j][k] = 0.;  
                fMeanQv3[i][j][k] = 0.; 
                fWidthQv3[i][j][k] = 0.;
            }
        }
    }

  //Fills the event from the AOD
  Int_t iNumberOfInputTracks = anInput->GetNumberOfTracks() ;

  //loop over tracks
  for (Int_t itrkN=0; itrkN<iNumberOfInputTracks; itrkN++)
  {
    AliAODTrack* pParticle = anInput->GetTrack(itrkN);   //get input particle

    //check if pParticle passes the cuts
    Bool_t rpOK = kTRUE;
    Bool_t poiOK = kTRUE;
    if (rpCFManager && poiCFManager)
    {
      rpOK = ( rpCFManager->CheckParticleCuts(AliCFManager::kPartRecCuts,pParticle) &&
               rpCFManager->CheckParticleCuts(AliCFManager::kPartSelCuts,pParticle));
      poiOK = ( poiCFManager->CheckParticleCuts(AliCFManager::kPartRecCuts,pParticle) &&
                poiCFManager->CheckParticleCuts(AliCFManager::kPartSelCuts,pParticle));
    }
    if (!(rpOK || poiOK)) continue;

    //make new AliFlowTrack
    AliFlowTrack* pTrack = new AliFlowTrack(pParticle);
    pTrack->SetSource(AliFlowTrack::kFromAOD);

    if (rpOK /* && rpCFManager */ ) // to be fixed - with CF managers uncommented only empty events (NULL in header files)
    {
      pTrack->SetForRPSelection(kTRUE);
      fNumberOfRPs++;
    }
    if (poiOK /* && poiCFManager*/ )
    {
      pTrack->SetForPOISelection(kTRUE);
      fNumberOfPOIs++;
    }
    AddTrack(pTrack);
  }

  //  if (iSelParticlesRP >= fMinMult && iSelParticlesRP <= fMaxMult)
  //  {
  //    if ( (++fCount % 100) == 0)
  //    {
  //      if (!fMCReactionPlaneAngle == 0) cout<<" MC Reaction Plane Angle = "<<  fMCReactionPlaneAngle << endl;
  //      else cout<<" MC Reaction Plane Angle = unknown "<< endl;
  //      cout<<" iGoodTracks = "<<iGoodTracks<<endl;
  //      cout<<" # of RP selected tracks = "<<iSelParticlesRP<<endl;
  //      cout<<" # of POI selected tracks = "<<iSelParticlesPOI<<endl;
  //      cout << "# " << fCount << " events processed" << endl;
  //    }
  //    return pEvent;
  //  }
  //  else
  //  {
  //    cout<<"Not enough tracks in the FlowEventSimple"<<endl;
  //    return 0;
  //  }
  //}
  //else
  //{
  //  cout<<"Event does not pass multiplicity cuts"<<endl;
  //  return 0;
  //}

}

//-----------------------------------------------------------------------
AliFlowEvent::AliFlowEvent( const AliESDEvent* anInput,
                            const AliMCEvent* anInputMc,
                            KineSource anOption,
                            const AliCFManager* rpCFManager,
                            const AliCFManager* poiCFManager ):
  AliFlowEventSimple(20), fApplyRecentering(kFALSE), fCachedRun(-1), fCurrentCentrality(-1)
{
    // constructor
    for(Int_t i(0); i < 9; i++) {
        for(Int_t j(0); j < 2; j++) {
            for(Int_t k(0); k < 2; k++) {
                fMeanQ[i][j][k] = 0.; 
                fWidthQ[i][j][k] = 0.;  
                fMeanQv3[i][j][k] = 0.; 
                fWidthQv3[i][j][k] = 0.;
            }
        }
    }

  //fills the event with tracks from the ESD and kinematics from the MC info via the track label
  if (anOption==kNoKine)
  {
    AliFatal("WRONG OPTION IN AliFlowEventMaker::FillTracks(AliESDEvent* anInput, AliMCEvent* anInputMc, KineSource anOption)");
    exit(1);
  }

  Int_t iNumberOfInputTracks = anInput->GetNumberOfTracks() ;

  Int_t iNumberOfInputTracksMC = anInputMc->GetNumberOfTracks() ;
  if (iNumberOfInputTracksMC==-1)
  {
    AliError("Skipping Event -- No MC information available for this event");
    return;
  }

  //loop over ESD tracks
  for (Int_t itrkN=0; itrkN<iNumberOfInputTracks; itrkN++)
  {
    AliESDtrack* pParticle = anInput->GetTrack(itrkN);   //get input particle
    //get Label
    Int_t iLabel = pParticle->GetLabel();
    //match to mc particle
    AliMCParticle* pMcParticle = (AliMCParticle*) anInputMc->GetTrack(TMath::Abs(iLabel));

    //check
    if (TMath::Abs(pParticle->GetLabel())!=pMcParticle->Label())
      AliWarning(Form("pParticle->GetLabel()!=pMcParticle->Label(), %i, %i", pParticle->GetLabel(), pMcParticle->Label()));

    //check if pParticle passes the cuts
    Bool_t rpOK = kFALSE;
    Bool_t poiOK = kFALSE;
    if (rpCFManager && poiCFManager)
    {
      if(anOption == kESDkine)
      {
        if (rpCFManager->CheckParticleCuts(AliCFManager::kPartGenCuts,pMcParticle,"mcGenCuts1") &&
            rpCFManager->CheckParticleCuts(AliCFManager::kPartRecCuts,pParticle))
          rpOK=kTRUE;
        if (poiCFManager->CheckParticleCuts(AliCFManager::kPartGenCuts,pMcParticle,"mcGenCuts2") &&
            poiCFManager->CheckParticleCuts(AliCFManager::kPartRecCuts,pParticle))
          poiOK=kTRUE;
      }
      else if (anOption == kMCkine)
      {
        if (rpCFManager->CheckParticleCuts(AliCFManager::kPartGenCuts,pMcParticle))
          rpOK=kTRUE;
        if (poiCFManager->CheckParticleCuts(AliCFManager::kPartGenCuts,pMcParticle))
          poiOK=kTRUE;
      }
    }

    if (!(rpOK || poiOK)) continue;

    //make new AliFlowTrack
    AliFlowTrack* pTrack = NULL;
    if(anOption == kESDkine)   //take the PID from the MC & the kinematics from the ESD
    {
      pTrack = new AliFlowTrack(pParticle);
    }
    else if (anOption == kMCkine)   //take the PID and kinematics from the MC
    {
      pTrack = new AliFlowTrack(pMcParticle);
    }

    if (rpOK && rpCFManager)
    {
      fNumberOfRPs++;
      pTrack->SetForRPSelection();
    }
    if (poiOK && poiCFManager) 
    { 
      fNumberOfPOIs++;
      pTrack->SetForPOISelection();
    }

    AddTrack(pTrack);
  }
  SetMCReactionPlaneAngle(anInputMc);
}

//-----------------------------------------------------------------------
AliFlowEvent::AliFlowEvent( const AliESDEvent* anInput,
			    const AliMultiplicity* anInputTracklets,
			    const AliCFManager* poiCFManager ):
  AliFlowEventSimple(20), fApplyRecentering(kFALSE), fCachedRun(-1), fCurrentCentrality(-1)
{
    // constructor
    for(Int_t i(0); i < 9; i++) {
        for(Int_t j(0); j < 2; j++) {
            for(Int_t k(0); k < 2; k++) {
                fMeanQ[i][j][k] = 0.; 
                fWidthQ[i][j][k] = 0.;  
                fMeanQv3[i][j][k] = 0.; 
                fWidthQv3[i][j][k] = 0.;
            }
        }
    }


  //Select the particles of interest from the ESD
  Int_t iNumberOfInputTracks = anInput->GetNumberOfTracks() ;

  //loop over tracks
  for (Int_t itrkN=0; itrkN<iNumberOfInputTracks; itrkN++)
    {
      AliESDtrack* pParticle = anInput->GetTrack(itrkN);   //get input particle

      //check if pParticle passes the cuts
      Bool_t poiOK = kTRUE;
      if (poiCFManager)
	{
	  poiOK = ( poiCFManager->CheckParticleCuts(AliCFManager::kPartRecCuts,pParticle) &&
		    poiCFManager->CheckParticleCuts(AliCFManager::kPartSelCuts,pParticle));
	}
      if (!poiOK) continue;
      
      //make new AliFLowTrack
      AliFlowTrack* pTrack = new AliFlowTrack(pParticle);
          
      //marking the particles used for the particle of interest (POI) selection:
      if(poiOK && poiCFManager)
	{
          fNumberOfPOIs++;
	  pTrack->SetForPOISelection(kTRUE);
	  pTrack->SetSource(AliFlowTrack::kFromESD);
	}

      AddTrack(pTrack);
    }//end of while (itrkN < iNumberOfInputTracks)

  //Select the reference particles from the SPD tracklets
  anInputTracklets = anInput->GetMultiplicity();
  Int_t multSPD = anInputTracklets->GetNumberOfTracklets();
  
  //loop over tracklets
  for (Int_t itracklet=0; itracklet<multSPD; ++itracklet) {
    Float_t thetaTr= anInputTracklets->GetTheta(itracklet);
    Float_t phiTr= anInputTracklets->GetPhi(itracklet);
    // calculate eta
    Float_t etaTr = -TMath::Log(TMath::Tan(thetaTr/2.));
    
    //make new AliFLowTrackSimple
    AliFlowTrack* pTrack = new AliFlowTrack();
    pTrack->SetPt(0.0);
    pTrack->SetEta(etaTr);
    pTrack->SetPhi(phiTr);
    //marking the particles used for the reference particle (RP) selection:
    fNumberOfRPs++;
    pTrack->SetForRPSelection(kTRUE);
    pTrack->SetSource(AliFlowTrack::kFromTracklet);

    //Add the track to the flowevent
    AddTrack(pTrack);
  }

}

//-----------------------------------------------------------------------
AliFlowEvent::AliFlowEvent( const AliESDEvent* esd,
			    const AliCFManager* poiCFManager,
                            Bool_t hybrid):
  AliFlowEventSimple(20), fApplyRecentering(kFALSE), fCachedRun(-1), fCurrentCentrality(-1)
{
    // constructor
    for(Int_t i(0); i < 9; i++) {
        for(Int_t j(0); j < 2; j++) {
            for(Int_t k(0); k < 2; k++) {
                fMeanQ[i][j][k] = 0.; 
                fWidthQ[i][j][k] = 0.;  
                fMeanQv3[i][j][k] = 0.; 
                fWidthQv3[i][j][k] = 0.;
            }
        }
    }


  //Select the particles of interest from the ESD
  Int_t iNumberOfInputTracks = esd->GetNumberOfTracks() ;

  //Double_t gPt = 0.0, gP = 0.0;
  Double_t dca[2] = {0.0,0.0}, cov[3] = {0.0,0.0,0.0};  //The impact parameters and their covariance.
//  Double_t dca3D = 0.0;       FIXME unused variable

  AliESDtrack trackTPC;

  //loop over tracks
  for (Int_t itrkN=0; itrkN<iNumberOfInputTracks; itrkN++)
    {

      if (!esd->GetTrack(itrkN)) continue;

      Bool_t useTPC = kFALSE;

      AliESDtrack* pParticle = esd->GetTrack(itrkN);   //get input particle

      //check if pParticle passes the cuts
      Bool_t poiOK = kTRUE;

      if (poiCFManager)
      {
        poiOK = ( poiCFManager->CheckParticleCuts(AliCFManager::kPartRecCuts,pParticle) &&
                  poiCFManager->CheckParticleCuts(AliCFManager::kPartSelCuts,pParticle));
      }

      if (!(poiOK)) continue;

      AliExternalTrackParam *tpcTrack = (AliExternalTrackParam *)pParticle->GetTPCInnerParam();

      if (tpcTrack)
      {

//      gPt = tpcTrack->Pt();
//      gP = tpcTrack->P();

        useTPC = kTRUE;

        const AliESDVertex *vertexSPD = esd->GetPrimaryVertexSPD();
        const AliESDVertex *vertexTPC = esd->GetPrimaryVertexTPC();

        if(hybrid)
          tpcTrack->PropagateToDCA(vertexSPD,esd->GetMagneticField(),100.,dca,cov);
        else
          tpcTrack->PropagateToDCA(vertexTPC,esd->GetMagneticField(),100.,dca,cov);

//        dca3D = TMath::Sqrt(TMath::Power(dca[0],2)+TMath::Power(dca[1],2));   FIXME unused variable

      }

      //make new AliFLowTrack
      AliFlowTrack* pTrack = new AliFlowTrack(pParticle);

      pTrack->SetSource(AliFlowTrack::kFromESD);

      //marking the particles used for diff. flow:
      if(poiOK && poiCFManager)
      {
        pTrack->SetForPOISelection(kTRUE);
        fNumberOfPOIs++;
      }

      if(useTPC)
      {
        pTrack->SetForRPSelection(kTRUE);
        fNumberOfRPs++;
      }

      AddTrack(pTrack);

    }//end of while (itrkN < iNumberOfInputTracks)

}

//-----------------------------------------------------------------------
AliFlowEvent::AliFlowEvent( const AliESDEvent* anInput,
			    const TH2F* anInputFMDhist,
			    const AliCFManager* poiCFManager ):
  AliFlowEventSimple(20), fApplyRecentering(kFALSE), fCachedRun(-1), fCurrentCentrality(-1)
{
    // constructor
    for(Int_t i(0); i < 9; i++) {
        for(Int_t j(0); j < 2; j++) {
            for(Int_t k(0); k < 2; k++) {
                fMeanQ[i][j][k] = 0.; 
                fWidthQ[i][j][k] = 0.;  
                fMeanQv3[i][j][k] = 0.; 
                fWidthQv3[i][j][k] = 0.;
            }
        }
    }


  //Select the particles of interest from the ESD
  Int_t iNumberOfInputTracks = anInput->GetNumberOfTracks() ;

  //loop over tracks
  for (Int_t itrkN=0; itrkN<iNumberOfInputTracks; itrkN++)
    {
      AliESDtrack* pParticle = anInput->GetTrack(itrkN);   //get input particle

      //check if pParticle passes the cuts
      Bool_t poiOK = kTRUE;
      if (poiCFManager)
	{
	  poiOK = ( poiCFManager->CheckParticleCuts(AliCFManager::kPartRecCuts,pParticle) &&
		    poiCFManager->CheckParticleCuts(AliCFManager::kPartSelCuts,pParticle));
	}
      if (!poiOK) continue;
 
      //make new AliFLowTrack
      AliFlowTrack* pTrack = new AliFlowTrack(pParticle);
          
      //marking the particles used for the particle of interest (POI) selection:
      if(poiOK && poiCFManager)
	{
          fNumberOfPOIs++; 
	  pTrack->SetForPOISelection(kTRUE);
	  pTrack->SetSource(AliFlowTrack::kFromESD);
	}

      AddTrack(pTrack);
    }//end of while (itrkN < iNumberOfInputTracks)

  //Select the reference particles from the FMD hits
  //loop over FMD histogram
  Int_t iBinsEta = anInputFMDhist->GetNbinsX();
  Int_t iBinsPhi = anInputFMDhist->GetNbinsY();
  
  for (Int_t iEta = 1; iEta <= iBinsEta; iEta++){
    Double_t etaFMD = anInputFMDhist->GetXaxis()->GetBinCenter(iEta);
    for (Int_t iPhi = 1; iPhi <= iBinsPhi; iPhi++){
      Double_t phiFMD = anInputFMDhist->GetYaxis()->GetBinCenter(iPhi);
      Double_t weightFMD = anInputFMDhist->GetBinContent(iEta,iPhi);
    
      if (weightFMD > 0.0) { //do not add empty bins
	//make new AliFLowTrackSimple
	AliFlowTrack* pTrack = new AliFlowTrack();
	pTrack->SetPt(0.0);
	pTrack->SetEta(etaFMD);
	pTrack->SetPhi(phiFMD);
	pTrack->SetWeight(weightFMD);
	//marking the particles used for the reference particle (RP) selection:
	pTrack->TagRP();
	fNumberOfRPs++;
	pTrack->SetSource(AliFlowTrack::kFromFMD);

	//Add the track to the flowevent
	AddTrack(pTrack);
	
      }
    }
  }

}

//-----------------------------------------------------------------------
void AliFlowEvent::Fill( AliFlowTrackCuts* rpCuts,
                         AliFlowTrackCuts* poiCuts )
{
  //Fills the event from a vevent: AliESDEvent,AliAODEvent,AliMCEvent
  //the input data needs to be attached to the cuts
  //we have two cases, if we're cutting the same collection of tracks
  //(same param type) then we can have tracks that are both rp and poi
  //in the other case we want to have two exclusive sets of rps and pois
  //e.g. one tracklets, the other PMD or global - USER IS RESPOSIBLE
  //FOR MAKING SURE THEY DONT OVERLAP OR ELSE THE SAME PARTICLE WILL BE
  //TAKEN TWICE

  ClearFast();

  if (!rpCuts || !poiCuts) return;
  AliFlowTrackCuts::trackParameterType sourceRP = rpCuts->GetParamType();
  AliFlowTrackCuts::trackParameterType sourcePOI = poiCuts->GetParamType();
  AliFlowTrack* pTrack=NULL;
  
  // if the source for rp's or poi's is the VZERO detector, get the calibration 
  // and set the calibration parameters
  if (sourceRP == AliFlowTrackCuts::kV0 || sourceRP == AliFlowTrackCuts::kVZERO) {
      SetVZEROCalibrationForTrackCuts(rpCuts);
//      for now, recentering is only applied if the user specifically asks for it
//      by setting the flag to kTRUE in the ali flow track cuts
      fApplyRecentering = rpCuts->GetApplyRecentering();    
      // note: this flag is used in the overloaded implementation of Get2Qsub()
      // and tells the function to use as Qsub vectors the recentered Q-vectors
      // from the VZERO aodb file
  }
  if (sourcePOI ==AliFlowTrackCuts::kV0 || sourcePOI == AliFlowTrackCuts::kVZERO) {
      // probably no-one will choose vzero tracks as poi's ...
      SetVZEROCalibrationForTrackCuts(poiCuts); 
  }
  if (sourceRP==sourcePOI)
  {
    //loop over tracks
    Int_t numberOfInputObject = rpCuts->GetNumberOfInputObjects();
    for (Int_t i=0; i<numberOfInputObject; i++)
    {
      //get input object (particle)
      TObject* particle = rpCuts->GetInputObject(i);

      Bool_t rp = rpCuts->IsSelected(particle,i);
      Bool_t poi = poiCuts->IsSelected(particle,i);
      
      if (!(rp||poi)) continue;

      //make new AliFLowTrack
      if (rp)
      {
        pTrack = ReuseTrack(fNumberOfTracks);
        if (!rpCuts->FillFlowTrack(pTrack)) continue;
        pTrack->TagRP(); fNumberOfRPs++;
        if (poi) {pTrack->TagPOI(); fNumberOfPOIs++;}
      }
      else if (poi)
      {
        pTrack = ReuseTrack(fNumberOfTracks);
        if (!poiCuts->FillFlowTrack(pTrack)) continue;
        pTrack->TagPOI(); fNumberOfPOIs++;
      }
      fNumberOfTracks++;
    }//end of while (i < numberOfTracks)
  }
  else if (sourceRP!=sourcePOI)
  {
    //here we have two different sources of particles, so we fill
    //them independently
    //RP
    Int_t numberOfInputObject = rpCuts->GetNumberOfInputObjects();
    for (Int_t i=0; i<numberOfInputObject; i++)
      {
      TObject* particle = rpCuts->GetInputObject(i);
      Bool_t rp = rpCuts->IsSelected(particle,i);
      if (!rp) continue;
      pTrack = ReuseTrack(fNumberOfTracks);
      if (!rpCuts->FillFlowTrack(pTrack)) continue;
      pTrack->TagRP();
      fNumberOfRPs++;
      fNumberOfTracks++;
    }
    //POI
    numberOfInputObject = poiCuts->GetNumberOfInputObjects();
    for (Int_t i=0; i<numberOfInputObject; i++)
    {
      TObject* particle = poiCuts->GetInputObject(i);
      Bool_t poi = poiCuts->IsSelected(particle,i);
      if (!poi) continue;
      pTrack = ReuseTrack(fNumberOfTracks);
      if (!poiCuts->FillFlowTrack(pTrack)) continue;
      pTrack->TagPOI();
      fNumberOfPOIs++;
      fNumberOfTracks++;
    }
  }
}

//-----------------------------------------------------------------------
void AliFlowEvent::InsertTrack(AliFlowTrack *thisTrack) {
  // adds a flow track at the end of the container
  AliFlowTrack *pTrack = ReuseTrack( fNumberOfTracks++ );
  pTrack->SetPt( thisTrack->Pt() );
  pTrack->SetPhi( thisTrack->Phi() );
  pTrack->SetEta( thisTrack->Eta() );
  pTrack->SetWeight( thisTrack->Weight() );
  pTrack->SetCharge( thisTrack->Charge() );
  pTrack->SetMass( thisTrack->Mass() );
  pTrack->SetForRPSelection( thisTrack->InRPSelection() );
  pTrack->SetForPOISelection( thisTrack->InPOISelection() );
  if(thisTrack->InSubevent(0)) pTrack->SetForSubevent(0);
  if(thisTrack->InSubevent(1)) pTrack->SetForSubevent(1);
  pTrack->SetID( thisTrack->GetID() );
  return;
}

//-----------------------------------------------------------------------
AliFlowTrack* AliFlowEvent::ReuseTrack(Int_t i)
{
  //try to reuse an existing track, if empty, make new one
  AliFlowTrack* pTrack = static_cast<AliFlowTrack*>(fTrackCollection->At(i));
  if (pTrack)
  {
    pTrack->Clear();
  }
  else 
  {
    pTrack = new AliFlowTrack();
    fTrackCollection->AddAtAndExpand(pTrack,i);
  }
  return pTrack;
}

//-----------------------------------------------------------------------
AliFlowEvent::AliFlowEvent( AliFlowTrackCuts* rpCuts,
                            AliFlowTrackCuts* poiCuts ):
  AliFlowEventSimple(20), fApplyRecentering(kFALSE), fCachedRun(-1), fCurrentCentrality(-1)
{
    // constructor
    for(Int_t i(0); i < 9; i++) {
        for(Int_t j(0); j < 2; j++) {
            for(Int_t k(0); k < 2; k++) {
                fMeanQ[i][j][k] = 0.; 
                fWidthQ[i][j][k] = 0.;  
                fMeanQv3[i][j][k] = 0.; 
                fWidthQv3[i][j][k] = 0.;
            }
        }
    }

  //Fills the event from a vevent: AliESDEvent,AliAODEvent,AliMCEvent
  //the input data needs to be attached to the cuts
  //we have two cases, if we're cutting the same collection of tracks
  //(same param type) then we can have tracks that are both rp and poi
  //in the other case we want to have two exclusive sets of rps and pois
  //e.g. one tracklets, the other PMD or global - USER IS RESPOSIBLE
  //FOR MAKING SURE THEY DONT OVERLAP OR ELSE THE SAME PARTICLE WILL BE
  //TAKEN TWICE

  if (!rpCuts || !poiCuts) return;
  AliFlowTrackCuts::trackParameterType sourceRP = rpCuts->GetParamType();
  AliFlowTrackCuts::trackParameterType sourcePOI = poiCuts->GetParamType();

  if (sourceRP==sourcePOI)
  {
    //loop over tracks
    Int_t numberOfInputObject = rpCuts->GetNumberOfInputObjects();
    for (Int_t i=0; i<numberOfInputObject; i++)
    {
      //get input object (particle)
      TObject* particle = rpCuts->GetInputObject(i);

      Bool_t rp = rpCuts->IsSelected(particle,i);
      Bool_t poi = poiCuts->IsSelected(particle,i);
      
      if (!(rp||poi)) continue;

      //make new AliFLowTrack
      AliFlowTrack* pTrack = NULL;
      if (rp)
      {
        pTrack = rpCuts->MakeFlowTrack();
        if (!pTrack) continue;
        pTrack->TagRP(); fNumberOfRPs++;
        if (poi) {pTrack->TagPOI(); fNumberOfPOIs++;}
      }
      else
      if (poi)
      {
        pTrack = poiCuts->MakeFlowTrack();
        if (!pTrack) continue;
        pTrack->TagPOI(); fNumberOfPOIs++;
      }
      AddTrack(pTrack);
    }//end of while (i < numberOfTracks)
  }
  else if (sourceRP!=sourcePOI)
  {
    //here we have two different sources of particles, so we fill
    //them independently
    AliFlowTrack* pTrack = NULL;
    //RP
    Int_t numberOfInputObject = rpCuts->GetNumberOfInputObjects();
    for (Int_t i=0; i<numberOfInputObject; i++)
    {
      TObject* particle = rpCuts->GetInputObject(i);
      Bool_t rp = rpCuts->IsSelected(particle,i);
      if (!rp) continue;
      pTrack = rpCuts->MakeFlowTrack();
      if (!pTrack) continue;
      pTrack->TagRP(); fNumberOfRPs++;
      AddTrack(pTrack);
    }
    //POI
    numberOfInputObject = poiCuts->GetNumberOfInputObjects();
    for (Int_t i=0; i<numberOfInputObject; i++)
    {
      TObject* particle = poiCuts->GetInputObject(i);
      Bool_t poi = poiCuts->IsSelected(particle,i);
      if (!poi) continue;
      pTrack = poiCuts->MakeFlowTrack();
      if (!pTrack) continue;
      pTrack->TagPOI(); fNumberOfPOIs++;
      AddTrack(pTrack);
    }
  }
}

//-------------------------------------------------------------------//
//---- Including PMD tracks as RP --------------------------//

AliFlowEvent::AliFlowEvent( const AliESDEvent* anInput,
			    const AliESDPmdTrack *pmdtracks,
			    const AliCFManager* poiCFManager ):
  AliFlowEventSimple(20), fApplyRecentering(kFALSE), fCachedRun(-1), fCurrentCentrality(-1)
{
    // constructor
    for(Int_t i(0); i < 9; i++) {
        for(Int_t j(0); j < 2; j++) {
            for(Int_t k(0); k < 2; k++) {
                fMeanQ[i][j][k] = 0.; 
                fWidthQ[i][j][k] = 0.;  
                fMeanQv3[i][j][k] = 0.; 
                fWidthQv3[i][j][k] = 0.;
            }
        }
    }

  Float_t GetPmdEta(Float_t xPos, Float_t yPos, Float_t zPos);
  Float_t GetPmdPhi(Float_t xPos, Float_t yPos);
  //Select the particles of interest from the ESD
  Int_t iNumberOfInputTracks = anInput->GetNumberOfTracks() ;
  
  //loop over tracks
  for (Int_t itrkN=0; itrkN<iNumberOfInputTracks; itrkN++)
    {
      AliESDtrack* pParticle = anInput->GetTrack(itrkN);   //get input particle
      //check if pParticle passes the cuts
      Bool_t poiOK = kTRUE;
      if (poiCFManager)
	{
	  poiOK = ( poiCFManager->CheckParticleCuts(AliCFManager::kPartRecCuts,pParticle) &&
		    poiCFManager->CheckParticleCuts(AliCFManager::kPartSelCuts,pParticle));
	}
      if (!poiOK) continue;
      
      //make new AliFLowTrack
      AliFlowTrack* pTrack = new AliFlowTrack(pParticle);
      
      //marking the particles used for the particle of interest (POI) selection:
      if(poiOK && poiCFManager)
	{
          fNumberOfPOIs++;
	  pTrack->SetForPOISelection(kTRUE);
	  pTrack->SetSource(AliFlowTrack::kFromESD);
	}
      
      AddTrack(pTrack);
    }//end of while (itrkN < iNumberOfInputTracks)
  
  //Select the reference particles from the PMD tracks
  Int_t npmdcl = anInput->GetNumberOfPmdTracks();
  printf("======There are %d PMD tracks in this event\n-------",npmdcl);
  //loop over clusters 
  for(Int_t iclust=0; iclust < npmdcl; iclust++){
    //AliESDPmdTrack *pmdtr = anInput->GetPmdTrack(iclust);
    pmdtracks = anInput->GetPmdTrack(iclust);
    Int_t   det   = pmdtracks->GetDetector();
    //Int_t   smn   = pmdtracks->GetSmn();
    Float_t clsX  = pmdtracks->GetClusterX();
    Float_t clsY  = pmdtracks->GetClusterY();
    Float_t clsZ  = pmdtracks->GetClusterZ();
    Float_t ncell = pmdtracks->GetClusterCells();
    Float_t adc   = pmdtracks->GetClusterADC();
    //Float_t pid   = pmdtracks->GetClusterPID();
    Float_t etacls = GetPmdEta(clsX,clsY,clsZ);
    Float_t phicls = GetPmdPhi(clsX,clsY);
    //make new AliFLowTrackSimple
    AliFlowTrack* pTrack = new AliFlowTrack();
    //if(det == 0){ //selecting preshower plane only
    if(det == 0 && adc > 270 && ncell > 1){ //selecting preshower plane only
      //pTrack->SetPt(adc);//cluster adc
      pTrack->SetPt(0.0);
      pTrack->SetEta(etacls);
      pTrack->SetPhi(phicls);
      //marking the particles used for the reference particle (RP) selection:
      fNumberOfRPs++;
      pTrack->SetForRPSelection(kTRUE);
      pTrack->SetSource(AliFlowTrack::kFromPMD);
      //Add the track to the flowevent
      AddTrack(pTrack);
    }//if det
  }
}
//----------------------------------------------------------------------------//
Float_t GetPmdEta(Float_t xPos, Float_t yPos, Float_t zPos)
{
  Float_t rpxpy, theta, eta;
  rpxpy  = TMath::Sqrt(xPos*xPos + yPos*yPos);
  theta  = TMath::ATan2(rpxpy,zPos);
  eta    = -TMath::Log(TMath::Tan(0.5*theta));
  return eta;
}
//--------------------------------------------------------------------------//
Float_t GetPmdPhi(Float_t xPos, Float_t yPos)
{
  Float_t pybypx, phi = 0., phi1;
  if(xPos==0)
    {
      if(yPos>0) phi = 90.;
      if(yPos<0) phi = 270.;
    }
  if(xPos != 0)
    {
      pybypx = yPos/xPos;
      if(pybypx < 0) pybypx = - pybypx;
      phi1 = TMath::ATan(pybypx)*180./3.14159;
      
      if(xPos > 0 && yPos > 0) phi = phi1;        // 1st Quadrant
      if(xPos < 0 && yPos > 0) phi = 180 - phi1;  // 2nd Quadrant
      if(xPos < 0 && yPos < 0) phi = 180 + phi1;  // 3rd Quadrant
      if(xPos > 0 && yPos < 0) phi = 360 - phi1;  // 4th Quadrant
      
    }
  phi = phi*3.14159/180.;
  return   phi;
}
//---------------------------------------------------------------//


void AliFlowEvent::Get2Qsub(AliFlowVector* Qarray, Int_t n, TList *weightsList, Bool_t usePhiWeights, Bool_t usePtWeights, Bool_t useEtaWeights)
{
  // get q vectors for the subevents. if no recentering is necessary, get the guy from the flow event simple
  AliFlowEventSimple::Get2Qsub(Qarray, n, weightsList, usePhiWeights, usePtWeights, useEtaWeights);
  // else get the recentering from the cached info
  if (fApplyRecentering)        // set by Fill()
  {     
    // first retrieve the q-vectors from the AliFlowEventSimple:: routine
    AliFlowVector vA = Qarray[0];
    AliFlowVector vB = Qarray[1];
    // extract the information form the current flow vectors
    Double_t Qxc(vA.X());       // IMPORTANT: user is responsible for the sign of eta
    Double_t Qyc(vA.Y());       // vzeroC has negative pseudorapidity and is taken as subevent A
    Double_t Qxa(vB.X());       // vzeroA has positive pseudorapidity and is taken as subevent B
    Double_t Qya(vB.Y());
    // init some values for the corrections
    
    // values for vector a (VZEROA)
    Double_t Qxamean(0);
    Double_t Qxarms(1);
    Double_t Qyamean(0);
    Double_t Qyarms(1);
    // values for vector b (VZEROC)
    Double_t Qxcmean(0);
    Double_t Qxcrms(1);
    Double_t Qycmean(0);
    Double_t Qycrms(1);	
    
    if( n == 2) {       // second order symmetry
        Qxamean = fMeanQ[fCurrentCentrality][1][0];
        Qxarms  = fWidthQ[fCurrentCentrality][1][0];
        Qyamean = fMeanQ[fCurrentCentrality][1][1];
        Qyarms  = fWidthQ[fCurrentCentrality][1][1];

        Qxcmean = fMeanQ[fCurrentCentrality][0][0];
        Qxcrms  = fWidthQ[fCurrentCentrality][0][0];
        Qycmean = fMeanQ[fCurrentCentrality][0][1];
        Qycrms  = fWidthQ[fCurrentCentrality][0][1];	
    } else if (n == 3) {        // third order symmetry
        Qxamean = fMeanQv3[fCurrentCentrality][1][0];
        Qxarms  = fWidthQv3[fCurrentCentrality][1][0];
        Qyamean = fMeanQv3[fCurrentCentrality][1][1];
        Qyarms  = fWidthQv3[fCurrentCentrality][1][1];
  
        Qxcmean = fMeanQv3[fCurrentCentrality][0][0];
        Qxcrms  = fWidthQv3[fCurrentCentrality][0][0];
        Qycmean = fMeanQv3[fCurrentCentrality][0][1];
        Qycrms  = fWidthQv3[fCurrentCentrality][0][1];	
    }
    // do the correction    
    Double_t QxaCor = (Qxa - Qxamean)/Qxarms;
    Double_t QyaCor = (Qya - Qyamean)/Qyarms;
    Double_t QxcCor = (Qxc - Qxcmean)/Qxcrms;
    Double_t QycCor = (Qyc - Qycmean)/Qycrms;
    // update the vector
    vA.Set(QxcCor, QycCor);
    vB.Set(QxaCor, QyaCor);
  }
}
//_____________________________________________________________________________
void AliFlowEvent::SetVZEROCalibrationForTrackCuts(AliFlowTrackCuts* cuts) {
    // open calibration info, copied from AliAnalyisTaskVnV0.cxx
    if(!cuts->GetEvent()) return; // coverity. we need to know the event to get the runnumber and centrlaity
    // get the vzero centrality percentile (cc dependent calibration)
    Float_t v0Centr(cuts->GetEvent()->GetCentrality()->GetCentralityPercentile("V0M"));
    if(v0Centr < 5) fCurrentCentrality = 0;
    else if(v0Centr < 10) fCurrentCentrality = 1;
    else if(v0Centr < 20) fCurrentCentrality = 2;
    else if(v0Centr < 30) fCurrentCentrality = 3;
    else if(v0Centr < 40) fCurrentCentrality = 4;
    else if(v0Centr < 50) fCurrentCentrality = 5;
    else if(v0Centr < 60) fCurrentCentrality = 6;
    else if(v0Centr < 70) fCurrentCentrality = 7;
    else fCurrentCentrality = 8;

    // if this event is from the same run as the previous event
    // we can use the cached calibration values, no need to re-open the 
    // aodb file
    Int_t run(cuts->GetEvent()->GetRunNumber());
//    printf ( " > run number is %i \n", run);
    if(fCachedRun == run) {
//        printf(" run number didn't change, using cached values \n");
        return;
    }
    // set the chached run number
    fCachedRun = run;
    
    TString oadbfilename = "$ALICE_ROOT/OADB/PWGCF/VZERO/VZEROcalibEP.root";
    TFile *foadb = TFile::Open(oadbfilename.Data());

    if(!foadb){
	printf("OADB file %s cannot be opened\n",oadbfilename.Data());
	return;
    }

    AliOADBContainer *cont = (AliOADBContainer*) foadb->Get("hMultV0BefCorr");
    if(!cont){
	printf("OADB object hMultV0BefCorr is not available in the file\n");
	return;	
    }

    if(!(cont->GetObject(run))){
	printf("OADB object hMultV0BefCorr is not available for run %i (used run 137366)\n",run);
	run = 137366;
    }
    // step 1) get the proper multiplicity weights from the vzero signal
    TProfile* fMultV0 = ((TH2F *) cont->GetObject(run))->ProfileX();

    TF1 *fpol0 = new TF1("fpol0","pol0"); 
    if(cuts->GetV0gainEqualizationPerRing()) {
        // do the calibration per ring
        // start with the vzero c rings (segments 0 through 31)
        fMultV0->Fit(fpol0, "", "", 0, 8);
        (cuts->GetUseVZERORing(0)) ? cuts->SetV0Cpol(0, fpol0->GetParameter(0)) : cuts->SetV0Cpol(0, 0.);
        fMultV0->Fit(fpol0, "", "", 8, 16);
        (cuts->GetUseVZERORing(1)) ? cuts->SetV0Cpol(1, fpol0->GetParameter(0)) : cuts->SetV0Cpol(1, 0.);
        fMultV0->Fit(fpol0, "", "", 16, 24);
        (cuts->GetUseVZERORing(2)) ? cuts->SetV0Cpol(2, fpol0->GetParameter(0)) : cuts->SetV0Cpol(2, 0.);
        fMultV0->Fit(fpol0, "", "", 24, 32);
        (cuts->GetUseVZERORing(3)) ? cuts->SetV0Cpol(3, fpol0->GetParameter(0)) : cuts->SetV0Cpol(3, 0.);
        // same thing for vero A
        fMultV0->Fit(fpol0, "", "", 32, 40);
        (cuts->GetUseVZERORing(4)) ? cuts->SetV0Apol(0, fpol0->GetParameter(0)) : cuts->SetV0Apol(0, 0.);
        fMultV0->Fit(fpol0, "", "", 40, 48);
        (cuts->GetUseVZERORing(5)) ? cuts->SetV0Apol(1, fpol0->GetParameter(0)) : cuts->SetV0Apol(1, 0.);
        fMultV0->Fit(fpol0, "", "", 48, 56);
        (cuts->GetUseVZERORing(6)) ? cuts->SetV0Apol(2, fpol0->GetParameter(0)) : cuts->SetV0Apol(2, 0.);
        fMultV0->Fit(fpol0, "", "", 56, 64);
        (cuts->GetUseVZERORing(7)) ? cuts->SetV0Apol(3, fpol0->GetParameter(0)) : cuts->SetV0Apol(3, 0.);
    } else {
        // do the calibration in one go. the calibration will still be 
        // stored per ring, but each ring has the same weight now
       fMultV0->Fit(fpol0,"","",0,31);
       for(Int_t i(0); i < 4; i++) cuts->SetV0Cpol(i, fpol0->GetParameter(0));
       fMultV0->Fit(fpol0,"","",32,64);
       for(Int_t i(0); i < 4; i++) cuts->SetV0Apol(i, fpol0->GetParameter(0));
    }
    // the parameters to weigh the vzero track cuts have been extracted now, 
    // so we can pass them to the current track cuts obect
    cuts->SetV0gainEqualisation(fMultV0);       // passed as a TH1

    // step 2) reweight the q-vectors that will be  called by flow methods which use
    // subevents
    // underlying assumption is that subevent a uses VZEROA
    // and subevent b uses VZEROC
    for(Int_t iside=0;iside<2;iside++){
	for(Int_t icoord=0;icoord<2;icoord++){
	    for(Int_t i=0;i  < 9;i++){
		char namecont[100];
  		if(iside==0 && icoord==0)
		  snprintf(namecont,100,"hQxc2_%i",i);
		else if(iside==1 && icoord==0)
		  snprintf(namecont,100,"hQxa2_%i",i);
		else if(iside==0 && icoord==1)
		  snprintf(namecont,100,"hQyc2_%i",i);
		else if(iside==1 && icoord==1)
		  snprintf(namecont,100,"hQya2_%i",i);

		cont = (AliOADBContainer*) foadb->Get(namecont);
		if(!cont){
		    printf("OADB object %s is not available in the file\n",namecont);
		    return;	
		}
	
		if(!(cont->GetObject(run))){
		    printf("OADB object %s is not available for run %i (used run 137366)\n",namecont,run);
		    run = 137366;
		}

                // after grabbing all the info, set the CORRECTION TERMS to
                // the 2nd and 3rd order qsub-vectors
                // we do this here for all centralities, so that subsequent events
                // can grab the correction from these cached values
                fMeanQ[i][iside][icoord] = ((TH1F *) cont->GetObject(run))->GetMean();
		fWidthQ[i][iside][icoord] = ((TH1F *) cont->GetObject(run))->GetRMS();

		//for v3
		if(iside==0 && icoord==0)
		  snprintf(namecont,100,"hQxc3_%i",i);
		else if(iside==1 && icoord==0)
		  snprintf(namecont,100,"hQxa3_%i",i);
		else if(iside==0 && icoord==1)
		  snprintf(namecont,100,"hQyc3_%i",i);
		else if(iside==1 && icoord==1)
		  snprintf(namecont,100,"hQya3_%i",i);

		cont = (AliOADBContainer*) foadb->Get(namecont);
		if(!cont){
		    printf("OADB object %s is not available in the file\n",namecont);
		    return;	
		}
		
		if(!(cont->GetObject(run))){
		    printf("OADB object %s is not available for run %i (used run 137366)\n",namecont,run);
		    run = 137366;
		}
		fMeanQv3[i][iside][icoord] = ((TH1F *) cont->GetObject(run))->GetMean();
		fWidthQv3[i][iside][icoord] = ((TH1F *) cont->GetObject(run))->GetRMS();

     	    }
	}
    }
}
//_____________________________________________________________________________
