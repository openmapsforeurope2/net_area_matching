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
    //_shapeLogger->closeShape("");
}

///
///
///
void app::calcul::GenerateCuttingLinesOp::generateCL(
)
{
	std::vector<std::string> vCountry;
	epg::tools::StringTools::Split(_borderCode, "#", vCountry);

	for (size_t i = 0; i < vCountry.size(); ++i) 
		_computeByCountry(vCountry[i]);
	
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

    // on recupere un buffer autour de la frontiere
   /* ign::geometry::GeometryPtr boundBuffPtr(new ign::geometry::Polygon());
    ign::feature::sql::FeatureStorePostgis* fsBoundary = context->getDataBaseManager().getFeatureStore(boundaryTableName, idName, geomName);
    ign::feature::FeatureIteratorPtr itBoundary = fsBoundary->getFeatures(ign::feature::FeatureFilter(countryCodeName +" = '"+_countryCode+"'"));
    while (itBoundary->hasNext())
    {
        ign::feature::Feature const& fBoundary = itBoundary->next();
        ign::geometry::LineString const& ls = fBoundary.getGeometry().asLineString();

        ign::geometry::GeometryPtr tmpBuffPtr(ls.buffer(100000));

        boundBuffPtr.reset(boundBuffPtr->Union(*tmpBuffPtr));
    }*/

    //on recupere la geometry des pays



	std::string cutlTableName = themeParameters->getValue(CUTL_TABLE).toString();
	if (cutlTableName == "") {
		std::string const cutlTableSuffix = themeParameters->getValue(CUTL_TABLE_SUFFIX).toString();
		cutlTableName = areaTableName + cutlTableSuffix;
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
	//_idGeneratorCutL = epg::sql::tools::IdGeneratorInterfacePtr(epg::sql::tools::IdGeneratorFactory::getNew(*_fsCutL, "CUTTINGLINE"));

    //--
    _logger->log(epg::log::INFO, "[END] initialization: " + epg::tools::TimeTools::getTime());
};

///
///
///
void app::calcul::GenerateCuttingLinesOp::_computeByCountry(
	std::string countryCode
)
{
	epg::Context *context = epg::ContextS::getInstance();
    // epg parameters
    epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
    std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
	std::string const linkedFeatIdName = context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString();

	GraphType graphArea;
	ign::geometry::graph::tools::SnapRoundPlanarizer< GraphType >  planarizerGraphArea(graphArea, 100);

	ign::feature::FeatureFilter filterCountry(countryCodeName + " = '" + countryCode + "'");
    ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterCountry);
	size_t numArea2load = context->getDataBaseManager().numFeatures(*_fsArea, filterCountry);
	boost::progress_display displayGrapLoad(numArea2load, std::cout, "[ LOADING GRAPH AREA "+ countryCode +" ]\n");
    while (itArea->hasNext())
    {
		++displayGrapLoad;
        ign::feature::Feature const& fArea = itArea->next();
        ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
        std::string idOrigin = fArea.getId();
		for (size_t i = 0; i < mp.numGeometries(); ++i) {
			planarizerGraphArea.addEdge(mp.polygonN(i).exteriorRing(), idOrigin);
		}        
    }
	planarizerGraphArea.planarize();
	graphArea.createFaces();

	GraphType::edge_iterator eit, eitEnd;
	graphArea.edges(eit, eitEnd);
	boost::progress_display displayGenerateCL(graphArea.numEdges(), std::cout, "[ GENERATE CUTTING LINES " + countryCode + " ]\n");
	while (eit != eitEnd) {
		++displayGenerateCL;
		bool hasLeftFace = graphArea.leftFace(*eit).first;
		bool hasRightFace =  graphArea.rightFace(*eit).first;

		if ( ! hasLeftFace || ! hasRightFace ) {
			++eit;
			continue;
		}

		std::vector<std::string> vCutlOrigins = graphArea.origins(*eit);
		ign::geometry::LineString lsCutl = graphArea.getGeometry(*eit);
		++eit;

		ign::feature::Feature featCutL = _fsCutL->newFeature();
		lsCutl.setFillZ(0);//temp
		featCutL.setGeometry(lsCutl);
		//std::string idCutL = _idGeneratorCutL->next();
		//featCutL.setId(idCutL);
		std::string idLinkedValue;
		for (size_t i = 0; i < vCutlOrigins.size(); ++i) {
			if (i != 0)
				idLinkedValue+="#";
			idLinkedValue += vCutlOrigins[i];
		}
		featCutL.setAttribute(countryCodeName, ign::data::String(countryCode));
		featCutL.setAttribute(linkedFeatIdName, ign::data::String(idLinkedValue));
		_fsCutL->createFeature(featCutL);
	}

	//fusion des CutL avec les mêmes linkedFeatIdName et qui se touchent (ou presque)
	/*std::ostringstream ss1;
	ss1 << linkedFeatIdName << " LIKE '%#%'";
	ign::feature::FeatureFilter mergeFilter(ss1.str());*/
	_mergeAllCutl(ign::feature::FeatureFilter());

} 

///
///
///
void app::calcul::GenerateCuttingLinesOp::_mergeAllCutl(
	ign::feature::FeatureFilter const& filter
) const {
	epg::Context* context = epg::ContextS::getInstance();
	epg::params::EpgParameters const& epgParams = context->getEpgParameters();
	std::string const linkedFeatureIdName = epgParams.getValue(LINKED_FEATURE_ID).toString();

	std::set<std::string> sMergedCl;
	ign::feature::FeatureIteratorPtr itCutl = _fsCutL->getFeatures(filter);
	while (itCutl->hasNext())
	{
		ign::feature::Feature fCutl = itCutl->next();
		ign::geometry::LineString const& cutlGeom = fCutl.getGeometry().asLineString();
		std::string linkedFeatureId = fCutl.getAttribute(linkedFeatureIdName).toString();

		if (sMergedCl.find(fCutl.getId()) != sMergedCl.end()) continue;

		std::set<std::string> sMergedCl2;
		ign::geometry::LineString mergedClGeom = _mergeCutl(fCutl, linkedFeatureId, sMergedCl2);

		if (sMergedCl2.size() < 2) continue;

		sMergedCl.insert(sMergedCl2.begin(), sMergedCl2.end());

		fCutl.setGeometry(mergedClGeom);
		_fsCutL->createFeature(fCutl );

	}
	for (std::set<std::string>::const_iterator sit = sMergedCl.begin(); sit != sMergedCl.end(); ++sit) {
		_fsCutL->deleteFeature(*sit);
	}

}


///
///
///
ign::geometry::LineString app::calcul::GenerateCuttingLinesOp::_mergeCutl(
	ign::feature::Feature const& refClFeat,
	std::string const& linkedFeatureId,
	std::set<std::string> & sMergedCl
) const {
	epg::Context* context = epg::ContextS::getInstance();
	epg::params::EpgParameters const& epgParams = context->getEpgParameters();
	std::string const linkedFeatureIdName = epgParams.getValue(LINKED_FEATURE_ID).toString();

	ign::geometry::LineString const& refClGeom = refClFeat.getGeometry().asLineString();
	ign::geometry::Point startPoint = refClGeom.startPoint();
	ign::geometry::Point endPoint = refClGeom.endPoint();
	bool bNewStart = true;
	bool bNewEnd = true;

	std::vector<ign::feature::Feature> vCandidates;
	ign::feature::FeatureIteratorPtr itCl = _fsCutL->getFeatures(linkedFeatureIdName + " LIKE '%" + linkedFeatureId + "%'");
	while (itCl->hasNext())
	{
		ign::feature::Feature const& fCl = itCl->next();
		vCandidates.push_back(fCl);
	}

	std::vector<ign::geometry::LineString> vGeom2Merge;
	vGeom2Merge.push_back(refClGeom);
	sMergedCl.insert(refClFeat.getId());

	do {
		std::vector<ign::feature::Feature>::const_iterator vit;
		size_t before = sMergedCl.size();
		for (vit = vCandidates.begin(); vit != vCandidates.end(); ++vit) {
			if (sMergedCl.find(vit->getId()) != sMergedCl.end()) continue;

			ign::geometry::LineString const& clGeom = vit->getGeometry().asLineString();

			//DEBUG
			double bTouchingEnd_d = startPoint.distance(clGeom.endPoint());
			double bTouchingStart_d = startPoint.distance(clGeom.startPoint());

			bool bTouchingEnd = startPoint.distance(clGeom.endPoint()) < 1e-1;
			bool bTouchingStart = startPoint.distance(clGeom.startPoint()) < 1e-1;
			if (bTouchingEnd || bTouchingStart) {
				sMergedCl.insert(vit->getId());
				vGeom2Merge.push_back(clGeom);
				if (bTouchingEnd) vGeom2Merge.back().endPoint() = startPoint;
				if (bTouchingStart) vGeom2Merge.back().startPoint() = startPoint;
				startPoint = bTouchingEnd ? clGeom.startPoint() : clGeom.endPoint();
				break;
			}
		}
		if (before == sMergedCl.size()) bNewStart = false;
	} while (bNewStart);

	do {
		std::vector<ign::feature::Feature>::const_iterator vit;
		size_t before = sMergedCl.size();
		for (vit = vCandidates.begin(); vit != vCandidates.end(); ++vit) {
			if (sMergedCl.find(vit->getId()) != sMergedCl.end()) continue;

			ign::geometry::LineString const& clGeom = vit->getGeometry().asLineString();
			bool bTouchingEnd = endPoint.distance(clGeom.endPoint()) < 1e-5;
			bool bTouchingStart = endPoint.distance(clGeom.startPoint()) < 1e-5;
			if (bTouchingEnd || bTouchingStart) {
				sMergedCl.insert(vit->getId());
				vGeom2Merge.push_back(clGeom);
				if (bTouchingEnd) vGeom2Merge.back().endPoint() = endPoint;
				if (bTouchingStart) vGeom2Merge.back().startPoint() = endPoint;
				endPoint = bTouchingEnd ? clGeom.startPoint() : clGeom.endPoint();
				break;
			}
		}
		if (before == sMergedCl.size()) bNewEnd = false;
	} while (bNewEnd);

	if (vGeom2Merge.size() == 1) return vGeom2Merge.front();

	std::vector<ign::geometry::LineString> vMergedGeom = ign::geometry::algorithm::LineMergerOpGeos::MergeLineStrings(vGeom2Merge);
	if (vMergedGeom.size() > 1) _logger->log(epg::log::WARN, "Merging adjacent CL gives a MultilineString [ref CL id] " + refClFeat.getId());

	return vMergedGeom.front();
};