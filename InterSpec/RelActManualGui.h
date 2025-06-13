#include <any>
#ifndef RelActManualGui_h
#define RelActManualGui_h
/* InterSpec: an application to analyze spectral gamma radiation data.
 
 Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC
 (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
 Government retains certain rights in this software.
 For questions contact William Johnson via email at wcjohns@sandia.gov, or
 alternative emails of interspec@sandia.gov.
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "InterSpec_config.h"

#include <map>
#include <deque>
#include <string>
#include <memory>
#include <utility>

#include <Wt/WContainerWidget>

// Forward declarations
class PeakModel;
class AuxWindow;
class InterSpec;
class RelEffChart;
class NativeFloatSpinBox;
struct RelEffShieldState;
class RelEffShieldWidget;
class RowStretchTreeView;
class DetectorPeakResponse;

namespace Wt
{
  class WMenu;
  class WCheckBox;
  class WTableRow;
  class WComboBox;
  class WResource;
  class WGridLayout;
}//namespace Wt

namespace rapidxml
{
  template<class Ch> class xml_node;
}//namespace rapidxml

namespace SpecUtils
{
  class Measurement;
}

namespace RelActCalc
{
  enum class RelEffEqnForm : int;
}

namespace RelActCalcManual
{
  struct RelEffInput;
  struct RelEffSolution;
  namespace PeakCsvInput
  {
    enum class NucDataSrc : int;
  }
}

/**

 Future work on this tool:
 - Add Pu240/U236 by correlation.  Also, group Am241 into Pu-mass
 - Add in Pu gamma BR from ICRP/FRAM/etc, as user selectable options
 - If only two nuclides, or U, or Pu, then put mass/activity fraction on Rel. Eff. chart
 - Indicate effect of background subtraction somehow (maybe list this in a table).
 - Add capability to fix ratios of nuclide/masses - this way you can easily show data is
 inconsistent with a particular hypothesis; although when something is fixed, it needs to be
 clearly shown on the results/report that it is fixed.
 - Add in "truth-level" hooks for testing; can maybe just be isotope/mass ratios at first
 - Add in saving Peak/Nuclide info, and current results to state XML, so this way it can easily be
 used for regression testing; this maybe ties into the "truth-level" bullet above.
 - Add in plot that shows deviations, or pulls, or whatever.
 */

class RelActManualGui : public Wt::WContainerWidget
{
public:
  struct GuiState; //Forward declaration
  
  // The additional uncertainty is a bit confusing, so to not overwhelm users, or have them worry
  //  about it to much, we'll provide a limited number of options.
  enum class AddUncert : int
  {
    Unweighted, StatOnly, OnePercent, FivePercent, TenPercent, TwentyFivePercent,
    FiftyPercent, SeventyFivePercent, OneHundredPercent, NumAddUncert
  };//enum class AddUncert
  
  static const char *to_str( const AddUncert val );
  
public:
  RelActManualGui( InterSpec *viewer, Wt::WContainerWidget *parent = nullptr );
  
  void init();

  std::shared_ptr<const RelActCalcManual::RelEffSolution> currentSolution();
  
  /** creates (and returns) a node "RelActManualGui" under `parent` which contains the XML for
   this object.
   
   Note that this XML doesnt include current solution information, or peak information (or hence
   nuclide information).
   */
  ::rapidxml::xml_node<char> *serialize( ::rapidxml::xml_node<char> *parent );
  
  /** takes in a "RelActManualGui" node and sets the state of this object to match the XML.
  
   Throws when it runs into an unexpected situation, or invalid parent_node.
  */
  void deSerialize( const ::rapidxml::xml_node<char> *parent_node );
  
protected:
  virtual void render( Wt::WFlags<Wt::RenderFlag> flags );
  
  void calculateSolution();
  void updateGuiWithError( std::shared_ptr<Wt::WString> error_msg );
  void updateGuiWithResults( std::shared_ptr<RelActCalcManual::RelEffSolution> answer );
  
  /** Handles hiding/showing and initializing widgets for the (now) currently set relative efficiency equation type.
   @param user_action If true, for when the user causes this action through the GUI, will add an undo/redo step.
   */
  void relEffEqnFormChanged( const bool user_action );
  void relEffEqnOrderChanged();
  void physModelUseHoerlChanged();
  void nucDataSrcChanged();
  void matchToleranceChanged();
  void addUncertChanged();
  void backgroundSubtractChanged();
  void initPhysicalModelAttenShieldWidgets();
  //void resultTabChanged(); //Creates a undo/redo cycle, so not currently tracking
  
  /** Marks that nuclides needs to be updated, and schedules rending; but doesnt do any real work */
  void handlePeaksChanged();
  
  /** Does the actual work of updating nuclide display.  Inserts/removes nuclide displays, sets
   nuclide ages hidden/shown, and hides/shows the nuclear dataset options.
   */
  void updateNuclides();
  
  /** Hides/shows the background subtract option. */
  void updateSpectrumBasedOptions();
  
  /** Marks that nuclides, options, and calculation, need to be updated, and schedules render. */
  void displayedSpectrumChanged();
  
  RelActCalc::RelEffEqnForm relEffEqnForm() const;
  size_t relEffEqnOrder() const;
  RelActCalcManual::PeakCsvInput::NucDataSrc nucDataSrc() const;
  
  void handlePhysicalModelShieldChanged();
  
protected:
  // Some items for adding undo/redo steps
  
  std::shared_ptr<const GuiState> getGuiState() const;
  void setGuiState( const GuiState &state );
  void addUndoRedoStep( const std::shared_ptr<const GuiState> &state );
  
  /** An enum to list action items we may need to take inside the #render function.  \sa m_renderFlags */
  enum RenderActions
  {
    UpdateNuclides = 0x01,
    UpdateCalc = 0x02,
    UpdateSpectrumOptions = 0x04,
    AddUndoRedoStep = 0x08
  };//enum D3RenderActions
  
  Wt::WFlags<RelActManualGui::RenderActions> m_renderFlags;
  
  
  std::shared_ptr<RelActCalcManual::RelEffSolution> m_currentSolution;
  
  InterSpec *m_interspec;
  
  Wt::WGridLayout *m_layout;
  Wt::WContainerWidget *m_optionsColumn;
  
  Wt::WComboBox *m_relEffEqnForm;
  
  Wt::WComboBox *m_relEffEqnOrder;
  Wt::WTableRow *m_relEffEqnOrderHolder;

  Wt::WCheckBox *m_physModelUseHoerl;
  Wt::WTableRow *m_physModelUseHoerlHolder;
  
  Wt::WTableRow *m_nucDataSrcHolder;
  Wt::WComboBox *m_nucDataSrc;
  
  NativeFloatSpinBox *m_matchTolerance;
  
  Wt::WComboBox *m_addUncertainty;
  
  Wt::WCheckBox *m_backgroundSubtract;
  Wt::WTableRow *m_backgroundSubtractHolder;
  
#if( BUILD_AS_OSX_APP || IOS )
  Wt::WAnchor *m_downloadHtmlReport;
#else
  Wt::WPushButton *m_downloadHtmlReport;
#endif
  Wt::WResource *m_htmlResource;
  
  Wt::WContainerWidget *m_peakTableColumn;
  PeakModel *m_peakModel;
  RowStretchTreeView *m_peakTable;
  
  /// All entries in this next <div> will be of class ManRelEffNucDisp
  Wt::WContainerWidget *m_nuclidesDisp;
  Wt::WText *m_nucColumnTitle;

  Wt::WContainerWidget *m_physicalModelShields;
  RelEffShieldWidget *m_selfAttenShield;
  Wt::WContainerWidget *m_extAttenShields;


  /// Keep a cache of nuclide ages around incase the user removes a nuclide, but adds it in later.
  std::map<std::string,double> m_nucAge;
  /// Keep a cache of if we should decay-correct the nuclide incase the user removes a nuclide, but adds it in later.
  std::map<std::string,bool> m_nucDecayCorrect;
  
  Wt::WMenu *m_resultMenu;
  
  RelEffChart *m_chart;
  Wt::WContainerWidget *m_results;

  /** A default detector to use, only if using `RelActCalc::RelEffEqnForm::FramPhysicalModel` 
   * and foreground detector is not found. 
  */
  std::shared_ptr<const DetectorPeakResponse> m_defaultDetector;
  
  /** The GUI state that is updated inside each call to #render. Will be used to add an undo/redo step
   if the #RenderActions::AddUndoRedoStep flag is set.
   */
  std::shared_ptr<const GuiState> m_currentGuiState;
  
public:
  /** A "lightweight" GUI state, for undo/redo steps, to keep in memory. */
  struct GuiState
  {
    RelActCalc::RelEffEqnForm m_relEffEqnFormIndex;
    int m_relEffEqnOrderIndex = 0;
    RelActCalcManual::PeakCsvInput::NucDataSrc m_nucDataSrcIndex;;
    float m_matchToleranceValue; //In FWHM
    RelActManualGui::AddUncert m_addUncertIndex;
    int m_resultTab = 0;
    bool m_backgroundSubtract = false;
    std::map<std::string,double> nucAge;
    std::map<std::string,bool> nucDecayCorrect;
    //std::vector<std::tuple<std::string,double,bool>> m_nucAgesAndDecayCorrect; //Previous to 20250115, we used this, instead of above two maps; it only tracks visible/current nuclide
    
    bool m_physModelUseHoerl = true;
    std::unique_ptr<RelEffShieldState> m_selfAttenShield;
    std::vector<std::unique_ptr<RelEffShieldState>> m_externalShields;
    
    ::rapidxml::xml_node<char> *serialize( ::rapidxml::xml_node<char> *parent_node ) const;
    void deSerialize( const ::rapidxml::xml_node<char> *base_node );
    
    bool operator==( const GuiState &rhs ) const;
  };//struct GuiState
  
  
  /** Struct that represents kinda raw information needed, to use to prepare the input for calculation.
   
   This information requires to be in the Wt GUI main thread to collect
   */
  struct RelActCalcRawInput
  {
    std::shared_ptr<const GuiState> state;
    std::shared_ptr<const SpecUtils::Measurement> fore_spec, back_spec;
    std::deque<std::shared_ptr<const PeakDef>> peaks, background_peaks;
    std::shared_ptr<const DetectorPeakResponse> detector;
  };//struct RelActCalcRawInput

  /** Collects together the 'raw' information that is needed to then prepair the actual actual input for computation.
   
   Must be called from the Wt GUI main thread.
   
   Throws exception on error.
   */
  RelActCalcRawInput get_raw_info_for_calc_input();
  
  /** Throws exception on error. */
  static void prepare_calc_input( const RelActCalcRawInput &input,
                                 MaterialDB *materialDB,
                                 RelActCalcManual::RelEffInput &output );
  
public:
  static const int sm_xmlSerializationMajorVersion;
  static const int sm_xmlSerializationMinorVersion;
};//class RelActManualGui


#endif //RelActManualGui_h
