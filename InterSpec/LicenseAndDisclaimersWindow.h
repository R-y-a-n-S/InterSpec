#include <any>
#ifndef LicenseAndDisclaimerWindow_h
#define LicenseAndDisclaimerWindow_h
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

#include <Wt/WString>
#include <Wt/WMessageResourceBundle>

#include "InterSpec/AuxWindow.h"

//forward declarations
class InterSpec;
class SideMenuItem;  //defined in UseInfoWindow.h

namespace Wt
{
  class WMenu;
  class WMenuItem;
  class WContainerWidget;
}

/** This class presents a modal dialog to the user (nominally only the first
    time the program is ran) displaying the various licenses, disclaimers, and
    notices.
 */
class LicenseAndDisclaimersWindow : public AuxWindow
{
public:
  /** Constructor; meant to only be called from the InterSpec class, due to undo/redo actions assuming this.
   
   \param screen_width width in pixels of the screen
   \param screen_height height in pixels of the screen
   */
  LicenseAndDisclaimersWindow( InterSpec *viewer );
  ~LicenseAndDisclaimersWindow();
  
  /** Handles the "path" component of an app URL.
   E.g., string should be one of "disclaimer", "license", "credits", "contact", "data"
   */
  bool handleAppUrlPath( const std::string &url_path );
  
protected:
  void itemCreator( const std::string &resource, Wt::WContainerWidget *parent, Wt::WString title );
  SideMenuItem *makeItem( const Wt::WString &title, const std::string &resource );
  
  void lgplLicenseCreator( Wt::WContainerWidget *parent );
  SideMenuItem *makeLgplLicenseItem();

#if( BUILD_AS_ELECTRON_APP || IOS || ANDROID || BUILD_AS_OSX_APP || BUILD_AS_LOCAL_SERVER || BUILD_AS_WX_WIDGETS_APP )
  void dataStorageCreator( Wt::WContainerWidget *parent );
  SideMenuItem *makeDataStorageItem();
#endif
  
  void right_select_item( Wt::WMenuItem *item );
  
protected:
  
  Wt::WMessageResourceBundle m_resourceBundle;
  Wt::WMenu                  *m_menu;
};//class LicenseAndDisclaimersWindow : public AuxWindow


#endif //LicenseAndDisclaimerWindow_h
