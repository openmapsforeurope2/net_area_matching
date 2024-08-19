// APP
#include <app/calcul/StandingWaterOp.h>
#include <app/params/ThemeParameters.h>

// BOOST
#include <boost/progress.hpp>

// EPG
#include <epg/Context.h>
#include <epg/tools/TimeTools.h>

///
///
///
app::calcul::StandingWaterOp::StandingWaterOp(
    std::string borderCode,
    bool verbose
) : 
	_borderCode(borderCode),
    _verbose(verbose)
{
    _init();
}

///
///
///
app::calcul::StandingWaterOp::~StandingWaterOp()
{
    //_shapeLogger->closeShape("");
}



///
///
///
void app::calcul::StandingWaterOp::_init()
{
    //--
    _logger = epg::log::EpgLoggerS::getInstance();
    _logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

    //--
    _shapeLogger = epg::log::ShapeLoggerS::getInstance();
	//_shapeLogger->addShape("", epg::log::ShapeLogger::POLYGON);

    //--
    epg::Context *context = epg::ContextS::getInstance();

    // epg parameters
    epg::params::EpgParameters const& epgParams = context->getEpgParameters();
    std::string const boundaryTableName = epgParams.getValue(TARGET_BOUNDARY_TABLE).toString();
    std::string const areaTableName = epgParams.getValue(AREA_TABLE).toString();
    std::string const idName = epgParams.getValue(ID).toString();
    std::string const geomName = epgParams.getValue(GEOM).toString();
    std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
	std::string const linkedFeatIdName = context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString();
            
    // app parameters
    params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();

    //--
	_fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);

	//--
	std::string areaStandingWaterTableName = themeParameters->getValue(AREA_TABLE_INIT_STANDING_WATER).toString();
	_fsAreaStandingWater = context->getDataBaseManager().getFeatureStore(areaStandingWaterTableName, idName, geomName);

    //--
    _logger->log(epg::log::INFO, "[END] initialization: " + epg::tools::TimeTools::getTime());
};


///
///
///
void app::calcul::StandingWaterOp::AddStandingWater( std::string borderCode,
	bool verbose)
{
	app::calcul::StandingWaterOp standingWaterOp(borderCode, verbose);
	standingWaterOp._addStandingWater();
}

///
///
///
void app::calcul::StandingWaterOp::_addStandingWater()
{
}

///
///
///
void app::calcul::StandingWaterOp::SortingStandingWater(std::string borderCode,
	bool verbose)
{
	app::calcul::StandingWaterOp standingWaterOp(borderCode, verbose);
	standingWaterOp._sortingStandingWater();
}

///
///
///
void app::calcul::StandingWaterOp::_sortingStandingWater()
{
}