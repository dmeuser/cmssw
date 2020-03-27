#ifndef ALIGNMENT_MILLEPEDEALIGNMENTALGORITHM_INTERFACE_MILLEPEDEFILEREADER_H_
#define ALIGNMENT_MILLEPEDEALIGNMENTALGORITHM_INTERFACE_MILLEPEDEFILEREADER_H_

/*** system includes ***/
#include <array>
#include <string>
#include <iostream>

/*** core framework functionality ***/
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

/*** Alignment ***/
#include "Alignment/MillePedeAlignmentAlgorithm/interface/PedeLabelerBase.h"
#include "CondFormats/PCLConfig/interface/AlignPCLThresholds.h"

struct mpPCLresults {
private:
  bool m_isDBUpdated;
  bool m_isDBUpdateVetoed;
  int m_nRecords;
  int m_exitCode;
  std::string m_exitMessage;
  std::bitset<4> m_updateBits;

public:
  mpPCLresults(bool isDBUpdated,
               bool isDBUpdateVetoed,
               int nRecords,
               int exitCode,
               std::string exitMessage,
               std::bitset<4> updateBits)
      : m_isDBUpdated(isDBUpdated),
        m_isDBUpdateVetoed(isDBUpdateVetoed),
        m_nRecords(nRecords),
        m_exitCode(exitCode),
        m_exitMessage(exitMessage),
        m_updateBits(updateBits) {}

  const bool getDBUpdated() { return m_isDBUpdated; }
  const bool getDBVetoed() { return m_isDBUpdateVetoed; }
  const bool exceedsThresholds() { return m_updateBits.test(0); }
  const bool exceedsCutoffs() { return m_updateBits.test(1); }
  const bool exceedsMaxError() { return m_updateBits.test(2); }
  const bool belowSignificance() { return m_updateBits.test(3); }
  const int getNRecords() { return m_nRecords; }
  const int getExitCode() { return m_exitCode; }
  const std::string getExitMessage() { return m_exitMessage; }

  void print() {
    edm::LogInfo("MillePedeFileReader") << " is DB updated: " << m_isDBUpdated
                                        << " is DB update vetoed: " << m_isDBUpdateVetoed << " nRecords: " << m_nRecords
                                        << " exitCode: " << m_exitCode << " (" << m_exitMessage << ")" << std::endl;
  }
};

class MillePedeFileReader {
  //========================== PUBLIC METHODS ==================================
public:  //====================================================================
  explicit MillePedeFileReader(const edm::ParameterSet&,
                               const std::shared_ptr<const PedeLabelerBase>&,
                               const std::shared_ptr<const AlignPCLThresholds>&);

  virtual ~MillePedeFileReader() = default;

  void read();
  bool storeAlignments();

  const std::array<double, 6>& getXobs() const { return Xobs_; }
  const std::array<double, 6>& getXobsErr() const { return XobsErr_; }
  const std::array<double, 6>& getTXobs() const { return tXobs_; }
  const std::array<double, 6>& getTXobsErr() const { return tXobsErr_; }

  const std::array<double, 6>& getYobs() const { return Yobs_; }
  const std::array<double, 6>& getYobsErr() const { return YobsErr_; }
  const std::array<double, 6>& getTYobs() const { return tYobs_; }
  const std::array<double, 6>& getTYobsErr() const { return tYobsErr_; }

  const std::array<double, 6>& getZobs() const { return Zobs_; }
  const std::array<double, 6>& getZobsErr() const { return ZobsErr_; }
  const std::array<double, 6>& getTZobs() const { return tZobs_; }
  const std::array<double, 6>& getTZobsErr() const { return tZobsErr_; }
  
  const std::array<double, 820>& getXobs_HG() const { return Xobs_HG_; }
  const std::array<double, 820>& getXobsErr_HG() const { return XobsErr_HG_; }
  const std::array<double, 820>& getTXobs_HG() const { return tXobs_HG_; }
  const std::array<double, 820>& getTXobsErr_HG() const { return tXobsErr_HG_; }

  const std::array<double, 820>& getYobs_HG() const { return Yobs_HG_; }
  const std::array<double, 820>& getYobsErr_HG() const { return YobsErr_HG_; }
  const std::array<double, 820>& getTYobs_HG() const { return tYobs_HG_; }
  const std::array<double, 820>& getTYobsErr_HG() const { return tYobsErr_HG_; }

  const std::array<double, 820>& getZobs_HG() const { return Zobs_HG_; }
  const std::array<double, 820>& getZobsErr_HG() const { return ZobsErr_HG_; }
  const std::array<double, 820>& getTZobs_HG() const { return tZobs_HG_; }
  const std::array<double, 820>& getTZobsErr_HG() const { return tZobsErr_HG_; }

  const AlignPCLThresholds::threshold_map getThresholdMap() const { return theThresholds_.get()->getThreshold_Map(); }

  const int binariesAmount() const { return binariesAmount_; }

  const mpPCLresults getResults() const {
    return mpPCLresults(updateDB_, vetoUpdateDB_, Nrec_, exitCode_, exitMessage_, updateBits_);
  }

private:
  //========================= PRIVATE ENUMS ====================================
  //============================================================================

  enum class PclHLS : int {
    NotInPCL = -1,
    TPEHalfCylinderXplusZminus = 0,
    TPEHalfCylinderXminusZminus = 1,
    TPBHalfBarrelXplus = 2,
    TPBHalfBarrelXminus = 3,
    TPEHalfCylinderXplusZplus = 4,
    TPEHalfCylinderXminusZplus = 5,
    TPBLadder = 6,
    TPEPanel = 7,
  };

  //========================= PRIVATE METHODS ==================================
  //============================================================================

  void readMillePedeEndFile();
  void readMillePedeLogFile();
  void readMillePedeResultFile();
  PclHLS getHLS(const Alignable*);
  std::string getStringFromHLS(PclHLS HLS);
  int getIndexForHG(align::ID id, int detIndex);

  //========================== PRIVATE DATA ====================================
  //============================================================================

  // pede labeler plugin
  const std::shared_ptr<const PedeLabelerBase> pedeLabeler_;

  // thresholds from DB
  const std::shared_ptr<const AlignPCLThresholds> theThresholds_;

  // file-names
  const std::string millePedeEndFile_;
  const std::string millePedeLogFile_;
  const std::string millePedeResFile_;

  // conversion factors: cm to um & rad to urad
  static constexpr std::array<double, 6> multiplier_ = {{10000.,      // X
                                                         10000.,      // Y
                                                         10000.,      // Z
                                                         1000000.,    // tX
                                                         1000000.,    // tY
                                                         1000000.}};  // tZ

  bool updateDB_{false};
  bool vetoUpdateDB_{false};

  // stores in a compact format the 4 decisions:
  // 1st bit: exceeds maximum thresholds
  // 2nd bit: exceeds cutoffs (significant movement)
  // 3rd bit: exceeds maximum errors
  // 4th bit: is below the significance
  std::bitset<4> updateBits_;

  // pede binaries available
  int binariesAmount_{0};

  int Nrec_{0};
  int exitCode_{-1};
  std::string exitMessage_{""};

  std::array<double, 6> Xobs_ = {{0., 0., 0., 0., 0., 0.}};
  std::array<double, 6> XobsErr_ = {{0., 0., 0., 0., 0., 0.}};
  std::array<double, 6> tXobs_ = {{0., 0., 0., 0., 0., 0.}};
  std::array<double, 6> tXobsErr_ = {{0., 0., 0., 0., 0., 0.}};

  std::array<double, 6> Yobs_ = {{0., 0., 0., 0., 0., 0.}};
  std::array<double, 6> YobsErr_ = {{0., 0., 0., 0., 0., 0.}};
  std::array<double, 6> tYobs_ = {{0., 0., 0., 0., 0., 0.}};
  std::array<double, 6> tYobsErr_ = {{0., 0., 0., 0., 0., 0.}};

  std::array<double, 6> Zobs_ = {{0., 0., 0., 0., 0., 0.}};
  std::array<double, 6> ZobsErr_ = {{0., 0., 0., 0., 0., 0.}};
  std::array<double, 6> tZobs_ = {{0., 0., 0., 0., 0., 0.}};
  std::array<double, 6> tZobsErr_ = {{0., 0., 0., 0., 0., 0.}};
  
  std::array<double, 820> Xobs_HG_ = std::array<double, 820>();
  std::array<double, 820> XobsErr_HG_ = std::array<double, 820>();
  std::array<double, 820> tXobs_HG_ = std::array<double, 820>();
  std::array<double, 820> tXobsErr_HG_ = std::array<double, 820>();

  std::array<double, 820> Yobs_HG_ = std::array<double, 820>();
  std::array<double, 820> YobsErr_HG_ = std::array<double, 820>();
  std::array<double, 820> tYobs_HG_ = std::array<double, 820>();
  std::array<double, 820> tYobsErr_HG_ = std::array<double, 820>();

  std::array<double, 820> Zobs_HG_ = std::array<double, 820>();
  std::array<double, 820> ZobsErr_HG_ = std::array<double, 820>();
  std::array<double, 820> tZobs_HG_ = std::array<double, 820>();
  std::array<double, 820> tZobsErr_HG_ = std::array<double, 820>();
};

const std::array<std::string, 8> coord_str = {{"X", "Y", "Z", "theta_X", "theta_Y", "theta_Z", "extra_DOF", "none"}};
inline std::ostream& operator<<(std::ostream& os, const AlignPCLThresholds::coordType& c) {
  if (c >= AlignPCLThresholds::endOfTypes || c < 0)
    return os << "unrecongnized coordinate";
  return os << coord_str[c];
}

#endif /* ALIGNMENT_MILLEPEDEALIGNMENTALGORITHM_INTERFACE_MILLEPEDEFILEREADER_H_ */
