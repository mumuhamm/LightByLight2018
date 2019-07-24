//  getDiphotonEfficiency
//
//  Created by Jeremi Niedziela on 22/07/2019.
//
// Calculates diphoton efficiency, defined as:
// Eff^{\gamma\gamma} = N^{reco}/N^{gen}
//
// where:
//
// N^{reco}: number of reconstructed diphoton events with E_T^{reco} > 2 GeV, |\eta^{reco}|<2.4,
// passing trigger, identification and exclusivity cuts
//
// N^{gen}: number of generated events with E_T^{gen} > 2 GeV, |\eta^{gen}|<2.4

#include "Helpers.hpp"
#include "EventProcessor.hpp"
#include "ConfigManager.hpp"

string configPath = "configs/efficiencies.md";

bool DiphotonPtAboveThreshold(const vector<shared_ptr<PhysObject>> &photonSCs)
{
  double pt_1 = photonSCs[0]->GetPt();
  double pt_2 = photonSCs[1]->GetPt();
  double deltaPhi = photonSCs[0]->GetPhi() - photonSCs[1]->GetPhi();
  
  double pairPt = sqrt(pt_1*pt_1 + pt_2*pt_2 + pt_1*pt_2*cos(deltaPhi));
  if(pairPt > config.params["diphotonMaxPt"]) return true;
  
  return false;
}

bool HasTwoMatchingPhotons(const vector<shared_ptr<PhysObject>> &genPhotons,
                           const vector<shared_ptr<PhysObject>> &photonSCs)
{
  int nMatched=0;
  
  for(auto &genPhoton : genPhotons){
    for(auto &cluster : photonSCs){
      double deltaR = sqrt(pow(genPhoton->GetEta() - cluster->GetEta(), 2)+
                           pow(genPhoton->GetPhi() - cluster->GetPhi(), 2));
      if(deltaR < config.params["maxDeltaR"]) nMatched++;
    }
  }
  
  return nMatched >= 2;
}

int main()
{
  config = ConfigManager(configPath);
  unique_ptr<EventProcessor> eventProcessor(new EventProcessor(kMClbl));
  
  int nGenEvents = 0;
  int nEventsPassingAll = 0;
  int nEventsIDmatched = 0;
  int iEvent;
  
  float bins[] = { 0, 2, 3, 4, 5, 6, 8, 12, 16, 20 };
  TH1D *recoEffNum = new TH1D("reco_id_eff_num", "reco_id_eff_num", 9, bins);
  TH1D *recoEffDen = new TH1D("reco_id_eff_den", "reco_id_eff_den", 9, bins);
  recoEffNum->Sumw2();
  recoEffDen->Sumw2();
  
  for(iEvent=0; iEvent<eventProcessor->GetNevents(); iEvent++){
    if(iEvent%10000 == 0) cout<<"Processing event "<<iEvent<<endl;
    if(iEvent >= config.params["maxEvents"]) break;
    
    auto event = eventProcessor->GetEvent(iEvent);
    
    auto goodGenPhotons  = event->GetGoodGenPhotons();
    auto photonSCpassing = event->GetGoodPhotonSCs();
    
    if(goodGenPhotons.size() == 2){ // Exactly 2 gen photons?
      nGenEvents++;
      recoEffDen->Fill(goodGenPhotons.front()->GetEt());
    }
    
    
    if(event->HasLbLTrigger()){ // Check if event has any of the LbL triggers
      
      if(photonSCpassing.size() == 2){ // Exactly 2 passing photon candidates?
        
        if(!event->HasAdditionalTowers()){ // Neutral exclusivity
          
          if(!event->HasChargedTracks()){ // Charged exlusivity

            if(!DiphotonPtAboveThreshold(photonSCpassing)){ // Diphoton momentum
              nEventsPassingAll++;
            }
          }
        }
      }
    }
    
    // Are there 2 reconstructed photons matching gen photons?
    if(HasTwoMatchingPhotons(goodGenPhotons, event->GetPhotonSCs())){
      nEventsIDmatched++;
      recoEffNum->Fill(goodGenPhotons.front()->GetEt());
    }
  }
  
  cout<<"\n\n------------------------------------------------------------------------"<<endl;
  cout<<"N event analyzed: "<<iEvent<<endl;
  cout<<"N gen events within limits: "<<nGenEvents<<endl;
  cout<<"N events passing selection: "<<nEventsPassingAll<<endl;
  cout<<"N events with matched photons passing selection: "<<nEventsIDmatched<<endl;
  
  cout<<"Diphoton efficiency: "<<(double)nEventsPassingAll/nGenEvents<<endl;
  cout<<"Diphoton efficiency uncertainty: "<<sqrt(1./nEventsPassingAll+1./nGenEvents)*(double)nEventsPassingAll/nGenEvents<<endl;
  
  
  cout<<"Reco+ID efficiency: "<<(double)nEventsIDmatched/nGenEvents<<endl;
  cout<<"Reco+ID efficiency uncertainty: "<<sqrt(1./nEventsIDmatched+1./nGenEvents)*(double)nEventsIDmatched/nGenEvents<<endl;
  cout<<"------------------------------------------------------------------------\n\n"<<endl;
  
  TH1D *recoEff = new TH1D(*recoEffNum);
  recoEff->SetTitle("reco_id_eff");
  recoEff->SetName("reco_id_eff");
  
  recoEff->Divide(recoEffNum, recoEffDen, 1, 1, "B");
  
  TFile *outFile = new TFile("results/efficiencies.root", "recreate");
  outFile->cd();
  recoEffNum->Write();
  recoEffDen->Write();
  recoEff->Write();
  outFile->Close();
  
  return 0;
}

