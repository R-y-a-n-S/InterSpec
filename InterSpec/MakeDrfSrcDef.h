#include <any>
#ifndef MakeDrfSrcDef_h
#define MakeDrfSrcDef_h
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
#include <istream>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <Wt/WContainerWidget>

class MaterialDB;
class PopupDivMenu;
class ShieldingSelect;

namespace Wt
{
  class WText;
  class WLabel;
  class WTable;
  class WLineEdit;
  class WCheckBox;
  class WDateEdit;
  class WComboBox;
  class WPushButton;
  class WDoubleSpinBox;
  class WSuggestionPopup;
}//namespace Wt

namespace SandiaDecay
{
  struct Nuclide;
}//namespace SandiaDecay


/** The user can have a Source.lib file in their data directory that contains
 calibration source information.
 
 Source lines should be in the format:
  "[nuclide]_[serial num]  [activity in bq]  [activity reference date]  [comment]"
  for example:
  "22NA_01551910  5.107E+04  25-Aug-2022  West Rad Lab"
    
  If any field is not valid  (except comment can be blank), the line wont be included in the results.
 
  Each line contains a different source.
 */
struct SrcLibLineInfo
{
  /** Activity of source (in units of PhysicalUnits), on date specified. */
  double m_activity;
  /** Nuclide of this source. */
  const SandiaDecay::Nuclide *m_nuclide;
  /** The date the activity was specified for.
   For sources that grow in, this is also assumed to be their T=0 date.
   */
  boost::posix_time::ptime m_activity_date;
  
  /** The first field specified. */
  std::string m_source_name;
  
  /** Everything past activity field. */
  std::string m_comments;
  
  /** The entire line all the information was extracted from. */
  std::string m_line;
  
  /** Optional activity uncertainty (in units of PhysicalUnits activity - i.e., bq)
   Will be zero or negative if not specified.
   */
  double m_activity_uncert;
  
  /** Optional distance used for the source.
   Will be zero or negative if not specified.
   */
  double m_distance;
  
  SrcLibLineInfo();
  
  /** Source lines should be in the format:
   "[nuclide]_[serial num]  [activity in bq]  [activity reference date]  [comment]"
   for example:
   "22NA_01551910  5.107E+04  25-Aug-2022  West Rad Lab, Distance=50cm, ActivityUncertainty=5.107E+03"
   
   The comment field can optionally contain both the distance, and activity uncertainty, for example:
    - "Distance=50cm"
    - "ActivityUncertainty=5.107E+03", where activity uncertainty is in Bq
   
   If any field is not valid  (except comment can be blank), the line wont be included in the results.
   */
  static std::vector<SrcLibLineInfo> sources_in_lib( std::istream &strm );
  
  /** A convenience function for above, that takes a file path, instead of a stream. */
  static std::vector<SrcLibLineInfo> sources_in_lib( const std::string &filename );
  
  /** Checks if first valid-ish line is a valid source, and if so, returns true. */
  static bool is_candidate_src_lib( std::istream &strm );
  
  /** Grabs sources in all Source.lib files in user and application data directories. */
  static std::vector<SrcLibLineInfo> sources_in_all_libs();
};//struct SrcLibLineInfo


/* Widget to allow entering:
   - Nuclide
   - Activity
   - Date of activity
   - Date of measurement being used for (can likely be auto-filed out)
   - Potentially age when activity was determined
   - Activity uncertainty
   - Distance to source
   - Shielding
 */
class MakeDrfSrcDef : public Wt::WContainerWidget
{
public:
  MakeDrfSrcDef( const SandiaDecay::Nuclide *nuc,
                 const boost::posix_time::ptime &measDate,
                 MaterialDB *materialDB,
                 Wt::WSuggestionPopup *materialSuggest,
                 Wt::WContainerWidget *parent = nullptr );
  
  virtual ~MakeDrfSrcDef();
  
  /** Signal emitted when widget is upadated by the user. */
  Wt::Signal<> &updated();
  
  /** Returns user entered distance.
   
     Throws exception if input field has invalid text.
   */
  double distance() const;
  
  /** Returns the nuclide this MakeDrfSrcDef is for.
   
   Will be nullptr if that is what was passed in.
   */
  const SandiaDecay::Nuclide *nuclide() const;
  
  /** Returns the nuclide activity, at the time the characterization measurment
     was taken.
   
     Throws exception if there are errors in any of the user input fileds.
   */
  double activityAtSpectrumTime() const;
  
  /** Returns the fractional (so 0.0 to 1.0) uncertainty on the activity.
   
   Throws exception if there are errors in the user input field.
   */
  double fractionalActivityUncertainty() const;
  
  /** Returns the nuclide age (in units of PhysicalUnits), at the time the
   characterization measurment was taken.  Returns the age you should assume the
   source is at the time of the characterization, not necassarily the actual
   age.
   
   Throws exception if there are errors in any of the user input fileds.
   */
  double ageAtSpectrumTime() const;
  
  /** Returns nullptr if the user has not selected a shielding, currently
   a fixed geometry, other wise returns the ShieldingSelect.
   */
  ShieldingSelect *shielding();
  
  /** Set the distance, maybe from a hint in the spectrum file.
   Does not cause the updated() signal to be emitted.
   */
  void setDistance( const double dist );
  
  /** Set the activity, maybe from a hint in the spectrum file.
   Does not cause the updated() signal to be emitted.
   */
  void setActivity( const double act );
  
  /** Sets the activity and assay date, enabling aging if it isnt already.
   Does not cause the updated() signal to be emitted.
   */
  void setAssayInfo( const double activity, const boost::posix_time::ptime &assay_date );
  
  /** Sets the age at assay date, enabling aging if it isnt already.
   Does not cause the updated() signal to be emitted.
   //Function was implemented but not tested, so leaving commented out until its needed
   */
  //void setAgeAtAssay( const double age );
  
  
  /** Sets the age at the time of measurement.
     If there is already a measurement and assay date, and 'age' is larger than
     their difference, the age at assay field is modified.  If this is not the
     case, the assay date is set equal to measurement date, and age at assay
     field is set to the value passed in.
     If a negative value is passed in, the setNuclide(...) is called, effectively
     reseting things.
   
     Does not cause the updated() signal to be emitted.
   */
  void setAgeAtMeas( const double age );
  
  
  /** Sets the shielding to be generic material with given atomoic number and
     areal density.
     Causes shielding widget to be shown.
   */
  void setShielding( const float atomic_number, const float areal_density );
  
  
  /** Sets geometry type of DRF being made (e.g., hide distance and shielding), or not (default).
   The int passed in is defined by DetectorPeakResponse::EffGeometryType.
   */
  void setIsEffGeometryType( const int drf_eff_geom_type );
  
  
  /** Returns a GADRAS style source string.
      May include source age, if it will matter.
      Will not include source distance.
   
      Examples are: "133Ba,10uCi"
                    "232U,10uC{26,10}"
                    "232U,10uC{26,10} Age=20y"  //The Age is a InterSpec extension and not followed by GADRAS.
   
      Throughs exception if any input widgets are invalid.
   */
  std::string toGadrasLikeSourceString() const;
  
  /** Returns the sources defined in Sources.lib, for this nuclide. */
  const std::vector<SrcLibLineInfo> &lib_srcs_for_nuc();
  
  /** Adds sources from a Source.lib file to this widget.
   
   \param srcs The sources to add to the menu of `m_lib_src_btn` - only sources matching this sources nuclide will be added.
   \param auto_populate If true, then the information will be set for the first matching nuclide found in `srcs`.
   */
  void addSourceLibrary( std::vector<std::shared_ptr<const SrcLibLineInfo>> srcs,
                        const bool auto_populate );
protected:
  /** Creates the widget, but doesnt fill out any of the information. */
  void create();
  
  void setNuclide( const SandiaDecay::Nuclide *nuc );
  
  void setSrcInfo( const SrcLibLineInfo &info );
  
  void updateAgedText();
  
  void useAgeInfoUserToggled();
  
  void useShieldingInfoUserToggled();
  
  void handleUserChangedShielding();
  
  void handleUserChangedDistance();
  
  void handleUserChangedActivity();
  
  void handleUserChangedActivityUncertainty();
  
  void handleUserChangedAgeAtAssay();
  
  void handleEnteredDatesUpdated();
  
  void validateDateFields();
  
  /** Returns user entered activity.  Throws exception if invalid. */
  double enteredActivity() const;
  
  void updateSourceLibNuclides();
  
  Wt::WTable *m_table;
  
  /** The name of the nuclide this source represents. */
  const SandiaDecay::Nuclide *m_nuclide;
  
  /** Pointer to shielding material database, guaranteed non-null.
      Not owned by this object.
   */
  MaterialDB *m_materialDB;
  
  /** The material suggestion widget shared everywhere within InterSpec.
      Not owned by this object.
   */
  Wt::WSuggestionPopup *m_materialSuggest;
  
  /** Display of the nuclide. */
  Wt::WText *m_nuclideLabel;
  
  
  Wt::WLabel *m_distanceLabel;
  
  /** The distance of source to face of detector.
   
   When geometry is fixed, this will be hidden.
   */
  Wt::WLineEdit *m_distanceEdit;
  
  Wt::WLabel *m_activityLabel;
  
  /** */
  Wt::WLineEdit *m_activityEdit;
  
  Wt::WComboBox *m_activityUnits;
  
  Wt::WDoubleSpinBox *m_activityUncertainty;
  
  /** Whether or not age/decay info should be entered (default: no) */
  Wt::WCheckBox *m_useAgeInfo;
  
  /** The date entered activity was determined. */
  Wt::WDateEdit *m_assayDate;
  
  /** The date the spectrum being used for creating the DRF was taken on. */
  Wt::WDateEdit *m_drfMeasurementDate;
  
  /** The source age and activity at the time of the measurement. */
  Wt::WText *m_sourceInfoAtMeasurement;
  
  /** The source age at assay, if applicable */
  Wt::WLineEdit *m_sourceAgeAtAssay;
  
  /** Whether or not to have shielding for the source (default: no) */
  Wt::WCheckBox *m_useShielding;
  
  /** The widget to allow selecting shielding, is selected to do so. */
  ShieldingSelect *m_shieldingSelect;
  
  /** The sources in Source.lib that are for this nuclide. */
  std::vector<SrcLibLineInfo> m_lib_srcs_for_nuc;
  
  /** All sources from Source.lib files found on the device. */
  std::vector<SrcLibLineInfo> m_lib_srcs_from_file;
  
  /** Sources added by dragging-n-dropping Source.lib onto app - via `addSourceLibrary(...)` */
  std::vector<std::shared_ptr<const SrcLibLineInfo>> m_lib_srcs_added;
  
  /** Button that lets you select sources that might be in Source.lib files */
  Wt::WPushButton *m_lib_src_btn;
  
  /** Menu that pops up when `m_lib_src_btn` is clicked. */
  PopupDivMenu *m_lib_src_menu;
  
  Wt::Signal<> m_updated;
};//MakeDrfSrcDef

#endif //MakeDrfSrcDef_h
