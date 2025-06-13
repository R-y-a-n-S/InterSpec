#include <any>
#ifndef DoseCalcWidget_h
#define DoseCalcWidget_h
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

#include <string>
#include <vector>
#include <memory>

#include <Wt/WContainerWidget>

#include "InterSpec/AuxWindow.h"

class MaterialDB;
class DoseCalcWidget;
class InterSpec;
class ShieldingSelect;
class GadrasScatterTable;
class NuclideSourceEnter;


namespace Wt
{
  class WMenu;
  class WText;
  class WComboBox;
  class WLineEdit;
  class WPushButton;
  class WGridLayout;
  class WButtonGroup;
  class WStackedWidget;
  class WSuggestionPopup;
}//namespace Wt

/* Todo as of 20151001
     -Add g, kg for self attenuating sources
     -Continuum groups for self attenuating sources
     -Continuum normalization
     -Neutron groups for a number of sources
 */


class DoseCalcWindow : public AuxWindow
{
public:
  DoseCalcWindow( MaterialDB *materialDB,
                  Wt::WSuggestionPopup *materialSuggestion,
                  InterSpec* viewer );
  
  virtual ~DoseCalcWindow();

  DoseCalcWidget *tool();
  
protected:
  DoseCalcWidget *m_dose;
};//class DoseCalcWindow


/** Calculates:
 * - Source to dose at a given distance and shielding.
 * - Dose to source activity at a given distance and shielding.
 * - Distance, given source, dose, and shielding.
 * - Shielding, given source, distance and dose.
 * - Gives stay time for given dose in each case.
*/
class DoseCalcWidget : public Wt::WContainerWidget
{
public:
  
  DoseCalcWidget( MaterialDB *materialDB,
                  Wt::WSuggestionPopup *materialSuggestion,
                  InterSpec *specViewer,
                  Wt::WContainerWidget *parent = 0 );
  
  
  virtual ~DoseCalcWidget();
  
  /** Performs some very basic runtime checks to make sure calculations are
   half-way reasonable.
   Throws exception with a descriptive message if there is an issue.
   */
  static void runtime_sanity_checks( const GadrasScatterTable * const scatter );
  
  /** Handles receiving a "deep-link" url starting with "interspec://dose/act?...".
   
   Example URIs:
   - "interspec://dose/act?ver=1&nuc=u238&dose=1.1ur/h&dist=100cm&..."
   
   @param query_str The query portion of the URI.  So for example, if the URI has a value of
          "interspec://dose/act?nuc=u238...", then this string would be "nuc=u238...".
          This string is is in standard URL format of "key1=value1&key2=value2&..." with ordering not mattering.
          Capitalization is not important.
          Assumes the string passed in has alaready been url-decoded.
          If not a valid path or query_str, throws exception.
   */
  void handleAppUrl( std::string path, std::string query_str );
  
  /** Encodes current tool state to app-url format.  Returned string does not include the
   "interspec://" protocol, or "dose" authority; so will look something like "dist?tab=act&nuc=u238&dose=1.1ur/h&dist=100cm&...",
   The path part of the URI specifies tab the tool is on.
   and it will not be url-encoded.
   */
  std::string encodeStateToUrl() const;
protected:
  
  enum Quantity{ Dose, Activity, Distance, Shielding, NumQuantity };
  
  void init();
  
  void handleQuantityClick( const Quantity q );
  void handleSourceTypeChange();
  void updateResult();
  void updateStayTime();
  
  void updateResultForGammaSource();
  void updateResultForNeutronSource();
  
  void checkAndWarnForBrehmSource();
  
  //Throws if invalid text in m_activityEnter, otherwise checks if user entered
  //  a unit, and if so, adjust the drop-down.
  double enteredActivity();
  
  //Gets current dose, either entered or computed, based on
  //  m_currentCalcQuantity.  Throws if no dose available.  Returns units in
  //  PhysicalUnits.
  double currentDose();
  
  std::unique_ptr<GadrasScatterTable> m_scatter;
  
  InterSpec *m_viewer;
  Wt::WSuggestionPopup *m_materialSuggest;
  MaterialDB *m_materialDB;
  ShieldingSelect *m_enterShieldingSelect;
  ShieldingSelect *m_answerShieldingSelect;

  NuclideSourceEnter *m_gammaSource;
  Wt::WButtonGroup *m_sourceType;
  Wt::WContainerWidget *m_gammaSourceDiv;
  Wt::WContainerWidget *m_neutronSourceDiv;
  Wt::WComboBox *m_neutronSourceCombo;
  
  Wt::WText     *m_activityAnswer;
  Wt::WLineEdit *m_activityEnter;
  Wt::WComboBox *m_activityEnterUnits;
  Wt::WComboBox *m_activityAnswerUnits;
  
  double m_doseAnswerValue;
  Wt::WText *m_doseAnswer;
  Wt::WLineEdit *m_doseEnter;
  Wt::WComboBox *m_doseEnterUnits;
  Wt::WComboBox *m_doseAnswerUnits;
  
  Wt::WLineEdit *m_distanceEnter;
  Wt::WText     *m_distanceAnswer;
  
  Wt::WText *m_issueTxt;
  
  Wt::WContainerWidget *m_stayTime;
  
  Wt::WGridLayout *m_layout;
  
  Quantity m_currentCalcQuantity;
  
  /** Holds two items: 1) the intro div, and 2) the content of the calculation,
   of which all quantities share the same parent div, just different elements
   are hidden/shown for the different quantities you want to calculate.
   */
  Wt::WStackedWidget *m_stack;
  
  /** The menu on the left side that lets you select which quantity you want to
      calculate.
   */
  Wt::WMenu *m_menu;
  
  Wt::WContainerWidget *m_enterWidgets[NumQuantity];
  Wt::WContainerWidget *m_answerWidgets[NumQuantity];
  
  /** For tracking undo/redo, we will keep track of the widgets state, as a URI.
   \sa encodeStateToUrl
   \sa handleAppUrl
   */
  std::string m_stateUri;
};//class DoseCalcWidget

#endif //DoseCalc_h
