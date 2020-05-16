#include "Run.hh"

#include "DetectorConstruction.hh"

#include "EventAction.hh"
#include "HistoManager.hh"
#include "PrimaryGeneratorAction.hh"

#include "G4Material.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include <iomanip>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

Run::Run(DetectorConstruction* detector)
: G4Run(),
  fDetector(detector),
  fParticle(0), fEkin(0.)
{
  for (G4int i=0; i<3; ++i) { fStatus[i] = 0; fTotEdep[i] = 0.; }
  fTotEdep[1] = joule;
  for (G4int i=0; i<kMaxAbsor; ++i) {
    fEdeposit[i] = 0.; fEmin[i] = joule; fEmax[i] = 0.;
  }  
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

Run::~Run()
{ }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::SetPrimary (G4ParticleDefinition* particle, G4double energy)
{ 
  fParticle = particle;
  fEkin     = energy;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::CountProcesses(const G4VProcess* process) 
{
  G4String procName = process->GetProcessName();
  std::map<G4String,G4int>::iterator it = fProcCounter.find(procName);
  if ( it == fProcCounter.end()) {
    fProcCounter[procName] = 1;
  }
  else {
    fProcCounter[procName]++; 
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::ParticleCount(G4int k, G4String name, G4double Ekin)
{
 std::map<G4String, ParticleData>::iterator it = fParticleDataMap[k].find(name);
  if ( it == fParticleDataMap[k].end()) {
    (fParticleDataMap[k])[name] = ParticleData(1, Ekin, Ekin, Ekin);
  }
  else {
    ParticleData& data = it->second;
    data.fCount++;
    data.fEmean += Ekin;
   
    //Update min & max
    
    G4double emin = data.fEmin;
    if (Ekin < emin) data.fEmin = Ekin;
    G4double emax = data.fEmax;
    if (Ekin > emax) data.fEmax = Ekin; 
  }   
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::AddEdep (G4int i, G4double e)        
{
  if (e > 0.) {
    fEdeposit[i]  += e;
    if (e < fEmin[i]) fEmin[i] = e;
    if (e > fEmax[i]) fEmax[i] = e;
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::AddTotEdep (G4double e)        
{
  if (e > 0.) {
    fTotEdep[0]  += e;
    if (e < fTotEdep[1]) fTotEdep[1] = e;
    if (e > fTotEdep[2]) fTotEdep[2] = e;
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
      
void Run::AddTrackStatus (G4int i)    
{
  fStatus[i]++ ;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::Merge(const G4Run* run)
{
  const Run* localRun = static_cast<const Run*>(run);
  
  // Retrieve information about events

  // Retrieve information about primary particle
  fParticle = localRun->fParticle;
  fEkin     = localRun->fEkin;

  // Energy deposition in calorimeter counters
  //
  G4int nbOfAbsor = fDetector->GetNbOfAbsor();
  for (G4int i=1; i<=nbOfAbsor; ++i) {
    fEdeposit[i]  += localRun->fEdeposit[i];
    
    // min, max
    
    G4double min,max;
    min = localRun->fEmin[i]; max = localRun->fEmax[i];
    if (fEmin[i] > min) fEmin[i] = min;
    if (fEmax[i] < max) fEmax[i] = max;
  }
   
  for (G4int i=0; i<3; ++i)  fStatus[i] += localRun->fStatus[i];
  
  // Total energy deposition
  fTotEdep[0] += localRun->fTotEdep[0];
  G4double min,max;
  min = localRun->fTotEdep[1]; max = localRun->fTotEdep[2];
  if (fTotEdep[1] > min) fTotEdep[1] = min;
  if (fTotEdep[2] < max) fTotEdep[2] = max;

  //Processes count
  std::map<G4String,G4int>::const_iterator itp;
  for ( itp = localRun->fProcCounter.begin();
        itp != localRun->fProcCounter.end(); ++itp ) {

    G4String procName = itp->first;
    G4int localCount = itp->second;
    if ( fProcCounter.find(procName) == fProcCounter.end()) {
      fProcCounter[procName] = localCount;
    }
    else {
      fProcCounter[procName] += localCount;
    }  
  }
  
  // Particles created in calorimeters counters (count)
  for (G4int k=0; k<=nbOfAbsor; ++k) {
    std::map<G4String,ParticleData>::const_iterator itc;
    for (itc = localRun->fParticleDataMap[k].begin(); 
         itc != localRun->fParticleDataMap[k].end(); ++itc) {

      G4String name = itc->first;
      const ParticleData& localData = itc->second;   
      if ( fParticleDataMap[k].find(name) == fParticleDataMap[k].end()) {
        (fParticleDataMap[k])[name]
         = ParticleData(localData.fCount, 
                        localData.fEmean, 
                        localData.fEmin, 
                        localData.fEmax);
      }
      else {
        ParticleData& data = (fParticleDataMap[k])[name];   
        data.fCount += localData.fCount;
        data.fEmean += localData.fEmean;
        G4double emin = localData.fEmin;
        if (emin < data.fEmin) data.fEmin = emin;
        G4double emax = localData.fEmax;
        if (emax > data.fEmax) data.fEmax = emax; 
      }
    }
  }

  G4Run::Merge(run); 
} 

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::EndOfRun() 
{
    G4int prec = 5, wid = prec + 2;  
    G4int dfprec = G4cout.precision(prec);

  //Run conditions
  
  G4String partName = fParticle->GetParticleName();
  G4int nbOfAbsor   = fDetector->GetNbOfAbsor();
  
  G4cout << "\n ======================== run summary =====================\n";
  G4cout 
    << "\n The run is " << numberOfEvent << " "<< partName << " of "
    << G4BestUnit(fEkin,"Energy") 
    << " through "  << nbOfAbsor << " Pb glass counters: \n";
  for (G4int i=1; i<= nbOfAbsor; i++) {
     G4Material* material = fDetector->GetAbsorMaterial(i);
     G4double thickness = fDetector->GetAbsorThickness(i);
     G4double density = material->GetDensity();
     G4cout << std::setw(5) << i
            << std::setw(10) << G4BestUnit(thickness,"Length") << " of "
            << material->GetName() << " (density: " 
            << G4BestUnit(density,"Volumic Mass") << ")" << G4endl;
  }         

  if (numberOfEvent == 0) { G4cout.precision(dfprec);  return;}
  
  G4cout.precision(3);
  
  //Frequency of processes
  //
  G4cout << "\n Process calls frequency :" << G4endl;
  G4int index = 0;
  std::map<G4String,G4int>::iterator it;    
  for (it = fProcCounter.begin(); it != fProcCounter.end(); it++) {
     G4String procName = it->first;
     G4int    count    = it->second;
     G4String space = " "; if (++index%3 == 0) space = "\n";
     G4cout << " " << std::setw(20) << procName << "="<< std::setw(7) << count
            << space;
  }
  G4cout << G4endl;
  
  //Energy deposition in calorimeter counters
  //
  for (G4int i=1; i<= nbOfAbsor; i++) {
     fEdeposit[i] /= numberOfEvent;

     G4cout 
       << "\n Edep in Pb glass counter " << i << " = " 
       << G4BestUnit(fEdeposit[i],"Energy")
       << "\t(" << G4BestUnit(fEmin[i], "Energy")
       << "-->" << G4BestUnit(fEmax[i], "Energy")
       << ")";
  }
  G4cout << G4endl;

  if (nbOfAbsor > 1) {
    fTotEdep[0] /= numberOfEvent;
    G4cout 
      << "\n Edep in all Pb glass counters = " << G4BestUnit(fTotEdep[0],"Energy")
      << "\t(" << G4BestUnit(fTotEdep[1], "Energy")
      << "-->" << G4BestUnit(fTotEdep[2], "Energy")
      << ")" << G4endl;
  }

  //Particles generated in calorimeter counters (count) 
  //
  for (G4int k=1; k<= nbOfAbsor; k++) {
  G4cout << "\n List of generated particles in Pb glass counter (count>100) " << k << ":" << G4endl;

    std::map<G4String,ParticleData>::iterator itc;               
    for (itc  = fParticleDataMap[k].begin();
         itc != fParticleDataMap[k].end(); itc++) {
       G4String name = itc->first;
       ParticleData data = itc->second;
       G4int count = data.fCount;
       G4double eMean = data.fEmean/count;
       G4double eMin = data.fEmin;
       G4double eMax = data.fEmax;    
      if(count > 100){
       G4cout << "  " << std::setw(13) << name << ": " << std::setw(7) << count
              << "  Emean = " << std::setw(wid) << G4BestUnit(eMean, "Energy")
              << "\t( "  << G4BestUnit(eMin, "Energy")
              << " --> " << G4BestUnit(eMax, "Energy") 
              << ") " 
              << "Mean Number of Particles Produced " << count/numberOfEvent <<G4endl;         
      }  
    }
  }
  //Particles emerging from calorimeter

  G4cout << "\n List of particles emerging from Pb glass counters :" << G4endl;
  
  std::map<G4String,ParticleData>::iterator itc;
  for (itc  = fParticleDataMap[0].begin();
       itc != fParticleDataMap[0].end(); itc++) {
    G4String name = itc->first;
    ParticleData data = itc->second;
    G4int count = data.fCount;
    G4double eMean = data.fEmean/count;
    G4double eMin = data.fEmin;
    G4double eMax = data.fEmax;
    G4cout << "  " << std::setw(13) << name << ": " << std::setw(7) << count
           << "  Emean = " << std::setw(wid) << G4BestUnit(eMean, "Energy")
           << "\t( "  << G4BestUnit(eMin, "Energy")
           << " --> " << G4BestUnit(eMax, "Energy") 
           << ") " 
           << "Mean Number of Particles Emerging "<< count/numberOfEvent<< G4endl;
  }

  //transmission coefficients
  //
  G4double dNofEvents = double(numberOfEvent);
  G4double absorbed  = 100.*fStatus[0]/dNofEvents;
  G4double transmit  = 100.*fStatus[1]/dNofEvents;

  G4cout.precision(2);       
  G4cout 
    << "\n Nb of events with primary absorbed = "  << absorbed  << " %,"
    << "   transmit = "  << transmit  << " %," << G4endl;

  // Normalize histograms of longitudinal energy profile
  
  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
  G4int ih = 10;
  G4double binWidth = analysisManager->GetH1Width(ih)
                     *analysisManager->GetH1Unit(ih);
  G4double fac = (1./(numberOfEvent*binWidth))*(mm/MeV);
  analysisManager->ScaleH1(ih,fac);

  //Remove all contents in fProcCounter, fCount 
  
  fProcCounter.clear();
  for (G4int k=0; k<= nbOfAbsor; k++) fParticleDataMap[k].clear();

  //Reset default formats
  
  G4cout.precision(dfprec);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
