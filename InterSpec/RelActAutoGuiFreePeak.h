#ifndef RelActAutoGuiFreePeak_h
#define RelActAutoGuiFreePeak_h
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

#include <Wt/WSignal>
#include <Wt/WContainerWidget>

//Forward declerations
namespace Wt
{
  class WText;
  class WCheckBox;
}

class RelActAutoGui;
class NativeFloatSpinBox;


class RelActAutoGuiFreePeak : public Wt::WContainerWidget
{
public:
  RelActAutoGuiFreePeak( Wt::WContainerWidget *parent = nullptr );
  float energy() const;
  void setEnergy( const float energy );
  bool fwhmConstrained() const;
  void setFwhmConstrained( const bool constrained );
  bool applyEnergyCal() const;
  void setApplyEnergyCal( const bool apply );
  void setInvalidEnergy( const bool invalid );
  void handleRemoveSelf();
  void handleEnergyChange();
  void handleFwhmConstrainChanged();
  void handleApplyEnergyCalChanged();
  void setApplyEnergyCalVisible( const bool visible );
  Wt::Signal<> &updated();
  Wt::Signal<> &remove();
  
protected:
  NativeFloatSpinBox *m_energy;
  Wt::WCheckBox *m_fwhm_constrained;
  Wt::WCheckBox *m_apply_energy_cal;
  Wt::WText *m_invalid;
  
  Wt::Signal<> m_updated;
  Wt::Signal<> m_remove;
};//RelActAutoGuiFreePeak


#endif //RelActAutoGuiFreePeak_h
