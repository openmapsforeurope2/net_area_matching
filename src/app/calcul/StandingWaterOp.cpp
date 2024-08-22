// APP
#include <app/calcul/StandingWaterOp.h>
#include <app/params/ThemeParameters.h>

// BOOST
#include <boost/progress.hpp>

// EPG
#include <epg/Context.h>
#include <epg/tools/TimeTools.h>
#include <epg/tools/StringTools.h>

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

	epg::tools::StringTools::Split(_borderCode, "#", _vCountriesCodeName);

    //--
	_fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);

	//--
	std::string areaStandingWaterTableName = themeParameters->getValue(AREA_TABLE_INIT_STANDING_WATER).toString();
	_fsAreaStandingWater = context->getDataBaseManager().getFeatureStore(areaStandingWaterTableName, idName, geomName);


	_attrValueStandingWater = "standing_water";
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

	//--
	epg::Context *context = epg::ContextS::getInstance();

	// epg parameters
	epg::params::EpgParameters const& epgParams = context->getEpgParameters();
	std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
	// app parameters
	params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
	std::string wTag = themeParameters->getValue(W_TAG).toString();

	ign::feature::FeatureFilter filterCountries("(" + countryCodeName + " = '" + _vCountriesCodeName[0] + "' or " + countryCodeName + " = '" + _vCountriesCodeName[1] + "')");
	ign::feature::FeatureIteratorPtr itStandingArea = _fsAreaStandingWater->getFeatures(filterCountries);
	int numFeaturesStandingWater = context->getDataBaseManager().numFeatures(*_fsAreaStandingWater, filterCountries);
	boost::progress_display display(numFeaturesStandingWater, std::cout, "[ IMPORT STANDING WATER]\n");
	std::set<std::string> sIdStandingArea2delete;
	while (itStandingArea->hasNext()) {
		++display;
		ign::feature::Feature fStandingArea = itStandingArea->next();

		std::string countryCodeStandingArea = fStandingArea.getAttribute(countryCodeName).toString();
/*
		//on test si le standing water ne superpose pas un watercourse
		bool isIntersectingWatercourse = false;
		double areaMaxIntersection = 10;
		ign::feature::FeatureFilter filterArroundStandingArea(countryCodeName + " = '" + countryCodeStandingArea + "'");;
		filterArroundStandingArea.setExtent(fStandingArea.getGeometry().getEnvelope());
		ign::feature::FeatureIteratorPtr itWaterCrsArea = _fsArea->getFeatures(filterArroundStandingArea);
		while (itWaterCrsArea->hasNext()) {
			ign::feature::Feature fWaterCrsArea = itWaterCrsArea->next();
			if (fWaterCrsArea.getGeometry().distance(fStandingArea.getGeometry()) > 0)
				continue;
			ign::geometry::Geometry* ptrGeomIntersected = fWaterCrsArea.getGeometry().Intersection(fStandingArea.getGeometry());
			double areaIntersected = 0;
			if (ptrGeomIntersected->isMultiPolygon())
				areaIntersected = ptrGeomIntersected->asMultiPolygon().area();
			else if(ptrGeomIntersected->isPolygon())
				areaIntersected = ptrGeomIntersected->asPolygon().area();
			if (areaIntersected > areaMaxIntersection) { //ajouter l'aire d'intersection? si intersection avec plusieurs?
				isIntersectingWatercourse = true;
				break;
			}
			ptrGeomIntersected = 0;
			delete ptrGeomIntersected;
		}

		if (isIntersectingWatercourse)
			continue;
*/
		fStandingArea.setAttribute(wTag, ign::data::String(_attrValueStandingWater));
		sIdStandingArea2delete.insert(fStandingArea.getId());
		_fsArea->createFeature(fStandingArea);
	}


	for (std::set<std::string>::iterator sit = sIdStandingArea2delete.begin(); sit != sIdStandingArea2delete.end(); ++sit)
		_fsAreaStandingWater->deleteFeature(*sit);

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

	//--
	epg::Context *context = epg::ContextS::getInstance();
	// app parameters
	params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
	std::string wTag = themeParameters->getValue(W_TAG).toString();

	ign::feature::FeatureFilter filterStandingArea(wTag + " = '"+_attrValueStandingWater+"#"+ _attrValueStandingWater+"' or "
	+ wTag + " = '" + _attrValueStandingWater + "'");
	ign::feature::FeatureIteratorPtr itStandingArea = _fsArea->getFeatures(filterStandingArea);
	int numFeaturesStandingWater = context->getDataBaseManager().numFeatures(*_fsArea, filterStandingArea);
	boost::progress_display display(numFeaturesStandingWater, std::cout, "[ EXPORT STANDING WATER]\n");
	std::set<std::string> sIdStandingArea2delete;
	while (itStandingArea->hasNext()) {
		++display;
		ign::feature::Feature fStandingArea = itStandingArea->next();

		sIdStandingArea2delete.insert(fStandingArea.getId());
		_fsAreaStandingWater->createFeature(fStandingArea);
	}

	for (std::set<std::string>::iterator sit = sIdStandingArea2delete.begin(); sit != sIdStandingArea2delete.end(); ++sit)
		_fsArea->deleteFeature(*sit);

}