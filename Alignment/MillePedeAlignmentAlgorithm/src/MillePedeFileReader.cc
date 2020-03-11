/*** Header file ***/
#include "Alignment/MillePedeAlignmentAlgorithm/interface/MillePedeFileReader.h"
#include "Alignment/CommonAlignment/interface/AlignableObjectId.h"

/*** system includes ***/
#include <cmath>  // include floating-point std::abs functions
#include <fstream>

/*** Alignment ***/
#include "Alignment/TrackerAlignment/interface/AlignableTracker.h"

//=============================================================================
//===   PUBLIC METHOD IMPLEMENTATION                                        ===
//=============================================================================

MillePedeFileReader ::MillePedeFileReader(const edm::ParameterSet& config,
                                          const std::shared_ptr<const PedeLabelerBase>& pedeLabeler,
                                          const std::shared_ptr<const AlignPCLThresholds>& theThresholds)
    : pedeLabeler_(pedeLabeler),
      theThresholds_(theThresholds),
      millePedeEndFile_(config.getParameter<std::string>("millePedeEndFile")),
      millePedeLogFile_(config.getParameter<std::string>("millePedeLogFile")),
      millePedeResFile_(config.getParameter<std::string>("millePedeResFile")) {}

void MillePedeFileReader ::read() {
  readMillePedeEndFile();
  readMillePedeLogFile();
  readMillePedeResultFile();
}

bool MillePedeFileReader ::storeAlignments() { return (updateDB_ && !vetoUpdateDB_); }

//=============================================================================
//===   PRIVATE METHOD IMPLEMENTATION                                       ===
//=============================================================================
void MillePedeFileReader ::readMillePedeEndFile() {
  std::ifstream endFile;
  endFile.open(millePedeEndFile_.c_str());

  if (endFile.is_open()) {
    edm::LogInfo("MillePedeFileReader") << "Reading millepede end-file";
    std::string line;
    getline(endFile, line);
    std::string trash;
    if (line.find("-1") != std::string::npos) {
      getline(endFile, line);
      exitMessage_ = line;
      std::istringstream iss(line);
      iss >> exitCode_ >> trash;
      edm::LogInfo("MillePedeFileReader")
          << " Pede exit code is: " << exitCode_ << " (" << exitMessage_ << ")" << std::endl;
    } else {
      exitMessage_ = line;
      std::istringstream iss(line);
      iss >> exitCode_ >> trash;
      edm::LogInfo("MillePedeFileReader")
          << " Pede exit code is: " << exitCode_ << " (" << exitMessage_ << ")" << std::endl;
    }
  } else {
    edm::LogError("MillePedeFileReader") << "Could not read millepede end-file.";
    exitMessage_ = "no exit code found";
  }
}

void MillePedeFileReader ::readMillePedeLogFile() {
  std::ifstream logFile;
  logFile.open(millePedeLogFile_.c_str());

  if (logFile.is_open()) {
    edm::LogInfo("MillePedeFileReader") << "Reading millepede log-file";
    std::string line;

    while (getline(logFile, line)) {
      std::string Nrec_string = "NREC =";
      std::string Binaries_string = "C_binary";

      if (line.find(Nrec_string) != std::string::npos) {
        std::istringstream iss(line);
        std::string trash;
        iss >> trash >> trash >> Nrec_;

        if (Nrec_ < theThresholds_->getNrecords()) {
          edm::LogInfo("MillePedeFileReader")
              << "Number of records used " << theThresholds_->getNrecords() << std::endl;
          updateDB_ = false;
        }
      }

      if (line.find(Binaries_string) != std::string::npos) {
        binariesAmount_ += 1;
      }
    }
  } else {
    edm::LogError("MillePedeFileReader") << "Could not read millepede log-file.";

    updateDB_ = false;
    Nrec_ = 0;
  }
}

void MillePedeFileReader ::readMillePedeResultFile() {
  // cutoffs by coordinate and by alignable
  std::map<std::string, std::array<float, 6> > cutoffs_;
  std::map<std::string, std::array<float, 6> > significances_;
  std::map<std::string, std::array<float, 6> > thresholds_;
  std::map<std::string, std::array<float, 6> > errors_;

  AlignableObjectId alignableObjectId{AlignableObjectId::Geometry::General};

  std::vector<std::string> alignables_ = theThresholds_->getAlignableList();
  for (auto& ali : alignables_) {
    cutoffs_[ali] = theThresholds_->getCut(ali);
    significances_[ali] = theThresholds_->getSigCut(ali);
    thresholds_[ali] = theThresholds_->getMaxMoveCut(ali);
    errors_[ali] = theThresholds_->getMaxErrorCut(ali);
  }

  updateDB_ = false;
  vetoUpdateDB_ = false;
  std::ifstream resFile;
  resFile.open(millePedeResFile_.c_str());
  
  std::vector<int> counts = {0,0,0,0,0,0};

  if (resFile.is_open()) {
    edm::LogInfo("MillePedeFileReader") << "Reading millepede result-file";

    std::string line;
    getline(resFile, line);  // drop first line

    while (getline(resFile, line)) {
      std::istringstream iss(line);

      std::vector<std::string> tokens;
      std::string token;
      while (iss >> token) {
        tokens.push_back(token);
      }
      
      auto alignableLabel = std::stoul(tokens[0]);
      const auto alignable = pedeLabeler_->alignableFromLabel(alignableLabel);
      align::ID id = alignable->id();
      auto det = getHLS(alignable);
      int detIndex = static_cast<int>(det);
      //~ std::cout<<detIndex<<"   "<<getIndexForHG(id, detIndex)<<std::endl;
      

      if (tokens.size() > 4 /*3*/) {
        //~ auto alignableLabel = std::stoul(tokens[0]);
        auto alignableIndex = alignableLabel % 10 - 1;
        //~ const auto alignable = pedeLabeler_->alignableFromLabel(alignableLabel);
        const auto paramNum = pedeLabeler_->paramNumFromLabel(alignableLabel);
        align::StructureType type = alignable->alignableObjectId();
        align::ID id = alignable->id();

        double ObsMove = std::stof(tokens[3]) * multiplier_[alignableIndex];
        double ObsErr = std::stof(tokens[4]) * multiplier_[alignableIndex];

        //~ auto det = getHLS(alignable);
        //~ int detIndex = static_cast<int>(det);
        auto coord = static_cast<AlignPCLThresholds::coordType>(alignableIndex);
        std::string detLabel = getStringFromHLS(det);

        if (det != PclHLS::NotInPCL) {
          if (det != PclHLS::TPBLadder && det != PclHLS::TPEPanel){
            switch (coord) {
              case AlignPCLThresholds::X:
                Xobs_[detIndex] = ObsMove;
                XobsErr_[detIndex] = ObsErr;
                break;
              case AlignPCLThresholds::Y:
                Yobs_[detIndex] = ObsMove;
                YobsErr_[detIndex] = ObsErr;
                break;
              case AlignPCLThresholds::Z:
                Zobs_[detIndex] = ObsMove;
                ZobsErr_[detIndex] = ObsErr;
                break;
              case AlignPCLThresholds::theta_X:
                tXobs_[detIndex] = ObsMove;
                tXobsErr_[detIndex] = ObsErr;
                break;
              case AlignPCLThresholds::theta_Y:
                tYobs_[detIndex] = ObsMove;
                tYobsErr_[detIndex] = ObsErr;
                break;
              case AlignPCLThresholds::theta_Z:
                tZobs_[detIndex] = ObsMove;
                tZobsErr_[detIndex] = ObsErr;
                break;
              default:
                edm::LogError("MillePedeFileReader") << "Currently not able to handle DOF " << coord << std::endl;
                break;
            }
          } else {
            auto hgIndex = getIndexForHG(id, detIndex);
            switch (coord) {
              case AlignPCLThresholds::X:
                Xobs_HG_[hgIndex-1] = ObsMove;
                XobsErr_HG_[hgIndex-1] = ObsErr;
                break;
              case AlignPCLThresholds::Y:
                Yobs_HG_[hgIndex-1] = ObsMove;
                YobsErr_HG_[hgIndex-1] = ObsErr;
                break;
              case AlignPCLThresholds::Z:
                Zobs_HG_[hgIndex-1] = ObsMove;
                ZobsErr_HG_[hgIndex-1] = ObsErr;
                break;
              case AlignPCLThresholds::theta_X:
                tXobs_HG_[hgIndex-1] = ObsMove;
                tXobsErr_HG_[hgIndex-1] = ObsErr;
                break;
              case AlignPCLThresholds::theta_Y:
                tYobs_HG_[hgIndex-1] = ObsMove;
                tYobsErr_HG_[hgIndex-1] = ObsErr;
                break;
              case AlignPCLThresholds::theta_Z:
                tZobs_HG_[hgIndex-1] = ObsMove;
                tZobsErr_HG_[hgIndex-1] = ObsErr;
                break;
              default:
                edm::LogError("MillePedeFileReader") << "Currently not able to handle DOF " << coord << std::endl;
                break;
            }
          }
            
        } else {
          edm::LogError("MillePedeFileReader")
              << "Currently not able to handle coordinate: " << coord << " (" << paramNum << ")  "
              << Form(" %s with ID %d (subdet %d)", alignableObjectId.idToString(type), id, DetId(id).subdetId())
              << std::endl;
          continue;
        }

        edm::LogVerbatim("MillePedeFileReader")
            << " alignableLabel: " << alignableLabel << " with alignableIndex " << alignableIndex << " detIndex "
            << detIndex << "\n"
            << " i.e. detLabel: " << detLabel << " (" << coord << ")\n"
            << " has movement: " << ObsMove << " +/- " << ObsErr << "\n"
            << " cutoff (cutoffs_[" << detLabel << "][" << coord << "]): " << cutoffs_[detLabel][alignableIndex] << "\n"
            << " significance (significances_[" << detLabel << "][" << coord
            << "]): " << significances_[detLabel][alignableIndex] << "\n"
            << " error thresolds (errors_[" << detLabel << "][" << coord << "]): " << errors_[detLabel][alignableIndex]
            << "\n"
            << " max movement (thresholds_[" << detLabel << "][" << coord
            << "]): " << thresholds_[detLabel][alignableIndex] << "\n"
            << "=============" << std::endl;

        if (std::abs(ObsMove) > thresholds_[detLabel][alignableIndex]) {
          edm::LogWarning("MillePedeFileReader") << "Aborting payload creation."
                                                 << " Exceeding maximum thresholds for movement: " << std::abs(ObsMove)
                                                 << " for" << detLabel << "(" << coord << ")";
          updateBits_.set(0);
          vetoUpdateDB_ = true;
          continue;

        } else if (std::abs(ObsMove) > cutoffs_[detLabel][alignableIndex]) {
          updateBits_.set(1);

          if (std::abs(ObsErr) > errors_[detLabel][alignableIndex]) {
            edm::LogWarning("MillePedeFileReader") << "Aborting payload creation."
                                                   << " Exceeding maximum thresholds for error: " << std::abs(ObsErr)
                                                   << " for" << detLabel << "(" << coord << ")";
            updateBits_.set(2);
            vetoUpdateDB_ = true;
            continue;
          } else {
            if (std::abs(ObsMove / ObsErr) < significances_[detLabel][alignableIndex]) {
              updateBits_.set(3);
              continue;
            }
          }
          updateDB_ = true;
          edm::LogInfo("MillePedeFileReader")
              << "This correction: " << ObsMove << "+/-" << ObsErr << " for " << detLabel << "(" << coord
              << ") will trigger a new Tracker Alignment payload!";
        }
      }
    }
  } else {
    edm::LogError("MillePedeFileReader") << "Could not read millepede result-file.";

    updateDB_ = false;
    Nrec_ = 0;
  }
}

MillePedeFileReader::PclHLS MillePedeFileReader ::getHLS(const Alignable* alignable) {
  if (!alignable)
    return PclHLS::NotInPCL;

  const auto& tns = pedeLabeler_->alignableTracker()->trackerNameSpace();

  switch (alignable->alignableObjectId()) {
    case align::TPBHalfBarrel:
      switch (tns.tpb().halfBarrelNumber(alignable->id())) {
        case 1:
          return PclHLS::TPBHalfBarrelXminus;
        case 2:
          return PclHLS::TPBHalfBarrelXplus;
        default:
          throw cms::Exception("LogicError") << "@SUB=MillePedeFileReader::getHLS\n"
                                             << "Found a pixel half-barrel number that should not exist: "
                                             << tns.tpb().halfBarrelNumber(alignable->id());
      }
    case align::TPEHalfCylinder:
      switch (tns.tpe().endcapNumber(alignable->id())) {
        case 1:
          switch (tns.tpe().halfCylinderNumber(alignable->id())) {
            case 1:
              return PclHLS::TPEHalfCylinderXminusZminus;
            case 2:
              return PclHLS::TPEHalfCylinderXplusZminus;
            default:
              throw cms::Exception("LogicError") << "@SUB=MillePedeFileReader::getHLS\n"
                                                 << "Found a pixel half-cylinder number that should not exist: "
                                                 << tns.tpe().halfCylinderNumber(alignable->id());
          }
        case 2:
          switch (tns.tpe().halfCylinderNumber(alignable->id())) {
            case 1:
              return PclHLS::TPEHalfCylinderXminusZplus;
            case 2:
              return PclHLS::TPEHalfCylinderXplusZplus;
            default:
              throw cms::Exception("LogicError") << "@SUB=MillePedeFileReader::getHLS\n"
                                                 << "Found a pixel half-cylinder number that should not exist: "
                                                 << tns.tpe().halfCylinderNumber(alignable->id());
          }
        default:
          throw cms::Exception("LogicError")
              << "@SUB=MillePedeFileReader::getHLS\n"
              << "Found a pixel endcap number that should not exist: " << tns.tpe().endcapNumber(alignable->id());
      }
    case align::TPBLadder:
      return PclHLS::TPBLadder;
    case align::TPEPanel:
      return PclHLS::TPEPanel;
    default:
      return PclHLS::NotInPCL;
  }
}

std::string MillePedeFileReader::getStringFromHLS(MillePedeFileReader::PclHLS HLS) {
  switch (HLS) {
    case PclHLS::TPBHalfBarrelXminus:
      return "TPBHalfBarrelXminus";
    case PclHLS::TPBHalfBarrelXplus:
      return "TPBHalfBarrelXplus";
    case PclHLS::TPEHalfCylinderXminusZminus:
      return "TPEHalfCylinderXminusZminus";
    case PclHLS::TPEHalfCylinderXplusZminus:
      return "TPEHalfCylinderXplusZminus";
    case PclHLS::TPEHalfCylinderXminusZplus:
      return "TPEHalfCylinderXminusZplus";
    case PclHLS::TPEHalfCylinderXplusZplus:
      return "TPEHalfCylinderXplusZplus";
    case PclHLS::TPBLadder:
      return "TPBLadder";
    case PclHLS::TPEPanel:
      return "TPEPanel";
    default:
      //return "NotInPCL";
      throw cms::Exception("LogicError")
          << "@SUB=MillePedeFileReader::getStringFromHLS\n"
          << "Found an alignable structure not possible to map in the default AlignPCLThresholds partitions";
  }
}

int MillePedeFileReader::getIndexForHG(align::ID id, int detIndex) {
  const auto& tns = pedeLabeler_->alignableTracker()->trackerNameSpace();
  switch (detIndex) {
    case 6:
      switch (tns.tpb().layerNumber(id)) {
        case 1:
          return (tns.tpb().halfBarrelNumber(id) == 1) ? tns.tpb().ladderNumber(id) : tns.tpb().ladderNumber(id)+6;
        case 2:
          return (tns.tpb().halfBarrelNumber(id) == 1) ? (tns.tpb().ladderNumber(id)+12) : (tns.tpb().ladderNumber(id)+12)+14;
        case 3:
          return (tns.tpb().halfBarrelNumber(id) == 1) ? (tns.tpb().ladderNumber(id)+40) : (tns.tpb().ladderNumber(id)+40)+22;
        case 4:
          return (tns.tpb().halfBarrelNumber(id) == 1) ? (tns.tpb().ladderNumber(id)+84) : (tns.tpb().ladderNumber(id)+84)+32;
      }
    case 7:
      switch ((tns.tpe().endcapNumber(id)==1) ? -1*tns.tpe().halfDiskNumber(id) : tns.tpe().halfDiskNumber(id)) {
        case -3:
          return (tns.tpe().halfCylinderNumber(id) == 1) ? (tns.tpe().bladeNumber(id)*2-(tns.tpe().panelNumber(id)%2))+148 : (tns.tpe().bladeNumber(id)*2-(tns.tpe().panelNumber(id)%2))+148+56;
        case -2:
          return (tns.tpe().halfCylinderNumber(id) == 1) ? (tns.tpe().bladeNumber(id)*2-(tns.tpe().panelNumber(id)%2))+148+112 : (tns.tpe().bladeNumber(id)*2-(tns.tpe().panelNumber(id)%2))+148+112+56;
        case -1:
          return (tns.tpe().halfCylinderNumber(id) == 1) ? (tns.tpe().bladeNumber(id)*2-(tns.tpe().panelNumber(id)%2))+148+224 : (tns.tpe().bladeNumber(id)*2-(tns.tpe().panelNumber(id)%2))+148+224+56;
        case 1:
          return (tns.tpe().halfCylinderNumber(id) == 1) ? (tns.tpe().bladeNumber(id)*2-(tns.tpe().panelNumber(id)%2))+148+336 : (tns.tpe().bladeNumber(id)*2-(tns.tpe().panelNumber(id)%2))+148+336+56;
        case 2:
          return (tns.tpe().halfCylinderNumber(id) == 1) ? (tns.tpe().bladeNumber(id)*2-(tns.tpe().panelNumber(id)%2))+148+448 : (tns.tpe().bladeNumber(id)*2-(tns.tpe().panelNumber(id)%2))+148+448+56;
        case 3:
          return (tns.tpe().halfCylinderNumber(id) == 1) ? (tns.tpe().bladeNumber(id)*2-(tns.tpe().panelNumber(id)%2))+148+560 : (tns.tpe().bladeNumber(id)*2-(tns.tpe().panelNumber(id)%2))+148+560+56;
      }
    default:
      return -200;
  } 
}

//=============================================================================
//===   STATIC CONST MEMBER DEFINITION                                      ===
//=============================================================================
constexpr std::array<double, 6> MillePedeFileReader::multiplier_;
