#include <any>
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

#include <Wt/WDialog>
#include <Wt/WServer>
#include <Wt/WPushButton>
#include <Wt/WApplication>
#include <Wt/WContainerWidget>

#include <string>

#include "InterSpec/InterSpec.h"  //for InterSpec::instance()
#include "InterSpec/SimpleDialog.h"

#if( BUILD_AS_WX_WIDGETS_APP || BUILD_AS_ELECTRON_APP )
#include "InterSpec/InterSpecApp.h"  //for InterSpecApp::isPrimaryWindowInstance()
#endif

using namespace std;
using namespace Wt;

#define INLINE_JAVASCRIPT(...) #__VA_ARGS__

WT_DECLARE_WT_MEMBER
 (SimpleDialogBringToFront, Wt::JavaScriptFunction, "SimpleDialogBringToFront",
  function( id )
  {
   const maxz = $('.Wt-dialog,.MobileMenuButton').get().reduce( function(result, item){
     if( item.id === id ) return result;
     const z = parseInt( $(item).css('z-index') );
     return isNaN(z) ? result : Math.max(result, z);
    }, 0);
   
   const z = parseInt( $('#'+id).css('z-index') );
   
   if( isNaN(z) || maxz >= z )
     $('#'+id).css('z-index', maxz+1);
   $('.window-controls-container').css('z-index', maxz + 2); //for wxWidgets and Electron builds
   $('.suggestion').css('z-index', maxz+2);
 }
);


SimpleDialog::SimpleDialog()
: Wt::WDialog( InterSpec::instance() ),  //for lifetime purposes
  m_title( nullptr ),
  m_msgContents( nullptr ),
  m_multipleBringToFront( true )
{
  init( "", "" );
}


SimpleDialog::SimpleDialog( const Wt::WString &title )
 : Wt::WDialog( InterSpec::instance() ),  //for lifetime purposes
  m_title( nullptr ),
  m_msgContents( nullptr ),
  m_multipleBringToFront( true )
{
  init( title, "" );
}


SimpleDialog::SimpleDialog( const Wt::WString &title, const Wt::WString &content )
 : Wt::WDialog( InterSpec::instance() ),  //for lifetime purposes
  m_title( nullptr ),
  m_msgContents( nullptr ),
  m_multipleBringToFront( true )
{
  init( title, content );
}


void SimpleDialog::render( Wt::WFlags<Wt::RenderFlag> flags )
{
  Wt::WDialog::render( flags );
  
  if( flags & RenderFull )
  {
    // WDialog::setMaximumSize will silently not use dimensions if WLength::Percentage, so we use CSS.
    //  Note that page dimensions wont be available during initial rendering of the webapp
    
    // The below seems to be necessary or else sometimes the window doesnt resize to fit its content
    wApp->doJavaScript( wApp->javaScriptClass() + ".TriggerResizeEvent();" );

#if( BUILD_AS_WX_WIDGETS_APP || BUILD_AS_ELECTRON_APP )
    // To allow moving window around when dialog showing; see note in AuxWindow::render 
    //  for the same code snippet
    if( InterSpecApp::isPrimaryWindowInstance() )
    {
#if( BUILD_AS_WX_WIDGETS_APP )
      WWidget* coverw = wApp->findWidget("dialog-cover");
      WContainerWidget* dialog_cover = dynamic_cast<WContainerWidget*>(coverw);
      if (dialog_cover && !dialog_cover->mouseWentDown().isConnected())
        dialog_cover->mouseWentDown().connect(wApp->javaScriptClass() + ".MouseDownOnDialogCover");
#endif

      // Raise windows controls (minimize, maximize, close), to above the dialog-cover.
      wApp->doJavaScript(wApp->javaScriptClass() + ".RaiseWinCntrlsAboveCover();");
    }
#endif
    
    // On mobile, it seems Wt.WT.AuxWindowBringToFront(...) may get called after this window is
    //  created (happens on the "QR code" link on Nuclide Decay Tool - since the user clicks
    //  a button in the titlebar), which will bring that dialog above this one - which isnt wanted,
    //  so we'll manually bring this dialog to the top on a delay.
    //  We'll add this JS, even on non-mobile, JIC
    LOAD_JAVASCRIPT(wApp, "SimpleDialog.cpp", "SimpleDialog", wtjsSimpleDialogBringToFront);
    
    const string time_delays_array = m_multipleBringToFront ? "[5,100,500]" : "[5]";
    
    doJavaScript( "for( const d of " + time_delays_array + "){"
                    "setTimeout( function(){ Wt.WT.SimpleDialogBringToFront('" + id() + "');}, d);"
                  "}");
  }//if( flags & RenderFull )
}//render( flags )


void SimpleDialog::init( const Wt::WString &title, const Wt::WString &content )
{
  wApp->useStyleSheet( "InterSpec_resources/SimpleDialog.css" );
  
  addStyleClass( "simple-dialog" );
  
  setModal( true );
  
  setMovable( false );
  
  if( title.empty() )
  {
    setTitleBarEnabled( false );
  }else
  {
    setTitleBarEnabled( true );
    //setWindowTitle( title ); //Dont use the default Wt <h4>Some Text...</h4> stuff
    titleBar()->removeStyleClass( "titlebar" );  //Avoid the Wt changing of text color and such
    titleBar()->addStyleClass( "title" );
    m_title = new WText( title, titleBar() );
    m_title->setInline( false );
    //m_title->addStyleClass( "title" );
  }
  
  if( !content.empty() )
  {
    m_msgContents = new WText( content, contents() );
    m_msgContents->addStyleClass( "content" );
    m_msgContents->setInline( false );
  }
  
  // We need to set the minimum size in C++; the maximum size is set in CSS.
  setMinimumSize( WLength(260,WLength::Pixel), WLength::Auto );
  
  show();
  finished().connect( this, &SimpleDialog::startDeleteSelf );
  
#if( WT_VERSION > 0x3040000 )
  // I havent checked version of Wt that does include `raiseToFront()`, but 3.3.4 doesnt.
  //20250228: raiseToFront() call removed do to encountering some JS exception, that _maybe_ have something to do with this (the object not found), on Windows - I dont actually know if this function call is the problem.
  //raiseToFront();
#endif
}//init(...)


SimpleDialog::~SimpleDialog()
{
}


void SimpleDialog::doNotUseMultpleBringstoFront()
{
  m_multipleBringToFront = false;
}

Wt::WPushButton *SimpleDialog::addButton( const Wt::WString &txt )
{
  Wt::WPushButton *b = new WPushButton( txt, footer() );
  b->setStyleClass( "simple-dialog-btn" );
  
  // TODO: closing the dialog seems a little laggy; check if WDialog::hide is faster, or if we
  //       should stick to using the JS
  //b->clicked().connect( this, &WDialog::hide );
  b->clicked().connect( "function(){$('#" + id() + "').hide(); $('.Wt-dialogcover').hide();}" );
  
  b->clicked().connect( boost::bind( &WDialog::done, this, Wt::WDialog::DialogCode::Accepted ) );
  return b;
}//addButton(...)


void SimpleDialog::startDeleteSelf()
{
  if( isModal() )
    setModal(false);
  
  // We'll actually delete the windows later on in the event loop incase the order of connections
  //  to its signals is out of intended order, but also we will protect against being deleted in the
  //  current event loop as well
  boost::function<void(void)> updater = wApp->bind( boost::bind( &SimpleDialog::deleteSelf, this ) );
  WServer::instance()->post( wApp->sessionId(), std::bind( [updater](){
    updater();
    wApp->triggerUpdate();
  } ) );
}//startDeleteSelf()


void SimpleDialog::deleteSelf()
{
  // calling 'delete this' makes me feel uneasy
  delete this;
}

