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
#include <ome2/geometry/tools/getEndingPoints.h>


///
///
///
app::calcul::GenerateCuttingPointsOp::GenerateCuttingPointsOp(
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
app::calcul::GenerateCuttingPointsOp::~GenerateCuttingPointsOp()
{

}

///
///
///
void app::calcul::GenerateCuttingPointsOp::generateCutP() 
{
	std::vector<std::string> vCountry;
	epg::tools::StringTools::Split(_borderCode, "#", vCountry);

	for (size_t i = 0; i < vCountry.size(); ++i)
		_generateCutpByCountry(vCountry[i]);
}

///
///
///
void app::calcul::GenerateCuttingPointsOp::_init()
{
	epg::Context* context = epg::ContextS::getInstance();
	_logger = epg::log::EpgLoggerS::getInstance();
	app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
	std::string const countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();
	std::string const areaTableName = context->getEpgParameters().getValue(AREA_TABLE).toString();
	std::string const idName = context->getEpgParameters().getValue(ID).toString();
	std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();
	std::string const countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();
	std::string const linkedFeatIdName = context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString();

	std::string cutpTableName = themeParameters->getValue(CUTP_TABLE).toString();
	if (cutpTableName == "") {
		std::string const cpTableSuffix = themeParameters->getValue(CUTP_TABLE_SUFFIX).toString();
		cutpTableName = areaTableName + cpTableSuffix;
	}
	{
		std::ostringstream ss;
		ss << "DROP TABLE IF EXISTS " << cutpTableName << " ;";
		ss << "CREATE TABLE " << cutpTableName << "("
			<< idName << " uuid DEFAULT gen_random_uuid(),"
			<< geomName << " geometry(PointZ),"
			<< countryCodeName << " character varying(8),"
			<< linkedFeatIdName << " character varying(255) "
			<< ");";
		//ajout d'index?

		context->getDataBaseManager().getConnection()->update(ss.str());
	}

	//--
	_fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);
	_fsCutP = context->getDataBaseManager().getFeatureStore(cutpTableName, idName, geomName);

	std::string cutlTableName = themeParameters->getValue(CUTL_TABLE).toString();
	if (cutlTableName == "") {
		std::string const cutlTableSuffix = themeParameters->getValue(CUTL_TABLE_SUFFIX).toString();
		cutlTableName = areaTableName + cutlTableSuffix;
	}
	_fsCutL = context->getDataBaseManager().getFeatureStore(cutlTableName, idName, geomName);

};

void app::calcul::GenerateCuttingPointsOp::_generateCutpByCountry(
	std::string countryCode
)
{
	epg::Context *context = epg::ContextS::getInstance();
	epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
	app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
	std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
	std::string const linkedFeatIdName = context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString();
	double const distSnapMergeCf = themeParameters->getValue(DIST_SNAP_MERGE_CF).toDouble();

	ign::feature::FeatureFilter filterCountry(countryCodeName + " = '" + countryCode + "'");
	ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterCountry);
	size_t numArea2load = context->getDataBaseManager().numFeatures(*_fsArea, filterCountry);
	boost::progress_display displayGrapLoad(numArea2load, std::cout, "[ GENERATE CUTTING POINTS " + countryCode + " ]\n");
	while (itArea->hasNext()) {
		++displayGrapLoad;
		ign::feature::Feature const& fArea = itArea->next();
		ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
		std::string idOrigin = fArea.getId();
		for (size_t i = 0; i < mp.numGeometries(); ++i) {
			std::pair<ign::geometry::Point, ign::geometry::Point> pEndingPointsSurf = ome2::geometry::tools::getEndingVerticesOnExtRing(mp.polygonN(i));
			std::vector<ign::geometry::Point> vEndingPt;
			vEndingPt.push_back(pEndingPointsSurf.first);
			vEndingPt.push_back(pEndingPointsSurf.second);
			for(std::vector<ign::geometry::Point>::iterator vit = vEndingPt.begin(); vit != vEndingPt.end(); ++vit) {
				ign::feature::FeatureCollection fCollection;
				ign::geometry::Envelope bboxCutP(vit->getEnvelope());
				bboxCutP.expandBy(distSnapMergeCf);
				_fsCutL->getFeaturesByExtent(bboxCutP, fCollection);
				if (fCollection.size() != 0)
					continue;

				ign::feature::Feature featCutP = _fsCutP->newFeature();
				vit->setZ(0);//temp
				featCutP.setGeometry(*vit);
				featCutP.setAttribute(countryCodeName, ign::data::String(countryCode));
				featCutP.setAttribute(linkedFeatIdName, ign::data::String(idOrigin));
				_fsCutP->createFeature(featCutP);
			}
		}
	}

}