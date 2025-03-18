// APP
#include <app/calcul/GenerateCuttingLinesOp.h>
#include <app/params/ThemeParameters.h>

// BOOST
#include <boost/progress.hpp>

//SOCLE
#include <ign/geometry/graph/tools/SnapRoundPlanarizer.h>
#include <ign/geometry/algorithm/LineMergerOpGeos.h>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <epg/sql/tools/numFeatures.h>
#include <epg/sql/DataBaseManager.h>
#include <epg/tools/StringTools.h>
#include <epg/tools/TimeTools.h>
#include <epg/tools/FilterTools.h>

//OME2
#include <ome2/calcul/detail/ClMerger.h>

///
///
///
app::calcul::GenerateCuttingLinesOp::GenerateCuttingLinesOp(
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
app::calcul::GenerateCuttingLinesOp::~GenerateCuttingLinesOp()
{
}

///
///
///
void app::calcul::GenerateCuttingLinesOp::Compute(
	std::string borderCode, 
	bool verbose
) {
	GenerateCuttingLinesOp op(borderCode, verbose);
	op._compute();
}

///
///
///
void app::calcul::GenerateCuttingLinesOp::_compute() const
{
	std::vector<std::string> vCountry;
	epg::tools::StringTools::Split(_borderCode, "#", vCountry);

	for (size_t i = 0; i < vCountry.size(); ++i) 
		_generateCutlByCountry(vCountry[i]);
	
}

///
///
///
void app::calcul::GenerateCuttingLinesOp::_init()
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

	std::string cutlTableName = themeParameters->getValue(CUTL_TABLE).toString();
	if (cutlTableName == "") {
		std::string const cutlTableSuffix = themeParameters->getValue(CUTL_TABLE_SUFFIX).toString();
		cutlTableName = themeParameters->getParameter(AREA_TABLE_INIT).getValue().toString() + cutlTableSuffix;
	}
	{
		std::ostringstream ss;
		ss << "DROP TABLE IF EXISTS " << cutlTableName << " ;";
		ss << "CREATE TABLE " << cutlTableName <<"("
			<< idName << " uuid DEFAULT gen_random_uuid(),"
			<< geomName << " geometry(LineStringZ),"
			<< countryCodeName << " character varying(8),"
			<< linkedFeatIdName << " character varying(255) "
			<< ");";
		//ajout d'index?
		context->getDataBaseManager().getConnection()->update(ss.str());
	}

    //--
	_fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);
	_fsCutL = context->getDataBaseManager().getFeatureStore(cutlTableName, idName, geomName);

    //--
    _logger->log(epg::log::INFO, "[END] initialization: " + epg::tools::TimeTools::getTime());
};

///
///
///
void app::calcul::GenerateCuttingLinesOp::_generateCutlByCountry(
	std::string countryCode
) const {
	epg::Context *context = epg::ContextS::getInstance();

    // epg parameters
    epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
    std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
	std::string const linkedFeatIdName = context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString();

	// theme paramaters
	params::ThemeParameters * themeParameters = params::ThemeParametersS::getInstance();
	std::string const natIdIdName = themeParameters->getValue(NATIONAL_IDENTIFIER_NAME).toString();

	//--
	GraphType graphArea;
	ign::geometry::graph::tools::SnapRoundPlanarizer< GraphType >  planarizerGraphArea(graphArea, 100);

	std::map<std::string, std::string> mIdNatId;

	ign::feature::FeatureFilter filterCountry(countryCodeName + " = '" + countryCode + "'");
    ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterCountry);
	size_t numArea2load = context->getDataBaseManager().numFeatures(*_fsArea, filterCountry);
	boost::progress_display displayGrapLoad(numArea2load, std::cout, "[ LOADING GRAPH AREA "+ countryCode +" ]\n");
    while (itArea->hasNext()){
		++displayGrapLoad;
        ign::feature::Feature const& fArea = itArea->next();
        ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
        std::string idOrigin = fArea.getId();
		for (size_t i = 0; i < mp.numGeometries(); ++i) {
			planarizerGraphArea.addEdge(mp.polygonN(i).exteriorRing(), idOrigin);
		}
		mIdNatId[idOrigin] = fArea.getAttribute(natIdIdName).toString();
    }
	planarizerGraphArea.planarize();

	//--
	GraphType::edge_iterator eit, eitEnd;
	graphArea.edges(eit, eitEnd);
	boost::progress_display displayGenerateCL(graphArea.numEdges(), std::cout, "[ GENERATE CUTTING LINES " + countryCode + " ]\n");
	while (eit != eitEnd) {
		++displayGenerateCL;

		std::vector<std::string> vCutlOrigins = graphArea.origins(*eit);

		if (vCutlOrigins.size() == 1) {
			++eit;
			continue;
		}

		ign::feature::Feature featCutL = _fsCutL->newFeature();

		ign::geometry::LineString lsCutl = graphArea.getGeometry(*eit);
		lsCutl.setFillZ(0);//temp
		featCutL.setGeometry(lsCutl);

		std::string idLinkedValue;
		for (size_t i = 0; i < vCutlOrigins.size(); ++i) {
			if (i != 0)
				idLinkedValue+="#";
			if(mIdNatId.find(vCutlOrigins[i]) != mIdNatId.end())
				idLinkedValue += mIdNatId.find(vCutlOrigins[i])->second;
		}

		featCutL.setAttribute(linkedFeatIdName, ign::data::String(idLinkedValue));

		featCutL.setAttribute(countryCodeName, ign::data::String(countryCode));

		_fsCutL->createFeature(featCutL);

		++eit;
	}

	//fusion des CutL avec les mêmes linkedFeatIdName et qui se touchent (ou presque)
	ome2::calcul::detail::ClMerger::mergeAll(_fsCutL, ign::feature::FeatureFilter());
} 
