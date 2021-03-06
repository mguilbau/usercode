// -*- C++ -*-
//
// Package:    CalibTowerJetProducer
// Class:      CalibTowerJetProducer
// 
/**\class CalibTowerJetCollection CalibTowerJetProducer.cc MVACALIB/CalibTowerJetProducer/src/CalibTowerJetProducer.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Robyn Elizabeth Lucas,510 1-002,+41227673823,
//         Created:  Mon Nov 19 10:20:06 CET 2012
// $Id: CalibTowerJetProducer.cc,v 1.8 2013/01/30 13:56:54 rlucas Exp $
//
//


// system include files
#include <memory>
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "TLorentzVector.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/L1Trigger/interface/L1JetParticle.h"
#include "DataFormats/L1Trigger/interface/L1JetParticleFwd.h"
#include "DataFormats/L1Trigger/interface/L1EtMissParticle.h"
#include "DataFormats/L1Trigger/interface/L1EtMissParticleFwd.h"
#include "SimDataFormats/SLHC/interface/L1TowerJet.h"
#include "SimDataFormats/SLHC/interface/L1TowerJetFwd.h"
//#include "JetMETCorrections/Objects/interface/JetCorrector.h"
//#include "DataFormats/JetReco/interface/CaloJetCollection.h"
#include "FWCore/Framework/interface/LuminosityBlock.h"
#include <iostream>
#include <fstream>

#include "TMVA/Tools.h"
#include "TMVA/Reader.h"

#include "FWCore/ParameterSet/interface/FileInPath.h"
//
// class declaration
//


using namespace l1slhc;
using namespace edm;
using namespace std;
using namespace reco;
using namespace l1extra;


bool myfunction (float i,float j) { return (i>j); }

bool sortTLorentz (TLorentzVector i,TLorentzVector j) { return ( i.Pt()>j.Pt() ); }


class CalibTowerJetProducer : public edm::EDProducer {
   public:
      explicit CalibTowerJetProducer(const edm::ParameterSet&);
      ~CalibTowerJetProducer();

      static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

   private:
      virtual void beginJob() ;
      virtual void produce(edm::Event&, const edm::EventSetup&);
      virtual void endJob() ;
      
      virtual void beginRun(edm::Run&, edm::EventSetup const&);
      virtual void endRun(edm::Run&, edm::EventSetup const&);
      virtual void beginLuminosityBlock(edm::LuminosityBlock&, edm::EventSetup const&);
      virtual void endLuminosityBlock(edm::LuminosityBlock&, edm::EventSetup const&);
    
      float get_rho(double L1rho);
    //central calibration
      float get_cal_pt( float l1pt , float l1eta , float l1phi  );
    //fwd calibration: V ROUGH (only to L1extra particles)
      double rough_ptcal(double pt);
    
      double Median(vector<double> aVec);

      void TMVA_calibration();
      // ----------member data ---------------------------
      ParameterSet conf_;
    
      ifstream indata;
      ifstream inrhodata;
    
      vector < pair<double, double> > rho_cal_vec;
      float corrFactors[100][100][100];

      edm::FileInPath inPtData_edm;
   
      edm::FileInPath inRhoData_edm;
   
      edm::FileInPath inMVAweights_edm;
      float l1Pt, l1Eta;
      TMVA::Reader *reader;
      float val_pt_cal(float l1pt, float l1eta);
};

//
// constants, enums and typedefs
//


//
// static data member definitions
//

//
// constructors and destructor
//

CalibTowerJetProducer::CalibTowerJetProducer(const edm::ParameterSet& iConfig):
conf_(iConfig)
{

    produces<L1TowerJetCollection>("CalibCenJets");
    produces<L1TowerJetCollection>("CalibFwdJets");
    produces<float>("Rho");
    produces< L1JetParticleCollection >( "Cen8x8" ) ;
    produces< L1JetParticleCollection >( "Fwd8x8" ) ;
    produces< L1EtMissParticleCollection >( "TowerMHT" ) ;
    produces<float>("TowerHT");
    
  //look up tables
    inPtData_edm = iConfig.getParameter<edm::FileInPath> ("inPtdata_file");
    inRhoData_edm = iConfig.getParameter<edm::FileInPath> ("inRhodata_file");
    inMVAweights_edm = iConfig.getParameter<edm::FileInPath> ("inMVA_weights_file");

}


CalibTowerJetProducer::~CalibTowerJetProducer()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}




// ------------ method called to produce the data  ------------
void
CalibTowerJetProducer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
    
    bool evValid =true;
    float outrho;
    float ht;
    auto_ptr< L1TowerJetCollection > outputCollCen(new L1TowerJetCollection());
    auto_ptr< L1TowerJetCollection > outputCollFwd(new L1TowerJetCollection());
    produces<L1TowerJetCollection>("CalibFwdJets");
    auto_ptr< L1JetParticleCollection > outputExtraCen(new L1JetParticleCollection());
    auto_ptr< L1JetParticleCollection > outputExtraFwd(new L1JetParticleCollection());
    auto_ptr<l1extra::L1EtMissParticleCollection> outputmht(new L1EtMissParticleCollection());
    auto_ptr<float> outRho(new float(outrho));
    auto_ptr<float> outHT(new float(ht));






    edm::Handle<L1TowerJetCollection > UnCalibCen;
    iEvent.getByLabel(conf_.getParameter<edm::InputTag>("FilteredCircle8"), UnCalibCen);
    if(!UnCalibCen.isValid()){evValid=false;}


    edm::Handle<L1TowerJetCollection > UnCalibFwd;
    iEvent.getByLabel(conf_.getParameter<edm::InputTag>("FilteredFwdCircle8"), UnCalibFwd);
    if(!UnCalibFwd.isValid()){evValid=false;}

    if( !evValid ) {
      edm::LogWarning("MissingProduct") << conf_.getParameter<edm::InputTag>("FilteredCircle8") << "," << conf_.getParameter<edm::InputTag>("FilteredFwdCircle8") << std::endl; 
    }
    else{

        //produce calibrated rho collection

      //cout<<" jet collection size: "<< UnCalibCen->size() <<endl;
      int count(0);
      vector<double> Jet2Energies;
      for (L1TowerJetCollection::const_iterator il1 = UnCalibCen->begin();
        il1!= UnCalibCen->end() ; ++il1 ){
        if( abs(il1->p4().eta() )>3) continue;
        if(count>1) {
          Jet2Energies.push_back(il1->p4().Pt());   
          //cout<<"jet energy: "<< il1->p4().Pt() <<endl;
        }
        count++;
      }
      double areaPerJet = 52 * (0.087 * 0.087) ;

      float raw_rho2 = ( Median( Jet2Energies ) / areaPerJet );

      double cal_rhoL1 = raw_rho2 * get_rho(raw_rho2);

      ///////////////////////////////////////////////////
      //              SET VALUE OF RHO 
      ///////////////////////////////////////////////////
      outrho=cal_rhoL1;
      //+cout<<"Setting output rho: "<< outrho << endl;

      ///////////////////////////////////////////////////
      //              JET VALUES 
      ///////////////////////////////////////////////////     
      
      //Value of HT                                                                                                            
      ht=0;
      //vector of MHT
      math::PtEtaPhiMLorentzVector mht, upgrade_jet;

      //Produce calibrated pt collection: central jets
      for (L1TowerJetCollection::const_iterator il1 = UnCalibCen->begin();
           il1!= UnCalibCen->end() ;
           ++il1 ){

          L1TowerJet h=(*il1);

          float l1Eta_ = il1->p4().eta();
          float l1Phi_ = il1->p4().phi();
          float l1Pt_  = il1->p4().Pt();

          //HACKK
          //not quite right conversion in L1TowerJet.cc: refine weighted eta/phi
          float l1wEta_ = il1->WeightedEta();
          if( il1->WeightedEta() >= 0.1 ) l1wEta_ += 0.110481;
          if( il1->WeightedEta() < 0 ) l1wEta_ += -0.0167664;
          float l1wPhi_ = il1->WeightedPhi() + 8.38659e-02;


          //PU subtraction
          float l1Pt_PUsub_ = l1Pt_ - (cal_rhoL1 * areaPerJet);

          //only keep jet if pt > 0 after PU sub 
          if(l1Pt_PUsub_>0.1){
            //Get the calibration factors from the MVA lookup table
            float cal_Pt_ = val_pt_cal( l1Pt_PUsub_ , l1wEta_) * l1Pt_PUsub_;

            math::PtEtaPhiMLorentzVector p4;

            p4.SetCoordinates(cal_Pt_ , l1wEta_ , l1wPhi_ , il1->p4().M() );

            h.setP4(p4);
            outputCollCen->insert( l1wEta_ , l1wPhi_ , h );
            upgrade_jet.SetCoordinates(cal_Pt_ , l1wEta_ , l1wPhi_ , il1->p4().M() );

            if( cal_Pt_>15 ) ht+=cal_Pt_;
            if( cal_Pt_>15 ) mht+=upgrade_jet;

	    // add jet to L1Extra list
            outputExtraCen->push_back( L1JetParticle( math::PtEtaPhiMLorentzVector( cal_Pt_,
										    l1wEta_,
										    l1wPhi_,
										    0. ),
						      Ref< L1GctJetCandCollection >(),
						      0 )
				       );

          }
 
        }
   
        //Produce (rougly) calibrated collection of fwd jets
        //THIS IS UNVERIFIED
        for (L1TowerJetCollection::const_iterator il1 = UnCalibFwd->begin();
             il1!= UnCalibFwd->end() ;
             ++il1 ){

          L1TowerJet h=(*il1);

          float l1Eta_ = il1->p4().eta();
          float l1Phi_ = il1->p4().phi();
          float l1Pt_  = il1->p4().Pt();
          
          float FwdEffArea=8*0.0873*(2*0.5);
          float l1Pt_PUsub_ = l1Pt_ - (cal_rhoL1 * FwdEffArea);

          if(l1Pt_PUsub_>0.1){
              //Get the calibration factors from the lookup table
              float cal_Pt_ =rough_ptcal( l1Pt_PUsub_  );

              math::PtEtaPhiMLorentzVector p4;

              p4.SetCoordinates(cal_Pt_ ,l1Eta_,l1Phi_,il1->p4().M());

              h.setP4(p4);
              outputCollFwd->insert( il1->iEta(), il1->iPhi(), h );
              outputExtraFwd->push_back(L1JetParticle(h.p4()));

/*
              upgrade_jet.SetCoordinates(cal_Pt_ ,l1Eta_,l1Phi_,il1->p4().M());

              ht+=cal_Pt_;
              mht+=upgrade_jet;*/

          }
 
        }
   

	// create L1Extra object
	math::PtEtaPhiMLorentzVector p4tmp = math::PtEtaPhiMLorentzVector( mht.pt(),
									   0.,
									   mht.phi(),
									   0. ) ;
	
	L1EtMissParticle l1extraMHT(p4tmp,
			     L1EtMissParticle::kMHT,
			     ht,
			     Ref< L1GctEtMissCollection >(),
			     Ref< L1GctEtTotalCollection >(),
			     Ref< L1GctHtMissCollection >(),
			     Ref< L1GctEtHadCollection >(),
			     0);

        outputmht->push_back(l1extraMHT);

    }

    iEvent.put(outputCollCen,"CalibCenJets");
    iEvent.put(outputCollFwd,"CalibFwdJets");
    iEvent.put(outRho,"Rho");
    iEvent.put(outputExtraCen,"Cen8x8");
    iEvent.put(outputExtraFwd,"Fwd8x8");
    iEvent.put(outputmht,"TowerMHT" );
    iEvent.put(outHT,"TowerHT");

}


// ------------ method called once each job just before starting event loop  ------------
void 
CalibTowerJetProducer::beginJob()
{

}

// ------------ method called once each job just after ending the event loop  ------------
void 
CalibTowerJetProducer::endJob() {
}

// ------------ method called when starting to processes a run  ------------
void 
CalibTowerJetProducer::beginRun(edm::Run&, edm::EventSetup const&)
{    

    //read in calibration for rho lookup table

    inrhodata.open(inRhoData_edm.fullPath().c_str());
    if(!inrhodata) cerr<<" unable to open rho lookup file. "<<endl;


    //read into a vector
    pair<double, double> rho_cal;
    double L1rho_(9999), calFac_(9999);
    while ( !inrhodata.eof() ) { // keep reading until end-of-file
        // sets EOF flag if no value found
        inrhodata >> L1rho_ >> calFac_ ;
        
        rho_cal.first = L1rho_;
        rho_cal.second= calFac_;

        rho_cal_vec.push_back(rho_cal);
    }
    inrhodata.close();
    
    cout<<" Read in rho lookup table"<<endl;
    

    //read in calibration for pt from TMVA
    
   
    TMVA_calibration();


//     indata.open(inPtData_edm.fullPath().c_str());
//     if(!indata) cerr<<" unable to open pt lookup file. "<<endl;
// 
// 
// 
//     
//     indata.seekg(0);
//     
//     float pt_, eta_, phi_, cal_pt_;
//     double prevpt(0), preveta(0), prevphi(0);
//     int ptcount(0), etacount(0), phicount(0);
//     
//     while ( !indata.eof() ) { // keep reading until end-of-file
//         
//         // sets EOF flag if no value found
//         indata >> pt_ >> eta_ >> phi_ >> cal_pt_;
//         
//         if(pt_!=prevpt)  ptcount++;
//         if(eta_!=preveta) etacount++;
//         if(phi_!=prevphi) phicount++;
//         
//         prevpt = pt_;
//         preveta = eta_ ;
//         prevphi = phi_ ;
//         
//         if(eta_/(-3) <1.001 && eta_/(-3) >0.9999) etacount=0;
//         if(phi_/(-3) <1.001 && phi_/(-3) >0.9999 ) phicount=0;
//         
//         corrFactors[ptcount][etacount][phicount] = cal_pt_;
//     }
//     
//     indata.close();
//     
//     cout<<" pt lookup table read in. "<<endl;
    



  TMVA_calibration();

}

// ------------ method called when ending the processing of a run  ------------
void 
CalibTowerJetProducer::endRun(edm::Run&, edm::EventSetup const&)
{
}

// ------------ method called when starting to processes a luminosity block  ------------
void 
CalibTowerJetProducer::beginLuminosityBlock(edm::LuminosityBlock&, edm::EventSetup const&)
{
}

// ------------ method called when ending the processing of a luminosity block  ------------
void 
CalibTowerJetProducer::endLuminosityBlock(edm::LuminosityBlock&, edm::EventSetup const&)
{
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
CalibTowerJetProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

//
// member functions
//
double CalibTowerJetProducer::Median( vector<double> aVec){
    sort( aVec.begin(), aVec.end() );
    double median(0);
    int size = aVec.size();
    if(size ==0){
        median = 0;
    }
    else if(size==1){
        median = aVec[size-1];
    }
    else if( size%2 == 0 ){
        median = ( aVec[ (size/2)-1  ] + aVec[ (size /2) ] )/2;
    }else{
        median = aVec [ double( (size/2) ) +0.5 ];
    }
    return median;
}



void CalibTowerJetProducer::TMVA_calibration()
{
  cout<<"Getting lookup from MVA"<<endl;

  reader = new TMVA::Reader("!Color:Silent");
  reader->AddVariable( "l1Pt", &l1Pt);
  reader->AddVariable( "l1Eta", &l1Eta);
//  reader->AddVariable( "l1Phi", &l1Phi);
//  reader->AddVariable( "MVA_Rho", &MVA_Rho );

   cout<<"Booking MVA reader"<<endl;

  reader->BookMVA("BDT method",inMVAweights_edm.fullPath().c_str());

  cout<<"MVA reader booked: start processing events."<<endl;

}

float
CalibTowerJetProducer::get_rho(double L1_rho)
{

  //get the rho multiplication factor:
  if(L1_rho<=40.5)   return rho_cal_vec[L1_rho*2].second;
  else return 1.44576*L1_rho;

}

double
CalibTowerJetProducer::rough_ptcal(double pt)
{

  vector<double> calibs;

  calibs.push_back(45.1184); calibs.push_back(1.42818); calibs.push_back(-0.00311431); 
  double l1pt = 0;
  for(unsigned int i=0; i<calibs.size(); i++){
    l1pt += calibs[i]*pow(pt,i);
  }
  return l1pt;

}

float
CalibTowerJetProducer::val_pt_cal(float l1pt, float l1eta)
{
  l1Eta=l1eta;
  l1Pt=l1pt;

  Float_t val = (reader->EvaluateRegression(TString("BDT method") ))[0];

  //cout<<"l1 pt: " << l1pt <<" corr_pt_1 "<<corr_pt_1<<endl;  


  return val;


}

float
CalibTowerJetProducer::get_cal_pt(  float pt , float eta , float phi  )
{
    int eta_bin = (5*eta + 15);
    int phi_bin = (5*phi + 15);
    
    //get pt bin
    int pt_bin(0);
    if(pt<18){
        pt_bin = ((log(pt)-log(0.1))/0.2)+1;
    }
    else if(pt<105){
        pt_bin = ((log(pt)-log(18.1272))/0.1) +27;
    }else{
        pt_bin = (log(pt)- log(109.663))/0.05 +45;
    }
    
    //get multiplication factor for calibration from lookup
    
    double cal_factor = corrFactors[pt_bin][eta_bin][phi_bin]; //get pt * val
    
    double cal_pt = cal_factor * pt; 
    
    //refine 
    
    vector<double> calibs;
    calibs.push_back(0.827817);
    calibs.push_back(-0.0520448);
    calibs.push_back(0.00103698);
    calibs.push_back(-8.55341e-06);
    calibs.push_back(2.49109e-08);
    
    double cal_fac(0);
    for(unsigned int i=0; i<calibs.size(); i++){
        cal_fac+=calibs[i]*pow(cal_pt,i);
    }

    return cal_factor*(1/(1-cal_fac));
    
    
}


//define this as a plug-in
DEFINE_FWK_MODULE(CalibTowerJetProducer);
