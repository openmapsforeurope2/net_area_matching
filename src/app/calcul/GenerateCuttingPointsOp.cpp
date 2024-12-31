// APP
#include <app/calcul/GenerateCuttingPointsOp.h>
#include <app/params/ThemeParameters.h>

// BOOST
#include <boost/progress.hpp>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <epg/sql/tools/numFeatures.h>
#include <epg/sql/DataBaseManager.h>
#include <epg/tools/StringTools.h>
#include <epg/tools/TimeTools.h>
#include <epg/tools/FilterTools.h>

//OME2
#include <ome2/geometry/tools/GetEndingPointsOp.h>
#include <ome2/geometry/tools/lineStringTools.h>


///
///
///
app::calcul::GenerateCuttingPointsOp::GenerateCuttingPointsOp(
    std::string const& borderCode,
    bool verbose,
	bool resetCpTable
) : 
	_borderCode(borderCode),
    _verbose(verbose)
{
    _init(resetCpTable);
}

///
///
///
app::calcul::GenerateCuttingPointsOp::~GenerateCuttingPointsOp()
{
	_shapeLogger->closeShape("gcp_medial_axis");
	_shapeLogger->closeShape("gcp_sections");
}

///
///
///
void app::calcul::GenerateCuttingPointsOp::computeByCountry(
	std::string const& borderCode, 
	bool verbose,
	bool resetCpTable
) {
	GenerateCuttingPointsOp op(borderCode, verbose, resetCpTable);
	op._computeByCountry();
}

///
///
///
void app::calcul::GenerateCuttingPointsOp::compute(
	bool verbose,
	bool resetCpTable
) {
	GenerateCuttingPointsOp op("", verbose, resetCpTable);
	op._generateCutp();
}

///
///
///
void app::calcul::GenerateCuttingPointsOp::_computeByCountry() const
{
	epg::Context* context = epg::ContextS::getInstance();
	std::string const countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();

	std::vector<std::string> vCountry;
	epg::tools::StringTools::Split(_borderCode, "#", vCountry);

	for (size_t i = 0; i < vCountry.size(); ++i) {
		ign::feature::FeatureFilter filter(countryCodeName + " = '" + vCountry[i] + "'");
		_generateCutp(filter);
	}	
}

///
///
///
void app::calcul::GenerateCuttingPointsOp::_init(bool resetCpTable)
{
	epg::Context* context = epg::ContextS::getInstance();

	//--
	_logger = epg::log::EpgLoggerS::getInstance();

	//--
	_shapeLogger = epg::log::ShapeLoggerS::getInstance();
	_shapeLogger->addShape("gcp_medial_axis", epg::log::ShapeLogger::LINESTRING);
	_shapeLogger->addShape("gcp_sections", epg::log::ShapeLogger::LINESTRING);
	_shapeLogger->addShape("gcp_medial_axis_vectors", epg::log::ShapeLogger::LINESTRING);
	_shapeLogger->addShape("gcp_vectors", epg::log::ShapeLogger::LINESTRING);

	//--
	std::string const areaTableName = context->getEpgParameters().getValue(AREA_TABLE).toString();
	std::string const idName = context->getEpgParameters().getValue(ID).toString();
	std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();
	std::string const countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();
	std::string const linkedFeatIdName = context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString();

	//--
	app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
	std::string const sectionGeomName = themeParameters->getParameter(CUTP_SECTION_GEOM).getValue().toString();
	std::string cutpTableName = themeParameters->getValue(CUTP_TABLE).toString();
	if (cutpTableName == "") {
		std::string const cpTableSuffix = themeParameters->getValue(CUTP_TABLE_SUFFIX).toString();
		cutpTableName = themeParameters->getParameter(AREA_TABLE_INIT).getValue().toString() + cpTableSuffix;
	}
	std::string cutlTableName = themeParameters->getValue(CUTL_TABLE).toString();
	if (cutlTableName == "") {
		std::string const cutlTableSuffix = themeParameters->getValue(CUTL_TABLE_SUFFIX).toString();
		cutlTableName = themeParameters->getParameter(AREA_TABLE_INIT).getValue().toString() + cutlTableSuffix;
	}
	
	if (resetCpTable || !context->getDataBaseManager().tableExists(cutpTableName)) {
		std::ostringstream ss;
		ss << "DROP TABLE IF EXISTS " << cutpTableName << " ;";
		ss << "CREATE TABLE " << cutpTableName << "("
			<< idName << " uuid DEFAULT gen_random_uuid(),"
			<< geomName << " geometry(PointZ),"
			<< sectionGeomName << " geometry(LineString),"
			<< countryCodeName << " character varying(8),"
			<< linkedFeatIdName << " character varying(255) "
			<< ");";
		//ajout d'index?

		context->getDataBaseManager().getConnection()->update(ss.str());
	}

	//--
	_fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);

	//--
	_fsCutP = context->getDataBaseManager().getFeatureStore(cutpTableName, idName, geomName);

	//--
	_fsCutL = context->getDataBaseManager().getFeatureStore(cutlTableName, idName, geomName);

};

///
///
///
void app::calcul::GenerateCuttingPointsOp::_generateCutp(
	ign::feature::FeatureFilter filter
) const {
	//--
	epg::Context *context = epg::ContextS::getInstance();
	//--
	epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
	std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
	std::string const linkedFeatIdName = epgParams.getValue(LINKED_FEATURE_ID).toString();	
	//--
	app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
	double const distSnapMergeCf = themeParameters->getValue(DIST_SNAP_MERGE_CF).toDouble();
	std::string const sectionGeomName = themeParameters->getValue(CUTP_SECTION_GEOM).toString();
	std::string const natIdIdName = themeParameters->getValue(NATIONAL_IDENTIFIER_NAME).toString();

	// a parametrer
	double sectionWidth = 100;

	ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filter);
	size_t numArea2load = context->getDataBaseManager().numFeatures(*_fsArea, filter);
	boost::progress_display display(numArea2load, std::cout, "[ GENERATE CUTTING POINTS ]\n");
	while (itArea->hasNext()) {
		++display;
		ign::feature::Feature const& fArea = itArea->next();
		ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
		std::string const idOrigin = fArea.getId();
		std::string const linkedNatId = fArea.getAttribute(natIdIdName).toString();
		std::string const countryCode = fArea.getAttribute(countryCodeName).toString();

		//--
		std::vector<std::string> vLkid;
		epg::tools::StringTools::Split(linkedNatId, "#", vLkid);
		std::string sqlFilter = linkedFeatIdName + " LIKE '%" + vLkid.front() + "%'";
		if(vLkid.size() > 1) sqlFilter += " || "+ linkedFeatIdName + " LIKE '%" + vLkid.back() + "%'";

		for (size_t i = 0; i < mp.numGeometries(); ++i) {
			ign::geometry::Polygon const& poly = mp.polygonN(i);

			// if (poly.distance(ign::geometry::Point(3835893.154,3097818.904)) < 0.2) {
			// 	bool test = true;
			// }
			std::vector<std::pair<ign::geometry::Point, ign::math::Vec2d>> vpEndingPtVector = _getEndingVectors(poly);

			for(size_t i = 0 ; i < vpEndingPtVector.size() ; ++i ) {

				ign::feature::FeatureFilter filterArroundEndPt(sqlFilter);
				ign::geometry::Envelope bboxPt(vpEndingPtVector[i].first.getEnvelope());
				bboxPt.expandBy(distSnapMergeCf);
				filterArroundEndPt.setExtent(bboxPt);
				if (_hasCutLArroundEndingPt(filterArroundEndPt, vpEndingPtVector[i].first, poly))
					continue;
						
				ign::math::Vec2d vOrtho(-vpEndingPtVector[i].second.y(), vpEndingPtVector[i].second.x());

				ign::geometry::Point sectionPt1( vpEndingPtVector[i].first.x() + (sectionWidth/2)*vOrtho.x(), vpEndingPtVector[i].first.y() + (sectionWidth/2)*vOrtho.y());
				ign::geometry::Point sectionPt2( vpEndingPtVector[i].first.x() - (sectionWidth/2)*vOrtho.x(), vpEndingPtVector[i].first.y() - (sectionWidth/2)*vOrtho.y());


				ign::feature::Feature featCutP = _fsCutP->newFeature();
				ign::geometry::LineString sectionGeom(sectionPt1, sectionPt2);

				ign::feature::Feature featSection;
				featSection.setGeometry(sectionGeom);
				_shapeLogger->writeFeature("gcp_sections", featSection);

				// sectionGeom.setFillZ(0);
				featCutP.setGeometry(vpEndingPtVector[i].first);
				featCutP.setAttribute(countryCodeName, ign::data::String(countryCode));
				featCutP.setAttribute(linkedFeatIdName, ign::data::String(linkedNatId));
				featCutP.setAttribute(sectionGeomName, sectionGeom);
				_fsCutP->createFeature(featCutP);

			}
		}
	}
}

///
///
///
std::vector<std::pair<ign::geometry::Point, ign::math::Vec2d>> app::calcul::GenerateCuttingPointsOp::_getEndingVectors(
	ign::geometry::Polygon const& poly
) const {
	double angleThreshold = 0.349; // 20 degres
	double xFactor = 3;
	double douglasPeuckerDist = 15;

	ign::geometry::LineString medialAxis = ome2::geometry::tools::GetEndingPointsOp::computeMedialAxis(poly, douglasPeuckerDist).second;

	//DEBUG
	// if(medialAxis.startPoint().distance(ign::geometry::Point(3835880.79999,3098307.79995))<1e-1) {
	// 	bool test = true;
	// }
	// if(medialAxis.endPoint().distance(ign::geometry::Point(3835880.79999,3098307.79995))<1e-1) {
	// 	bool test = true;
	// }

	ign::feature::Feature featMedialAxis;
	featMedialAxis.setGeometry(medialAxis);
	_shapeLogger->writeFeature("gcp_medial_axis", featMedialAxis);

	ign::math::Vec2d vStart;
	ign::math::Vec2d vEnd;

	if (medialAxis.numPoints() < 4) {
		vStart = medialAxis.startPoint().toVec2d() - medialAxis.pointN(1).toVec2d();
		vEnd = medialAxis.endPoint().toVec2d() - medialAxis.pointN(medialAxis.numPoints()-2).toVec2d();
	} else {

		int id1 = ome2::geometry::tools::getIndex(medialAxis.startPoint(), poly.exteriorRing());
		int id2 = ome2::geometry::tools::getIndex(medialAxis.endPoint(), poly.exteriorRing());
		
		//--
		std::pair<bool, std::pair<ign::geometry::LineString,ign::geometry::LineString>> pSides = ome2::geometry::tools::getSubLineStrings(id1, id2, poly.exteriorRing());

		if (!pSides.first) {
			_logger->log(epg::log::ERROR, "Error in sub-linestrings calculation : "+poly.toString());
			vStart = medialAxis.startPoint().toVec2d() - medialAxis.pointN(1).toVec2d();
			vEnd = medialAxis.endPoint().toVec2d() - medialAxis.pointN(medialAxis.numPoints()-2).toVec2d();
		} else {
			//--
			ign::math::Vec2d vStart1 = _getLsEndingVector(pSides.second.first, medialAxis.startPoint());
			ign::math::Vec2d vStart2 = _getLsEndingVector(pSides.second.second, medialAxis.startPoint());

			//--
			ign::math::Vec2d vEnd1 = _getLsEndingVector(pSides.second.first, medialAxis.endPoint());
			ign::math::Vec2d vEnd2 = _getLsEndingVector(pSides.second.second, medialAxis.endPoint());

			//--
			ign::math::Vec2d vStartBis = medialAxis.startPoint().toVec2d() - medialAxis.pointN(1).toVec2d();
			ign::math::Vec2d vEndBis = medialAxis.endPoint().toVec2d() - medialAxis.pointN(medialAxis.numPoints()-2).toVec2d();

			//--
			double angleStartBis1 = _getAngle(vStartBis, vStart1);
			double angleStartBis2 = _getAngle(vStartBis, vStart2);

			double angleEndBis1 = _getAngle(vEndBis, vEnd1);
			double angleEndBis2 = _getAngle(vEndBis, vEnd2);

			vStart = (angleStartBis1 < angleThreshold || angleStartBis2 < angleThreshold) ? vStartBis : medialAxis.startPoint().toVec2d() - medialAxis.pointN(2).toVec2d();
			vEnd = (angleEndBis1 < angleThreshold || angleEndBis2 < angleThreshold) ? vEndBis : medialAxis.endPoint().toVec2d() - medialAxis.pointN(medialAxis.numPoints()-3).toVec2d();

			//--
			ign::feature::Feature featMedialAxisVector;
			featMedialAxisVector.setGeometry(ign::geometry::LineString(medialAxis.startPoint(), medialAxis.pointN(2)));
			_shapeLogger->writeFeature("gcp_medial_axis_vectors", featMedialAxisVector);
			featMedialAxisVector.setGeometry(ign::geometry::LineString(medialAxis.endPoint(), medialAxis.pointN(medialAxis.numPoints()-3)));
			_shapeLogger->writeFeature("gcp_medial_axis_vectors", featMedialAxisVector);

			double angleStart1 = _getAngle(vStart, vStart1);
			double angleStart2 = _getAngle(vStart, vStart2);

			if( angleStart2 > xFactor*angleStart1 ) vStart = vStart1;
			else if( angleStart1 > xFactor*angleStart2 ) vStart = vStart2;
			
			double angleEnd1 = _getAngle(vEnd, vEnd1);
			double angleEnd2 = _getAngle(vEnd, vEnd2);

			if( angleEnd2 > xFactor*angleEnd1 ) vEnd = vEnd1;
			else if( angleEnd1 > xFactor*angleEnd2 ) vEnd = vEnd2;
		}
	}
	vStart.normalize();
	vEnd.normalize();

	std::vector<std::pair<ign::geometry::Point, ign::math::Vec2d>> vResult;
	vResult.push_back(std::make_pair(medialAxis.startPoint(), vStart));
	vResult.push_back(std::make_pair(medialAxis.endPoint(), vEnd));

	ign::feature::Feature featVector;
	ign::math::Vec2d vFront = vResult.front().first.toVec2d()-vResult.front().second;
	featVector.setGeometry(ign::geometry::LineString(vResult.front().first, ign::geometry::Point(vFront.x(), vFront.y())));
	_shapeLogger->writeFeature("gcp_vectors", featVector);

	ign::math::Vec2d vBack = vResult.back().first.toVec2d()-vResult.back().second;
	featVector.setGeometry(ign::geometry::LineString(vResult.back().first, ign::geometry::Point(vBack.x(), vBack.y())));
	_shapeLogger->writeFeature("gcp_vectors", featVector);

	return vResult;
}

///
///
///
ign::math::Vec2d app::calcul::GenerateCuttingPointsOp::_getLsEndingVector(
	ign::geometry::LineString const& ls,
	ign::geometry::Point const& endingPoint
) const {
	if (ls.startPoint().distance(endingPoint) < ls.endPoint().distance(endingPoint))
		return ls.startPoint().toVec2d() - ls.pointN(1).toVec2d();
	return ls.endPoint().toVec2d() - ls.pointN(ls.numPoints()-2).toVec2d();
}

///
///
///
double app::calcul::GenerateCuttingPointsOp::_getAngle(
	ign::math::Vec2d const& vRef,
	ign::math::Vec2d const& v
) const {
	return acos( ( v.x()*vRef.x()+vRef.y()*v.y())/(sqrt(vRef.x()*vRef.x()+vRef.y()*vRef.y())*sqrt(v.x()*v.x()+v.y()*v.y()) ) );
}


bool app::calcul::GenerateCuttingPointsOp::_hasCutLArroundEndingPt(
	ign::feature::FeatureFilter const& filterArroundEndPt,
	ign::geometry::Point const& ptEndPt,
	ign::geometry::Polygon const& polyArea
) const {
	app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
	double const distSnapMergeCf = themeParameters->getValue(DIST_SNAP_MERGE_CF).toDouble();

	ign::feature::FeatureIteratorPtr itCutLArroundEndPt = _fsCutL->getFeatures(filterArroundEndPt);
	bool hasCutLArround = false;
	while (itCutLArroundEndPt->hasNext()) {
		ign::feature::Feature fCutLArroundEndPt = itCutLArroundEndPt->next();
		if (fCutLArroundEndPt.getGeometry().distance(ptEndPt) < distSnapMergeCf && fCutLArroundEndPt.getGeometry().intersects(polyArea) )
			return true;	
	}
	return false;
}