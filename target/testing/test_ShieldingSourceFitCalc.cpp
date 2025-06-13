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

#include <string>
#include <iostream>

#include <Wt/Utils>

#ifdef _WIN32
// For some reason, we need to include the following includes, before unit_test.hpp,
//  or we get a bunch of errors relating to winsock.k being included multiple times,
//  or something
#include "winsock2.h"
#include "Windows.h"
#endif

//#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ShieldingSourceFitCalc_suite
//#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"
#include "rapidxml/rapidxml_print.hpp"

//Roots Minuit2 includes
#include "Minuit2/MnUserParameters.h"
#include "Minuit2/MnUserParameterState.h"


#include "SpecUtils/StringAlgo.h"
#include "SpecUtils/Filesystem.h"
#include "SpecUtils/RapidXmlUtils.hpp"

#include "InterSpec/PeakDef.h"
#include "InterSpec/SpecMeas.h"
#include "InterSpec/InterSpec.h"
#include "InterSpec/MaterialDB.h"
#include "InterSpec/PhysicalUnits.h"
#include "InterSpec/DecayDataBaseServer.h"
#include "InterSpec/DetectorPeakResponse.h"
#include "InterSpec/DetectorPeakResponse.h"
#include "InterSpec/GammaInteractionCalc.h"
#include "InterSpec/ShieldingSourceFitCalc.h"


using namespace std;
using namespace boost::unit_test;


std::string g_test_file_dir;

// We need to set the static data directory, so the code knows where
//  like sandia.decay.xml is located.
void set_data_dir()
{
  // We only need to initialize things once
  static bool s_have_set = false;
  if( s_have_set )
    return;
  
  s_have_set = true;
  
  int argc = boost::unit_test::framework::master_test_suite().argc;
  char **argv = boost::unit_test::framework::master_test_suite().argv;
  
  string datadir;
  
  for( int i = 1; i < argc; ++i )
  {
    cout << "Arg " << i << ": '" << argv[i] << "'" << endl;
    const string arg = argv[i];
    if( SpecUtils::istarts_with( arg, "--datadir=" ) )
      datadir = arg.substr( 10 );
    
    if( SpecUtils::istarts_with( arg, "--testfiledir=" ) )
      g_test_file_dir = arg.substr( 14 );
  }//for( int arg = 1; arg < argc; ++ arg )
  
  SpecUtils::ireplace_all( datadir, "%20", " " );
  SpecUtils::ireplace_all( g_test_file_dir, "%20", " " );
  
  // Search around a little for the data directory, if it wasnt specified
  if( datadir.empty() )
  {
    for( const auto &d : { "data", "../data", "../../data", "../../../data", "/Users/wcjohns/rad_ana/InterSpec/data" } )
    {
      if( SpecUtils::is_file( SpecUtils::append_path(d, "sandia.decay.xml") ) )
      {
        datadir = d;
        break;
      }
    }//for( loop over candidate dirs )
  }//if( datadir.empty() )
  
  const string required_data_file = "findCharacteristics/202204_example_problem_1.n42";
  if( g_test_file_dir.empty() )
  {
    for( const auto &d : { "test_data", "../test_data", "../../test_data", "/Users/wcjohns/rad_ana/InterSpec/target/testing/test_data" } )
    {
      if( SpecUtils::is_file( SpecUtils::append_path(d, required_data_file) ) )
      {
        g_test_file_dir = d;
        break;
      }
    }//for( loop over candidate dirs )
  }
  
  const string sandia_deacay_file = SpecUtils::append_path(datadir, "sandia.decay.xml");
  BOOST_REQUIRE_MESSAGE( SpecUtils::is_file( sandia_deacay_file ), "sandia.decay.xml not at '" << sandia_deacay_file << "'" );
  
  BOOST_REQUIRE_NO_THROW( InterSpec::setStaticDataDirectory( datadir ) );
  
  // Make sure we can actually init the decay database
  const SandiaDecay::SandiaDecayDataBase * const db = DecayDataBaseServer::database();
  BOOST_REQUIRE_MESSAGE( db, "Error initing SandiaDecayDataBase" );
  BOOST_REQUIRE_MESSAGE( db->nuclide("U238"), "SandiaDecayDataBase empty?" );
  
  const string required_data_path = SpecUtils::append_path(g_test_file_dir, required_data_file);
  BOOST_REQUIRE_MESSAGE( SpecUtils::is_file( required_data_path ), "'" << required_data_file << "' not at '" << required_data_path << "'" );
}//void set_data_dir()


BOOST_AUTO_TEST_CASE( ShieldingInfoUri )
{
  using namespace ShieldingSourceFitCalc;
  
  set_data_dir();
  
  const SandiaDecay::SandiaDecayDataBase *db = DecayDataBaseServer::database();
  MaterialDB matdb;
  
  const string materialfile = SpecUtils::append_path( InterSpec::staticDataDirectory(), "MaterialDataBase.txt" );
  BOOST_REQUIRE_NO_THROW( matdb.parseGadrasMaterialFile( materialfile, db, false ) );
  
  const Material * const iron = matdb.material("Fe (iron)");
  BOOST_REQUIRE( iron );
  
  const Material * const uranium = matdb.material("U (uranium)");
  BOOST_REQUIRE( uranium );
  
  {// Begin test Trace source serialization
    TraceSourceInfo trace;
    trace.m_type = GammaInteractionCalc::TraceActivityType::ActivityPerCm3;
    trace.m_fitActivity = true;
    BOOST_REQUIRE( trace.m_nuclide = db->nuclide( "U238" ) );
    trace.m_activity = 2.0*PhysicalUnits::microCi / PhysicalUnits::cm2;
    trace.m_relaxationDistance = 0;
    
    {
      rapidxml::xml_document<char> doc;
      BOOST_REQUIRE_NO_THROW( trace.serialize( &doc ) );
      
      TraceSourceInfo from_xml;
      BOOST_REQUIRE_NO_THROW( from_xml.deSerialize( doc.first_node() ) );
      BOOST_REQUIRE_NO_THROW( TraceSourceInfo::equalEnough( trace, from_xml ) );
    }
    
    trace.m_type = GammaInteractionCalc::TraceActivityType::ExponentialDistribution;
    trace.m_relaxationDistance = 5*PhysicalUnits::cm;
    
    {
      rapidxml::xml_document<char> doc;
      BOOST_REQUIRE_NO_THROW( trace.serialize( &doc ) );
      
      TraceSourceInfo from_xml;
      BOOST_REQUIRE_NO_THROW( from_xml.deSerialize( doc.first_node() ) );
      BOOST_REQUIRE_NO_THROW( TraceSourceInfo::equalEnough( trace, from_xml ) );
    }
  }// End test Trace source serialization
  
  
  {// Begin test Trace source serialization, with empty src
    TraceSourceInfo trace;
    trace.m_type = GammaInteractionCalc::TraceActivityType::ActivityPerCm3;
    trace.m_fitActivity = false;
    trace.m_nuclide = nullptr;
    trace.m_activity = 0.0f;
    trace.m_relaxationDistance = 0.0f;
    
    {
      rapidxml::xml_document<char> doc;
      BOOST_REQUIRE_NO_THROW( trace.serialize( &doc ) );
      
      TraceSourceInfo from_xml;
      BOOST_REQUIRE_NO_THROW( from_xml.deSerialize( doc.first_node() ) );
      BOOST_REQUIRE_NO_THROW( TraceSourceInfo::equalEnough( trace, from_xml ) );
    }
  }// End test Trace source serialization, with empty src
  

  
  
  {// Begin test ShieldingInfo encode/decode from URI/XML
    // URI currently does not encode self-attenuating source, trace-source information, or "truth" values.
    ShieldingInfo info;
    
    info.m_geometry = GammaInteractionCalc::GeometryType::CylinderEndOn;
    info.m_isGenericMaterial = false;
    info.m_forFitting = true;
    info.m_material = make_shared<Material>( *iron );
    info.m_dimensions[0] = 1.2*PhysicalUnits::meter;
    info.m_dimensions[1] = 1.1*PhysicalUnits::meter;
    info.m_dimensions[2] = 1.0E-6*PhysicalUnits::meter;
    info.m_fitDimensions[0] = false;
    info.m_fitDimensions[1] = true;
    info.m_fitDimensions[2] = false;
    
    
    {// Begin test URI - doesnt include truth values or trace/self-atten sources, so test before we set those
      const string uri = info.encodeStateToUrl();
      
      ShieldingInfo from_uri;
      BOOST_REQUIRE_NO_THROW( from_uri.handleAppUrl( uri, &matdb ) );
      BOOST_CHECK_NO_THROW( ShieldingInfo::equalEnough( info, from_uri ) );
    }// End test URI
    
    
#if( PERFORM_DEVELOPER_CHECKS || BUILD_AS_UNIT_TEST_SUITE )
    // Truth info only tested for XML serialization
    info.m_truthDimensions[0] = 1.11E-6f;
    info.m_truthDimensions[1] = boost::optional<float>();
    info.m_truthDimensions[2] = 9.99E-3f;
    
    info.m_truthDimensionsTolerances[0] = 1.11E-7f;
    info.m_truthDimensionsTolerances[1] = 100.0f;
    info.m_truthDimensionsTolerances[2] = boost::optional<float>();
#endif
    
    
    {
      rapidxml::xml_document<char> doc;
      BOOST_REQUIRE_NO_THROW( info.serialize( &doc ) );
      
      ShieldingInfo from_xml;
      BOOST_REQUIRE_NO_THROW( from_xml.deSerialize( doc.first_node(), &matdb ) );
      BOOST_REQUIRE_NO_THROW( ShieldingInfo::equalEnough( info, from_xml ) );
    }
    
    
    // Now modify `info` to include trace and self-attenuating sources
    info.m_material = make_shared<Material>( *uranium );
    
    
    // Self-atten source stuff
    {// Begin add self-atten source
      map<const SandiaDecay::Element *,vector<tuple<const SandiaDecay::Nuclide *,double,bool>>> 
        &el_to_nuc_frac = info.m_nuclideFractions_;
      
      const SandiaDecay::Nuclide *nuc = db->nuclide( "U-238" );
      const SandiaDecay::Element *el = db->element( nuc->atomicNumber );
      BOOST_REQUIRE( nuc && el );
      el_to_nuc_frac[el].emplace_back( nuc, 0.8, true );
      
      nuc = db->nuclide( "U-235" );
      el = db->element( nuc->atomicNumber );
      BOOST_REQUIRE( nuc && el );
      el_to_nuc_frac[el].emplace_back( nuc, 0.15, true );
      
      nuc = db->nuclide( "U-232" );
      el = db->element( nuc->atomicNumber );
      BOOST_REQUIRE( nuc && el );
      el_to_nuc_frac[el].emplace_back( nuc, 2.5E-8, false );
    }// End add self-atten source
    
    // Trace-source stuff
    {// Begin add trace source source
      
      TraceSourceInfo trace;
      trace.m_type = GammaInteractionCalc::TraceActivityType::ActivityPerCm3;
      trace.m_fitActivity = false;
      trace.m_nuclide = db->nuclide( "Cs137" );
      trace.m_activity = 2.0*PhysicalUnits::microCi / PhysicalUnits::cm2;
      trace.m_relaxationDistance = 0.0f;
      info.m_traceSources.push_back( trace );
      
      
      trace.m_type = GammaInteractionCalc::TraceActivityType::ActivityPerGram;
      trace.m_fitActivity = true;
      trace.m_nuclide = db->nuclide( "Ba133" );
      trace.m_activity = 5.0*PhysicalUnits::microCi / PhysicalUnits::gram;
      trace.m_relaxationDistance = 0.0f;
      info.m_traceSources.push_back( trace );
      
      trace.m_type = GammaInteractionCalc::TraceActivityType::ExponentialDistribution;
      trace.m_fitActivity = true;
      trace.m_nuclide = db->nuclide( "I131" );
      trace.m_activity = 1.0E5*PhysicalUnits::bq / PhysicalUnits::m2;
      trace.m_relaxationDistance = 1.23*PhysicalUnits::cm;
      info.m_traceSources.push_back( trace );
    }// End add trace source source
    
    {
      rapidxml::xml_document<char> doc;
      BOOST_REQUIRE_NO_THROW( info.serialize( &doc ) );
      
      ShieldingInfo from_xml;
      BOOST_REQUIRE_NO_THROW( from_xml.deSerialize( doc.first_node(), &matdb ) );
      BOOST_REQUIRE_NO_THROW( ShieldingInfo::equalEnough( info, from_xml ) );
    }
  }// End test ShieldingInfo encode/decode from URI
}//BOOST_AUTO_TEST_CASE( ShieldingInfoUri )


BOOST_AUTO_TEST_CASE( IsoFitStructSerialization )
{
  set_data_dir();
  
  const SandiaDecay::SandiaDecayDataBase * const db = DecayDataBaseServer::database();
  BOOST_REQUIRE_MESSAGE( db, "Error initing SandiaDecayDataBase" );
  
  ShieldingSourceFitCalc::IsoFitStruct test;
  
  test.nuclide = db->nuclide( "U238" );
  test.activity = 1.1*PhysicalUnits::curie;
  test.fitActivity = true;
  test.age = 20*PhysicalUnits::year;
  test.fitAge = false;
  test.ageDefiningNuc = db->nuclide( "U235" );
  test.sourceType = ShieldingSourceFitCalc::ModelSourceType::Intrinsic;
  test.numProgenyPeaksSelected = 2;
  test.ageIsFittable = !PeakDef::ageFitNotAllowed( test.ageDefiningNuc ) ;
  test.activityUncertainty = 0.1*test.activity;
  test.ageUncertainty = 0.0;
  
#if( INCLUDE_ANALYSIS_TEST_SUITE || PERFORM_DEVELOPER_CHECKS || BUILD_AS_UNIT_TEST_SUITE )
  test.truthActivity = 1.0*PhysicalUnits::curie;
  test.truthActivityTolerance = 0.15*PhysicalUnits::curie;
  test.truthAge = 21.5*PhysicalUnits::year;
  test.truthAgeTolerance = 1.05*PhysicalUnits::year;
#endif
  
  rapidxml::xml_document<char> doc;
  BOOST_REQUIRE_NO_THROW( test.serialize( &doc ) );
  
  ShieldingSourceFitCalc::IsoFitStruct from_xml;
  BOOST_REQUIRE_NO_THROW( from_xml.deSerialize( doc.first_node() ) );
  BOOST_CHECK_NO_THROW( ShieldingSourceFitCalc::IsoFitStruct::equalEnough( test, from_xml ) );
}//BOOST_AUTO_TEST_CASE( IsoFitStructSerialization )



BOOST_AUTO_TEST_CASE( SrcFitOptionsSerialization )
{
  set_data_dir();
  
  for( size_t i = 0; i < 20; ++i )
  {
    ShieldingSourceFitCalc::ShieldingSourceFitOptions test;
    
    test.multiple_nucs_contribute_to_peaks = (rand() % 2);
    test.attenuate_for_air = (rand() % 2);
    test.account_for_decay_during_meas = (rand() % 2);
    test.multithread_self_atten = (rand() % 2);
    test.photopeak_cluster_sigma = ((10.0*rand()) / RAND_MAX);
    test.background_peak_subtract = (rand() % 2);
    test.same_age_isotopes = (rand() % 2);
    
    rapidxml::xml_document<char> doc;
    BOOST_REQUIRE_NO_THROW( test.serialize( &doc ) );
    
    ShieldingSourceFitCalc::ShieldingSourceFitOptions from_xml;
    BOOST_REQUIRE_NO_THROW( from_xml.deSerialize( &doc ) );
    BOOST_CHECK_NO_THROW( ShieldingSourceFitCalc::ShieldingSourceFitOptions::equalEnough( test, from_xml ) );
  }//for( size_t i = 0; i < 20; ++i )
}//BOOST_AUTO_TEST_CASE( IsoFitStructSerialization )


// A simple, fully programmatically defined case of fitting a source/shielding model
BOOST_AUTO_TEST_CASE( SimpleSourceFit )
{
  const SandiaDecay::SandiaDecayDataBase * const db = DecayDataBaseServer::database();
  BOOST_REQUIRE_MESSAGE( db, "Error initing SandiaDecayDataBase" );
  
  const double distance = 100*PhysicalUnits::cm;
  const float live_time = 100*PhysicalUnits::second;
  const float peak_area = PhysicalUnits::microCi;
  
  const GammaInteractionCalc::GeometryType geometry = GammaInteractionCalc::GeometryType::Spherical;
  vector<ShieldingSourceFitCalc::ShieldingInfo> shieldings;
  vector<ShieldingSourceFitCalc::SourceFitDef> src_definitions;
  
  {
    ShieldingSourceFitCalc::SourceFitDef cs137_src;
    cs137_src.nuclide = db->nuclide( "Cs137" );
    cs137_src.activity = 2.0*peak_area;
    cs137_src.fitActivity = true;
    cs137_src.age = 180*PhysicalUnits::day;
    cs137_src.fitAge = false;
    cs137_src.ageDefiningNuc = nullptr;
    cs137_src.sourceType = ShieldingSourceFitCalc::ModelSourceType::Point;
    
    src_definitions.push_back(cs137_src);
  }
  
  // Create a 100% efficient detector at our test distance.
  auto detector = make_shared<DetectorPeakResponse>();
  detector->fromExpOfLogPowerSeriesAbsEff( {0.0f, 0.0f}, {}, distance,
                                          5*PhysicalUnits::cm, PhysicalUnits::keV,
                                          0, 3000*PhysicalUnits::keV,
                                          DetectorPeakResponse::EffGeometryType::FarField );
  
  auto foreground = make_shared<SpecUtils::Measurement>();
  
  auto spec = make_shared<vector<float>>();
  *spec = {0.0f, 1.0f, 5.0f, 2.0f, 3.5f};
  foreground->set_gamma_counts( spec, live_time, live_time );
  
  std::deque<std::shared_ptr<const PeakDef>> foreground_peaks;
  
  {// Begin create peak for 661 keV in Cs137
    auto peak = make_shared<PeakDef>();
    peak->setMean( 661 );
    peak->setSigma( 10.0 );
    peak->setPeakArea( peak_area );
    peak->setPeakAreaUncert( sqrt(peak_area) );
    
    const auto cs137 = db->nuclide( "Cs137" );
    BOOST_REQUIRE( cs137 );
    const auto ba137m = db->nuclide( "Ba137m" ); //The 661 keV actually comes from Ba137m
    BOOST_REQUIRE( ba137m );
    int radParticle = -1;
    const SandiaDecay::Transition *transition = nullptr;
    
    for( size_t i = 0; !transition && (i < ba137m->decaysToChildren.size()); ++i )
    {
      const SandiaDecay::Transition *trans = ba137m->decaysToChildren[i];
      for( size_t j = 0; !transition && (j < trans->products.size()); ++j )
      {
        if( (trans->products[j].type == SandiaDecay::GammaParticle)
           && fabs(trans->products[j].energy - 661.657) < 1.0)
        {
          transition = trans;
          radParticle = static_cast<int>(j);
        }
      }
    }//
    
    BOOST_REQUIRE( transition && (radParticle >= 0) );
    
    peak->setNuclearTransition( cs137, transition, radParticle, PeakDef::SourceGammaType::NormalGamma );
    peak->useForShieldingSourceFit( true );
    
    foreground_peaks.push_back( peak );
  }// End create peak for 661 keV in Cs137
  
  
  ShieldingSourceFitCalc::ShieldingSourceFitOptions options;
  options.attenuate_for_air = false;
  
  pair<shared_ptr<GammaInteractionCalc::ShieldingSourceChi2Fcn>, ROOT::Minuit2::MnUserParameters> fcn_pars =
                GammaInteractionCalc::ShieldingSourceChi2Fcn::create( distance, geometry,
                                  shieldings, src_definitions, detector,
                                  foreground, nullptr, foreground_peaks, nullptr, options );
 
  auto inputPrams = make_shared<ROOT::Minuit2::MnUserParameters>();
  *inputPrams = fcn_pars.second;
  
  auto progress = make_shared<ShieldingSourceFitCalc::ModelFitProgress>();
  auto results = make_shared<ShieldingSourceFitCalc::ModelFitResults>();
  
  
  auto progress_fcn = [progress](){
    // Probably wont get called, since its a simple fit
  };
  
  bool finished_fit_called = false;
  auto finished_fcn = [results, &finished_fit_called](){
    finished_fit_called = true;
  };
  
  ShieldingSourceFitCalc::fit_model( "", fcn_pars.first, inputPrams, progress, progress_fcn, results, finished_fcn );
  
  BOOST_CHECK( finished_fit_called );
  
  BOOST_REQUIRE( results->fit_src_info.size() == 1 );
  
  const ShieldingSourceFitCalc::IsoFitStruct &fit_src_info = results->fit_src_info[0];
  BOOST_REQUIRE( results->fit_src_info.size() == 1 );
  BOOST_CHECK( fit_src_info.nuclide == db->nuclide( "Cs137" ) );
  BOOST_CHECK( !fit_src_info.ageDefiningNuc );
  BOOST_CHECK( fit_src_info.sourceType == ShieldingSourceFitCalc::ModelSourceType::Point );
  
  const double activity = results->fit_src_info[0].activity;
  
  const double br = 0.8533;
  const double drf_eff = detector->efficiency( 661.657, distance );
  
  const double expected_activity = peak_area / (live_time * br * drf_eff);

  BOOST_CHECK_MESSAGE( fabs(activity - expected_activity) < 1.0E-4*expected_activity,
                       "Fit activity (" << PhysicalUnits::printToBestActivityUnits(activity,10)
                       << ") didnt match expected (" << PhysicalUnits::printToBestActivityUnits(expected_activity,10) << ")" );
}//BOOST_AUTO_TEST_CASE( SimpleSourceFit )



std::tuple<bool,int,int,vector<string>> test_fit_against_truth( const ShieldingSourceFitCalc::ModelFitResults &results )
{
  bool successful = true;
  int numCorrect = 0, numTested = 0;
  vector<string> textInfoLines;
  const SandiaDecay::SandiaDecayDataBase * const db = DecayDataBaseServer::database();    

  try
  {
    for( int i = 0; i < results.fit_src_info.size(); ++i )
    {
      const ShieldingSourceFitCalc::IsoFitStruct &src = results.fit_src_info[i];
      const SandiaDecay::Nuclide *nuc = src.nuclide;
      BOOST_REQUIRE( nuc );
      
      const SandiaDecay::Nuclide *ageNuc = src.ageDefiningNuc;
      const ShieldingSourceFitCalc::ModelSourceType sourceType = src.sourceType;
      const bool selfAttNuc = (sourceType == ShieldingSourceFitCalc::ModelSourceType::Intrinsic);
      
      // For self-attenuating shieldings, we'll just test the shielding thickness
      // For nuclides whose age is controlled by another nuclide, we dont need to test age.
      if( selfAttNuc || (ageNuc && (ageNuc != nuc)) )
        continue;
      
      bool isSelfAtten = false;
      
      for( const auto &shield : results.initial_shieldings )
      {
        const SandiaDecay::Element *el = db->element( nuc->atomicNumber );
        auto el_nucs = shield.m_nuclideFractions_.find(el); 
        if( el_nucs != end(shield.m_nuclideFractions_) )
        {
          for( const auto &nuc_frac : el_nucs->second )
            isSelfAtten |= (std::get<0>(nuc_frac) == nuc);
        }
      }//for( const auto &shield : results.initial_shieldings )

      if( src.fitActivity && !isSelfAtten )
      {
        const boost::optional<double> &truthAct = src.truthActivity;
        const boost::optional<double> &tolerance = src.truthActivityTolerance;
        
        BOOST_CHECK_MESSAGE( truthAct.has_value(), "Did not have truth activity value for " + nuc->symbol + "." );
        BOOST_CHECK_MESSAGE( tolerance.has_value(), "Did not have truth activity tolerance for " + nuc->symbol + "." );
        
        
        if( !truthAct || !tolerance )
        {
          successful = false;
          textInfoLines.push_back( "Did not have truth value for " + nuc->symbol + " activity." );
          continue;
        }
        
        const bool closeEnough = (fabs(*truthAct - src.activity) < *tolerance);
        
        numTested += 1;
        numCorrect += closeEnough;
        
        textInfoLines.push_back( "For " + nuc->symbol + " fit activity "
                                + PhysicalUnits::printToBestActivityUnits(src.activity) + " with the"
                                " truth value of "
                                + PhysicalUnits::printToBestActivityUnits(*truthAct)
                                + " and tolerance "
                                + PhysicalUnits::printToBestActivityUnits(*tolerance)
                                + (closeEnough ? " - within tolerance." : " - out of tolerance." )
                                );
        
        BOOST_CHECK_MESSAGE( fabs(*truthAct - src.activity) < *tolerance, textInfoLines.back() );
      }//if( fit activity )
      
      if( src.fitAge )
      {
        const boost::optional<double> &truthAge = src.truthAge;
        const boost::optional<double> &tolerance = src.truthAgeTolerance;
        
        BOOST_CHECK_MESSAGE( truthAge.has_value(), "Did not have truth age value for " + nuc->symbol + " age." );
        BOOST_CHECK_MESSAGE( tolerance.has_value(), "Did not have truth age tolerance for " + nuc->symbol + " age." );
      
        
        if( !truthAge || !tolerance )
        {
          successful = false;
          textInfoLines.push_back( "Did not have truth value for " + nuc->symbol + " age." );
          continue;
        }
        
        const bool closeEnough = (fabs(*truthAge - src.age) < *tolerance);
        
        numTested += 1;
        numCorrect += closeEnough;
        
        textInfoLines.push_back( "For " + nuc->symbol + " fit age "
                                + PhysicalUnits::printToBestTimeUnits(src.age) + " with the"
                                " truth value of "
                                + PhysicalUnits::printToBestTimeUnits(*truthAge)
                                + " and tolerance "
                                + PhysicalUnits::printToBestTimeUnits(*tolerance)
                                + (closeEnough ? " - within tolerance." : " - out of tolerance." )
                                );
        
        BOOST_CHECK_MESSAGE( fabs(*truthAge - src.age) < *tolerance, textInfoLines.back() );
      }//if( fit age )
    }//for( int i = 0; i < nnuc; ++i )
    
    for( const ShieldingSourceFitCalc::FitShieldingInfo &shield : results.final_shieldings )
    {
      if( shield.m_isGenericMaterial )
      {
        if( shield.m_fitDimensions[1] )
        {
          const boost::optional<double> &truth = shield.m_truthDimensions[1];
          const boost::optional<double> &tolerance = shield.m_truthDimensionsTolerances[1];
          
          BOOST_CHECK_MESSAGE( truth.has_value(), "Did not have truth value AD for generic shielding" );
          BOOST_CHECK_MESSAGE( tolerance.has_value(), "Did not have truth tolerance AD for generic shielding" );
          
          if( !truth || !tolerance )
          {
            successful = false;
            textInfoLines.push_back( "Did not have truth AD for generic shielding" );
            continue;
          }
          
          const double fitAD = shield.m_dimensions[1];
          const bool closeEnough = (fabs(*truth - fitAD) < *tolerance);
          
          numTested += 1;
          numCorrect += closeEnough;
          
          textInfoLines.push_back( "For Generic Shielding fit AN " + std::to_string(fitAD)
                                  + " with the truth value of " + std::to_string(*truth)
                                  + " and tolerance " + std::to_string(*tolerance)
                                  + (closeEnough ? " - within tolerance." : " - out of tolerance." )
                                  );
          
          BOOST_CHECK_MESSAGE( fabs(*truth - fitAD) < *tolerance, textInfoLines.back() );
        }//if( fit AD )
        
        if( shield.m_fitDimensions[0] )
        {
          const boost::optional<double> &truth = shield.m_truthDimensions[0];
          const boost::optional<double> &tolerance = shield.m_truthDimensionsTolerances[0];
          
          BOOST_CHECK_MESSAGE( truth.has_value(), "Did not have truth value AN for generic shielding" );
          BOOST_CHECK_MESSAGE( tolerance.has_value(), "Did not have truth tolerance AN for generic shielding" );
          
          if( !truth || !tolerance )
          {
            successful = false;
            textInfoLines.push_back( "Did not have truth AN for generic shielding" );
            continue;
          }
          
          const double fitAN = shield.m_dimensions[0];
          const bool closeEnough = (fabs(*truth - fitAN) < *tolerance);
          
          numTested += 1;
          numCorrect += closeEnough;
          
          textInfoLines.push_back( "For Generic Shielding fit AN " + std::to_string(fitAN)
                                  + " with the truth value of " + std::to_string(*truth)
                                  + " and tolerance " + std::to_string(*tolerance)
                                  + (closeEnough ? " - within tolerance." : " - out of tolerance." )
                                  );
          
          BOOST_CHECK_MESSAGE( fabs(*truth - fitAN) < *tolerance, textInfoLines.back() );
        }//if( fit AN )
      }else
      {
        shared_ptr<const Material> mat = shield.m_material;
        
        BOOST_CHECK( results.geometry == shield.m_geometry );
        
        BOOST_CHECK( mat );
        if( !mat )
        {
          successful = false;
          textInfoLines.push_back( "There was an invalid material." );
          continue;
        }
        
        const auto geom = shield.m_geometry;
        auto checkDimension = [shield, geom, mat, &successful, &textInfoLines, &numTested, &numCorrect]( const int dim ){
          
          if( !shield.m_fitDimensions[dim] )
            return;
          
          string labeltxt;
          const double fitValue = shield.m_dimensions[dim];
          const boost::optional<double> &thicknessVal = shield.m_truthDimensions[dim];
          const boost::optional<double> &toleranceVal = shield.m_truthDimensionsTolerances[dim];
          
          switch( geom )
          {
            case GammaInteractionCalc::GeometryType::Spherical:
              BOOST_REQUIRE( dim < 1 );
              labeltxt = "thickness";
              break;
              
            case GammaInteractionCalc::GeometryType::CylinderEndOn:
            case GammaInteractionCalc::GeometryType::CylinderSideOn:
              BOOST_REQUIRE( dim < 2 );
              if( dim == 0 )
                labeltxt = "cyl. radius";
              else if( dim == 1 )
                labeltxt = "cyl. length";
              break;
              
            case GammaInteractionCalc::GeometryType::Rectangular:
              BOOST_REQUIRE( dim < 3 );
              if( dim == 0 )
                labeltxt = "rect. width";
              else if( dim == 1 )
                labeltxt = "rect. height";
              else if( dim == 2 )
                labeltxt = "rect. depth";
              
              break;
              
            case GammaInteractionCalc::GeometryType::NumGeometryType:
              assert( 0 );
              break;
          }//switch( geometry() )
          
          
          BOOST_CHECK_MESSAGE( thicknessVal.has_value() && toleranceVal.has_value(),
                              "Missing truth " + labeltxt + " for shielding '" + mat->name + "'" );
          
          if( !thicknessVal.has_value() || !toleranceVal.has_value() )
          {
            successful = false;
            textInfoLines.push_back( "Missing truth " + labeltxt + " for shielding '" + mat->name + "'" );
            return;
          }
          
          const bool closeEnough = (fabs((*thicknessVal) - fitValue) < (*toleranceVal));
          
          numTested += 1;
          numCorrect += closeEnough;
          
          textInfoLines.push_back( "For shielding '" + mat->name + "' fit " + labeltxt + " "
                                  + PhysicalUnits::printToBestLengthUnits(fitValue,4)
                                  + " with the truth value of "
                                  + PhysicalUnits::printToBestLengthUnits(*thicknessVal,4)
                                  + " and tolerance "
                                  + PhysicalUnits::printToBestLengthUnits(*toleranceVal)
                                  + (closeEnough ? " - within tolerance." : " - out of tolerance." )
                                  );
          
          BOOST_CHECK_MESSAGE( fabs((*thicknessVal) - fitValue) < (*toleranceVal), textInfoLines.back() );
        };//auto checkDimension
        
        
        switch( geom )
        {
          case GammaInteractionCalc::GeometryType::Spherical:
            checkDimension(0);
            break;
            
          case GammaInteractionCalc::GeometryType::CylinderEndOn:
          case GammaInteractionCalc::GeometryType::CylinderSideOn:
            checkDimension(0);
            checkDimension(1);
            break;
            
          case GammaInteractionCalc::GeometryType::Rectangular:
            checkDimension(0);
            checkDimension(1);
            checkDimension(2);
            break;
            
          case GammaInteractionCalc::GeometryType::NumGeometryType:
            break;
        }//switch( geom )
        
        auto checkMassFrac = [shield, mat, &successful, &textInfoLines, &numTested, &numCorrect](
                                    const SandiaDecay::Nuclide * const nuc, 
                                    const SandiaDecay::Element * const el, 
                                    double value ){
          BOOST_REQUIRE( el );
          const auto truth_nucs_pos = shield.m_truthFitMassFractions.find(el);
          BOOST_CHECK_MESSAGE( truth_nucs_pos != end(shield.m_truthFitMassFractions),
                              "Missing truth mass-fraction for " + el->symbol );

          if( truth_nucs_pos == end(shield.m_truthFitMassFractions) )
          {
            successful = false;
            textInfoLines.push_back( "Missing truth mass-fractions for element " + el->symbol );
            return;
          }

          const map<const SandiaDecay::Nuclide *,pair<double,double>> &nucs = truth_nucs_pos->second;
          auto truth_pos = nucs.find(nuc);
          BOOST_CHECK_MESSAGE( truth_pos != end(nucs),
                                "Missing truth mass-fraction for " + (nuc ? nuc->symbol : "other nuclides") );
          if( truth_pos == end(nucs) )
          {
            successful = false;
            textInfoLines.push_back( "Missing truth mass-fraction for " + (nuc ? nuc->symbol : "other nuclides") );
            return;
          }                           
          
                                  
          const double truthval = get<0>(truth_pos->second);
          const double truthtol = get<1>(truth_pos->second);
          BOOST_CHECK_MESSAGE( (truthval >= 0.0) && (truthval <= 1.0) && (truthtol >= 0.0) && (truthtol <= 1.0),
                                "Invalid truth mass-fraction for " + (nuc ? nuc->symbol : "other nuclides") );
                                      
          if( (truthval < 0.0) || (truthval > 1.0) || (truthtol < 0.0) || (truthtol > 1.0) )
          {
            successful = false;
            textInfoLines.push_back( "Invalid truth mass-fraction for " + (nuc ? nuc->symbol : "other nuclides") );
            return;
          }
            
          numTested += 1;
          const bool closeEnough = ((value >= (truthval - truthtol)) && (value <= (truthval + truthtol)));
          numCorrect += closeEnough;
            
          textInfoLines.push_back( "For shielding '" + mat->name + "' fit " + (nuc ? nuc->symbol : "other nuclides")
                                     + " to have mass fraction " + SpecUtils::printCompact(value, 5)
                                     + " with the truth value of " + SpecUtils::printCompact(truthval,5)
                                     + " and tolerance " + SpecUtils::printCompact(truthtol,5)
                                     + (closeEnough ? " - within tolerance." : " - out of tolerance." ) );
                                      
          BOOST_CHECK_MESSAGE( (value >= (truthval - truthtol)) && (value <= (truthval + truthtol)),
                                textInfoLines.back() );
        };//checkMassFrac( ... )
          
        for( const auto &el_nucs : shield.m_nuclideFractions_ )
        {
          const SandiaDecay::Element * const el = el_nucs.first;
          for( const tuple<const SandiaDecay::Nuclide *,double,bool> &nuc_frac : el_nucs.second )
          {
            checkMassFrac( std::get<0>(nuc_frac), el, std::get<1>(nuc_frac) );
          }
        }//for( const auto &prev_nuc_frac : shield.m_nuclideFractionUncerts )
      }//if( generic material ) / else
    }//for( WWidget *widget : m_shieldingSelects->children() )
    
    successful = (successful && numTested);
  }catch( std::exception &e )
  {
    successful = false;
    textInfoLines.push_back( "Caught exception during testing: " + string(e.what()) );
  }//try / catch
  
  return tuple<bool,int,int,vector<string>>( successful, numCorrect, numTested, textInfoLines );
}//std::tuple<bool,int,int,vector<string>> test_fit_against_truth( const ShieldingSourceFitCalc::ModelFitResults &results )



void set_fit_quantities_to_default_values( vector<ShieldingSourceFitCalc::ShieldingInfo> &shield_definitions,
                                     vector<ShieldingSourceFitCalc::SourceFitDef> &src_definitions )
{
  for( ShieldingSourceFitCalc::SourceFitDef &src : src_definitions )
  {
    const SandiaDecay::Nuclide *nuc = src.nuclide;
    BOOST_REQUIRE( nuc );
    
    const SandiaDecay::Nuclide *ageNuc = src.ageDefiningNuc;
    const ShieldingSourceFitCalc::ModelSourceType sourceType = src.sourceType;
    const bool selfAttNuc = (sourceType == ShieldingSourceFitCalc::ModelSourceType::Intrinsic);
    
    // For self-attenuating shieldings, we'll just test the shielding thickness
    // For nuclides whose age is controlled by another nuclide, we dont need to test age.
    if( selfAttNuc || (ageNuc && (ageNuc != nuc)) )
      continue;
    
    if( src.fitActivity )
      src.activity = 0.001*PhysicalUnits::curie;
    
    if( src.fitAge )
      src.age = PeakDef::defaultDecayTime( nuc, nullptr );
  }//for( int i = 0; i < nnuc; ++i )
  
  
  for( ShieldingSourceFitCalc::ShieldingInfo &shield : shield_definitions )
  {
    if( shield.m_isGenericMaterial )
    {
      if( shield.m_fitDimensions[0] )
        shield.m_dimensions[0] = 26; // Atomic Number
      
      if( shield.m_fitDimensions[1] )
        shield.m_dimensions[1] = 0;  // Areal Density
    }else
    {
      shared_ptr<const Material> mat = shield.m_material;
      if( !mat )
        continue;
      
      // Set mass-fractions to all be even, if we are fitting for them
      for( auto &el_nucs : shield.m_nuclideFractions_ )
      {
        int num_fit = 0;
        double frac_fit_sum = 0.0;
          
        for( const tuple<const SandiaDecay::Nuclide *,double,bool> &nuc_frac : el_nucs.second )
        {
          num_fit += std::get<2>(nuc_frac);
          if( std::get<2>(nuc_frac) )
            frac_fit_sum += std::get<1>(nuc_frac);
        }
        assert( num_fit != 1 );
        for( tuple<const SandiaDecay::Nuclide *,double,bool> &nuc_frac : el_nucs.second )
        {
          if( std::get<2>(nuc_frac) )
            std::get<1>(nuc_frac) = (frac_fit_sum / num_fit);
        }
      }//for( auto &el_nucs : shield.m_nuclideFractions_ )
      
      
      switch( shield.m_geometry )
      {
        case GammaInteractionCalc::GeometryType::Spherical:
          if( shield.m_fitDimensions[0] )
            shield.m_dimensions[0] = 1.0*PhysicalUnits::cm;
          break;
          
        case GammaInteractionCalc::GeometryType::CylinderEndOn:
        case GammaInteractionCalc::GeometryType::CylinderSideOn:
          if( shield.m_fitDimensions[0] )
            shield.m_dimensions[0] = 0.5*PhysicalUnits::cm;
          if( shield.m_fitDimensions[1] )
            shield.m_dimensions[1] = 0.5*PhysicalUnits::cm;
          break;
          
        case GammaInteractionCalc::GeometryType::Rectangular:
          if( shield.m_fitDimensions[0] )
            shield.m_dimensions[0] = 0.5*PhysicalUnits::cm;
          if( shield.m_fitDimensions[1] )
            shield.m_dimensions[1] = 0.5*PhysicalUnits::cm;
          if( shield.m_fitDimensions[2] )
            shield.m_dimensions[2] = 0.5*PhysicalUnits::cm;
          break;
          
        case GammaInteractionCalc::GeometryType::NumGeometryType:
          assert( 0 );
          break;
      }//switch( geometry() )
      
      for( ShieldingSourceFitCalc::TraceSourceInfo &trace : shield.m_traceSources )
      {
        if( !trace.m_fitActivity )
          continue;
        
        switch( trace.m_type )
        {
          case GammaInteractionCalc::TraceActivityType::TotalActivity:
            trace.m_activity = 0.001*PhysicalUnits::curie;;
            break;
          case GammaInteractionCalc::TraceActivityType::ActivityPerCm3:
          case GammaInteractionCalc::TraceActivityType::ExponentialDistribution:
          case GammaInteractionCalc::TraceActivityType::ActivityPerGram:
            trace.m_activity = 1.0E-6*PhysicalUnits::curie;;
            break;
            
          case GammaInteractionCalc::TraceActivityType::NumTraceActivityType:
            assert( 0 );
            break;
        }//switch( trace.m_type )
      }//switch( trace.m_type )
    }//if( generic material ) / else
  }//for( WWidget *widget : m_shieldingSelects->children() )
}//void set_fit_quantities_to_default_values(...)


BOOST_AUTO_TEST_CASE( FitAnalystTraceSource )
{
  set_data_dir();
  
  const SandiaDecay::SandiaDecayDataBase * const db = DecayDataBaseServer::database();
  BOOST_REQUIRE_MESSAGE( db, "Error initing SandiaDecayDataBase" );
 
  MaterialDB matdb;
  
  const string materialfile = SpecUtils::append_path( InterSpec::staticDataDirectory(), "MaterialDataBase.txt" );
  BOOST_REQUIRE_NO_THROW( matdb.parseGadrasMaterialFile( materialfile, db, false ) );
  
  const Material * const iron = matdb.material("Fe (iron)");
  BOOST_REQUIRE( iron );
  
  const string test_n42_file = SpecUtils::append_path(g_test_file_dir, "../analysis_tests/AEGIS_Eu152_surface_contamination.n42_20230622T113239.276178.n42");
  BOOST_REQUIRE( SpecUtils::is_file(test_n42_file) );
  
  
  
  SpecMeas specfile;
  BOOST_REQUIRE( specfile.load_N42_file( test_n42_file ) );
  BOOST_REQUIRE( specfile.sample_numbers().size() == 1 );
  BOOST_REQUIRE( specfile.num_measurements() == 1 );
  
  shared_ptr<const deque<shared_ptr<const PeakDef>>> peaks = specfile.peaks( specfile.sample_numbers() );
  BOOST_REQUIRE( peaks && (peaks->size() > 10) );
  
  shared_ptr<const SpecUtils::Measurement> foreground = specfile.measurements()[0];
  BOOST_REQUIRE( foreground );
  
  shared_ptr<const DetectorPeakResponse> detector = specfile.detector();
  BOOST_REQUIRE( detector && detector->isValid() );
  
  rapidxml::xml_document<char> *model_xml = specfile.shieldingSourceModel();
  BOOST_REQUIRE( model_xml );
  rapidxml::xml_node<char> *base_node = model_xml->first_node();
  BOOST_REQUIRE( base_node );
  
  //string xml;
  //rapidxml::print(std::back_inserter(xml), *model_xml, 0);
  //cout << "XML: " << xml << endl;
  
  const rapidxml::xml_node<char> *geom_node = base_node->first_node( "Geometry" );
  BOOST_REQUIRE( geom_node );
  const rapidxml::xml_node<char> *dist_node = base_node->first_node( "Distance" );
  BOOST_REQUIRE( dist_node );
  
  const string diststr = SpecUtils::xml_value_str( dist_node );
  double distance;
  BOOST_REQUIRE_NO_THROW( distance = PhysicalUnits::stringToDistance( diststr ) );
  
  const string geomstr = SpecUtils::xml_value_str( geom_node );
  GammaInteractionCalc::GeometryType geometry = GammaInteractionCalc::GeometryType::NumGeometryType;
  for( GammaInteractionCalc::GeometryType type = GammaInteractionCalc::GeometryType(0);
      type != GammaInteractionCalc::GeometryType::NumGeometryType;
      type = GammaInteractionCalc::GeometryType(static_cast<int>(type) + 1) )
  {
    if( SpecUtils::iequals_ascii(geomstr, GammaInteractionCalc::to_str(type)) )
    {
      geometry = type;
      break;
    }
  }
  
  BOOST_REQUIRE( geometry != GammaInteractionCalc::GeometryType::NumGeometryType );
  
  vector<ShieldingSourceFitCalc::ShieldingInfo> shield_definitions;
  vector<ShieldingSourceFitCalc::SourceFitDef> src_definitions;
  ShieldingSourceFitCalc::ShieldingSourceFitOptions options;
  BOOST_REQUIRE_NO_THROW( options.deSerialize( base_node ) );
  
  const rapidxml::xml_node<char> *shieldings_node = base_node->first_node( "Shieldings" );
  BOOST_REQUIRE( shieldings_node );
  
  XML_FOREACH_CHILD(shield_node, shieldings_node, "Shielding")
  {
    ShieldingSourceFitCalc::ShieldingInfo info;
    BOOST_REQUIRE_NO_THROW( info.deSerialize( shield_node, &matdb ) );
    shield_definitions.push_back( info );
  }
  
  BOOST_REQUIRE( shield_definitions.size() == 1 );
  
  const rapidxml::xml_node<char> *srcs_node = base_node->first_node( "Nuclides" );
  BOOST_REQUIRE( srcs_node );
  
  XML_FOREACH_CHILD( src_node, srcs_node, "Nuclide" )
  {
    ShieldingSourceFitCalc::SourceFitDef info;
    BOOST_REQUIRE_NO_THROW( info.deSerialize( src_node ) );
    src_definitions.push_back( info );
  }
  BOOST_REQUIRE( src_definitions.size() == 1 );
  
  set_fit_quantities_to_default_values( shield_definitions, src_definitions );
  
  // We have all the parts, lets do the computation:
  pair<shared_ptr<GammaInteractionCalc::ShieldingSourceChi2Fcn>, ROOT::Minuit2::MnUserParameters> fcn_pars =
                GammaInteractionCalc::ShieldingSourceChi2Fcn::create( distance, geometry,
                                  shield_definitions, src_definitions, detector,
                                  foreground, nullptr, *peaks, nullptr, options );
 
  auto inputPrams = make_shared<ROOT::Minuit2::MnUserParameters>();
  *inputPrams = fcn_pars.second;
  
  auto progress = make_shared<ShieldingSourceFitCalc::ModelFitProgress>();
  auto results = make_shared<ShieldingSourceFitCalc::ModelFitResults>();
  
  
  auto progress_fcn = [progress](){
    // Probably wont get called, since its a simple fit
  };
  
  bool finished_fit_called = false;
  auto finished_fcn = [results, &finished_fit_called](){
    finished_fit_called = true;
  };
  
  ShieldingSourceFitCalc::fit_model( "", fcn_pars.first, inputPrams, progress, progress_fcn, results, finished_fcn );
  
  BOOST_CHECK( finished_fit_called );
  
  BOOST_CHECK_MESSAGE( results->successful == ShieldingSourceFitCalc::ModelFitResults::FitStatus::Final,
                      "Fit status was not successful" );
  
  BOOST_REQUIRE( results->fit_src_info.size() >= 1 );
  
  tuple<bool,int,int,vector<string>> test_results = test_fit_against_truth( *results );
  const bool successful = get<0>(test_results);
  const int numCorrect = get<1>(test_results);
  const int numTested = get<2>(test_results);
  const vector<string> &textInfoLines = get<3>(test_results);
  
  cout << (successful ? "Successfully" : "Unsuccessfully") << " checked against truth values with "
       << numCorrect << " tests passing, out of " << numTested << ".\nNotes:" << endl;
  for( const auto &msg : textInfoLines )
    cout << "\t" << msg << endl;
}//BOOST_AUTO_TEST_CASE( FitAnalystTraceSource )


BOOST_AUTO_TEST_CASE( FitAnalystShieldingSourcecases )
{
  set_data_dir();

  cout << "Starting FitAnalystShieldingSourcecases" << endl;

  const SandiaDecay::SandiaDecayDataBase * const db = DecayDataBaseServer::database();
  BOOST_REQUIRE_MESSAGE( db, "Error initing SandiaDecayDataBase" );
 
  MaterialDB matdb;
  
  const string materialfile = SpecUtils::append_path( InterSpec::staticDataDirectory(), "MaterialDataBase.txt" );
  BOOST_REQUIRE_NO_THROW( matdb.parseGadrasMaterialFile( materialfile, db, false ) );
  
  const Material * const iron = matdb.material("Fe (iron)");
  BOOST_REQUIRE( iron );
  
  const string base_dir = SpecUtils::append_path(g_test_file_dir, "../analysis_tests");
  BOOST_REQUIRE( SpecUtils::is_directory(base_dir) );
  
  const vector<string> files = SpecUtils::recursive_ls( base_dir, ".n42" );
  
  // Make sure we found some files
  BOOST_REQUIRE( files.size() >= 1 );
  
  for( const string n42_filename : files )
  {
    SpecMeas specfile;
    const bool loaded = specfile.load_N42_file( n42_filename );
    BOOST_CHECK_MESSAGE( loaded, "Analyst file '" << n42_filename << "' couldnt be loaded - skipping testing." );
    if( !loaded )
      continue;
    
    rapidxml::xml_document<char> *model_xml = specfile.shieldingSourceModel();
    BOOST_CHECK_MESSAGE( model_xml, "Analyst file '" << n42_filename << "' doesnt have a ShieldingSourceModel - skipping test." );
    if( !model_xml )
      continue;
    
    rapidxml::xml_node<char> *base_node = model_xml->first_node();
    BOOST_CHECK_MESSAGE( base_node, "Analyst file '" << n42_filename << "' has invalid ShieldingSourceModel - skipping test." );
    if( !base_node )
      continue;
    
    // We will go through and try to figure out what the "foreground" should be.
    //  Note: we could probably just call `specfile.displayedSampleNumbers()`, but we'll
    //        be hard on ourselves
    const vector<string> &detectors = specfile.detector_names();
    const set<set<int>> samplesWithPeaks = specfile.sampleNumsWithPeaks();
    size_t num_foreground = 0, num_background = 0;
    set<int> foreground_samples, background_samples;
    
    for( const set<int> &samples : samplesWithPeaks )
    {
      bool classified_sample_nums = false;
      for( const int sample : samples )
      {
        for( const string &det : detectors )
        {
          auto m = specfile.measurement( sample, det );
          if( !m )
            continue;
          
          switch( m->source_type() )
          {
            case SpecUtils::SourceType::IntrinsicActivity:
            case SpecUtils::SourceType::Calibration:
              break;
              
            case SpecUtils::SourceType::Background:
              classified_sample_nums = true;
              num_background += 1;
              background_samples = samples;
              break;
            
            case SpecUtils::SourceType::Foreground:
            case SpecUtils::SourceType::Unknown:
              classified_sample_nums = true;
              num_foreground += 1;
              foreground_samples = samples;
              break;
          }//switch( m->source_type() )
          
          if( classified_sample_nums )
            break;
        }//for( const string &det : detectors )
        
        if( classified_sample_nums )
          break;
      }//for( const int sample : samples )
      
      assert( !classified_sample_nums || ((num_foreground >= 1) || (num_background >= 1)) );
    }//for( const set<int> &samples : samplesWithPeaks )
    
    if( (num_background == 1) && (num_foreground == 0) )
    {
      std::swap( num_background, num_foreground );
      std::swap( foreground_samples, background_samples );
    }
    
    // Check that it is unambiguous what foreground the model file is to
    BOOST_CHECK_MESSAGE( num_foreground == 1,
                        "Analyst file '" << n42_filename << "' does not have exactly one foreground"
                        " - skipping test." );
    if( num_foreground != 1 )
      continue;
    
    const shared_ptr<const SpecUtils::Measurement> foreground
                 = ((foreground_samples.size() == 1) && (detectors.size() == 1))
                   ? specfile.measurement( *begin(foreground_samples), detectors.front() )
                   : specfile.sum_measurements( foreground_samples, detectors,  nullptr );
    
    BOOST_CHECK_MESSAGE( foreground,
                        "Analyst file '" << n42_filename << "' failed to extract foreground"
                        " - skipping test." );
    if( !foreground )
      continue;
    
    
    // Find foreground that has act/shielding fit associated
    shared_ptr<const deque<shared_ptr<const PeakDef>>> foreground_peaks = specfile.peaks( foreground_samples );
    BOOST_CHECK_MESSAGE( foreground_peaks && (foreground_peaks->size() >= 1),
                         "Analyst file '" << n42_filename << "' didnt have peaks for the"
                         " identified foreground - skipping test." );
    if( !foreground_peaks || foreground_peaks->empty() )
      continue;
        
    shared_ptr<const DetectorPeakResponse> detector = specfile.detector();
    BOOST_CHECK_MESSAGE( detector && detector->isValid(),
                        "Analyst file '" << n42_filename << "' didnt have a DRF - skipping test." );
    
    if( !detector || !detector->isValid() )
      continue;
    
    
    //string xml;
    //rapidxml::print(std::back_inserter(xml), *model_xml, 0);
    //cout << "XML: " << xml << endl;
    
    double distance;
    try
    {
      const rapidxml::xml_node<char> *dist_node = base_node->first_node( "Distance" );
      const string diststr = SpecUtils::xml_value_str( dist_node );
      distance = PhysicalUnits::stringToDistance( diststr );
    }catch( std::exception &e )
    {
      BOOST_CHECK_MESSAGE( false,
                          "Analyst file '" << n42_filename << "' <Distance> node invalid: "
                          << e.what() << " - skipping test." );
      continue;
    }//try / catch to get distance
    
    GammaInteractionCalc::GeometryType geometry = GammaInteractionCalc::GeometryType::NumGeometryType;
    
    const rapidxml::xml_node<char> *geom_node = base_node->first_node( "Geometry" );
    const string geomstr = SpecUtils::xml_value_str( geom_node );
    
    for( GammaInteractionCalc::GeometryType type = GammaInteractionCalc::GeometryType(0);
        type != GammaInteractionCalc::GeometryType::NumGeometryType;
        type = GammaInteractionCalc::GeometryType(static_cast<int>(type) + 1) )
    {
      if( SpecUtils::iequals_ascii(geomstr, GammaInteractionCalc::to_str(type)) )
      {
        geometry = type;
        break;
      }
    }
    
    BOOST_CHECK_MESSAGE( geometry != GammaInteractionCalc::GeometryType::NumGeometryType,
                        "Analyst file '" << n42_filename << "' <Geometry> node missing or invalid"
                        " - skipping test.");
    if( geometry == GammaInteractionCalc::GeometryType::NumGeometryType )
      continue;
    
    
    vector<ShieldingSourceFitCalc::ShieldingInfo> shield_definitions;
    vector<ShieldingSourceFitCalc::SourceFitDef> src_definitions;
    ShieldingSourceFitCalc::ShieldingSourceFitOptions options;
    try
    {
      options.deSerialize( base_node );
    }catch( std::exception &e )
    {
      BOOST_CHECK_MESSAGE( false,
                          "Analyst file '" << n42_filename << "' Options invalid: "
                          << e.what()
                          << " - skipping test.");
      continue;
    }

    
    BOOST_CHECK_MESSAGE( !options.background_peak_subtract || (num_background <= 1),
                        "Analyst file '" << n42_filename << "' Options indicated"
                        " background-subtract, but background is ambiguous. - skipping test.");
    if( options.background_peak_subtract && (num_background > 1) )
      continue;
    
    // It looks like background subtract _could_ be set to true, even if we didnt have any
    //  background peaks for the fit - at least if things are defined not using the GUI (the
    //  GUI enforces background peaks must be present to have this option checked) so I guess
    //  we'll just be a bit loose around this for now, but we probably dont have to be.
    shared_ptr<const SpecUtils::Measurement> background;
    shared_ptr<const deque<shared_ptr<const PeakDef>>> background_peaks;
    if( options.background_peak_subtract && (num_background == 1) )
    {
      background = specfile.sum_measurements( background_samples, detectors, nullptr );
      BOOST_CHECK_MESSAGE( background && (background->num_gamma_channels() > 16),
                          "Analyst file '" << n42_filename << "' Options indicated"
                          " background-subtract, but background couldnt be extracted - skipping test.");
      if( !background || (background->num_gamma_channels() <= 16) )
        continue;
      
      background_peaks = specfile.peaks( background_samples );
      
      if( background_peaks && background_peaks->empty() )
      {
        background = nullptr;
        background_peaks.reset();
      }
      
      BOOST_WARN_MESSAGE( background_peaks, "Analyst file '" + n42_filename + "' Options indicated"
                   " background-subtract, but no background peaks found - continuing anyway." );
    }//if( options.background_peak_subtract && (num_background == 1) )
    
    
    const rapidxml::xml_node<char> *shieldings_node = base_node->first_node( "Shieldings" );
    BOOST_REQUIRE( shieldings_node );
    
    bool success_parsing = true;
    XML_FOREACH_CHILD(shield_node, shieldings_node, "Shielding")
    {
      try
      {
        ShieldingSourceFitCalc::ShieldingInfo info;
        info.deSerialize( shield_node, &matdb );
        shield_definitions.push_back( std::move(info) );
      }catch( std::exception &e )
      {
        success_parsing = false;
        BOOST_CHECK_MESSAGE( false,
                            "Analyst file '" << n42_filename << "' <Shielding> node invalid: "
                             << e.what() << " - skipping test.");
      }
    }//XML_FOREACH_CHILD(shield_node, shieldings_node, "Shielding")
    
    if( !success_parsing )
      continue;
    
    
    const rapidxml::xml_node<char> *srcs_node = base_node->first_node( "Nuclides" );
    BOOST_CHECK_MESSAGE( srcs_node,
                        "Analyst file '" << n42_filename << "' missing <Nuclides> node: "
                         " - skipping test.");
    
    XML_FOREACH_CHILD( src_node, srcs_node, "Nuclide" )
    {
      try
      {
        ShieldingSourceFitCalc::SourceFitDef info;
        info.deSerialize( src_node );
        src_definitions.push_back( std::move(info) );
      }catch( std::exception &e )
      {
        success_parsing = false;
        BOOST_CHECK_MESSAGE( false,
                            "Analyst file '" << n42_filename << "' <Nuclide> node invalid: "
                             << e.what() << " - skipping test.");
      }// try / catch
    }//XML_FOREACH_CHILD( src_node, srcs_node, "Nuclide" )
    
    if( !success_parsing )
      continue;
    
    BOOST_CHECK_MESSAGE( src_definitions.size() >= 1,
                        "Analyst file '" << n42_filename << "' no sources defined." );
    
    // Check that the number of fit nuclides in `src_definitions` matches the sources in the peaks
    set<const SandiaDecay::Nuclide *> peak_fit_nucs;
    for( const auto &p : *foreground_peaks )
    {
      if( p->parentNuclide() && p->useForShieldingSourceFit())
        peak_fit_nucs.insert( p->parentNuclide() );
    }
    
    BOOST_CHECK_MESSAGE( peak_fit_nucs.size() == src_definitions.size(),
                        "Analyst file '" << n42_filename << "' has mismatching number of sources in peaks ("
                        << peak_fit_nucs.size() << "), vs src_definitions (" << src_definitions.size() << ")" );
    
    
    set_fit_quantities_to_default_values( shield_definitions, src_definitions );
    
    // We have all the parts, lets do the computation:
    pair<shared_ptr<GammaInteractionCalc::ShieldingSourceChi2Fcn>, ROOT::Minuit2::MnUserParameters> fcn_pars =
    GammaInteractionCalc::ShieldingSourceChi2Fcn::create( distance, geometry,
                                                         shield_definitions, src_definitions, detector,
                                                         foreground, background, *foreground_peaks, background_peaks, options );
    
    auto inputPrams = make_shared<ROOT::Minuit2::MnUserParameters>();
    *inputPrams = fcn_pars.second;
    
    auto progress = make_shared<ShieldingSourceFitCalc::ModelFitProgress>();
    auto results = make_shared<ShieldingSourceFitCalc::ModelFitResults>();
    
    
    auto progress_fcn = [progress](){
      // Probably wont get called, since its a simple fit
    };
    
    bool finished_fit_called = false;
    auto finished_fcn = [results, &finished_fit_called](){
      finished_fit_called = true;
    };
    
    ShieldingSourceFitCalc::fit_model( "", fcn_pars.first, inputPrams, progress, progress_fcn, results, finished_fcn );
    
    BOOST_CHECK( finished_fit_called );
    
    BOOST_CHECK_MESSAGE( results->successful == ShieldingSourceFitCalc::ModelFitResults::FitStatus::Final,
                        "Analyst file '" << n42_filename << "': Fit status was not successful" );
    
    BOOST_CHECK_MESSAGE( results->fit_src_info.size() == src_definitions.size(),
                        "Analyst file '" << n42_filename << "': Fit sources size not equal to input." );
    
    tuple<bool,int,int,vector<string>> test_results = test_fit_against_truth( *results );
    const bool successful = get<0>(test_results);
    const int numCorrect = get<1>(test_results);
    const int numTested = get<2>(test_results);
    const vector<string> &textInfoLines = get<3>(test_results);
    
    cout << (successful ? "Successfully" : "Unsuccessfully") << " checked against truth values with "
    << numCorrect << " tests passing, out of " << numTested << ".\nNotes:" << endl;
    for( const auto &msg : textInfoLines )
      cout << "\t" << msg << endl;
  }//for( const string n42_filename : files )
}//BOOST_AUTO_TEST_CASE( FitAnalystTraceSource )
