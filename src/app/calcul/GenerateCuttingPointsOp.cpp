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
	std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();
	std::string areaTableName = context->getEpgParameters().getValue(AREA_TABLE).toString();
	std::string const idName = context->getEpgParameters().getValue(ID).toString();
	std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();
	std::string countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();
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
			<< geomName << " geometry(LineStringZ),"
			<< countryCodeName << " character varying(8),"
			<< linkedFeatIdName << " character varying(255) "
			<< ");";
		//ajout d'index?

		context->getDataBaseManager().getConnection()->update(ss.str());
	}

	//--
	_fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);
	_fsCutP = context->getDataBaseManager().getFeatureStore(cutpTableName, idName, geomName);
};

void app::calcul::GenerateCuttingPointsOp::_generateCutpByCountry(
	std::string countryCode
)
{
	epg::Context *context = epg::ContextS::getInstance();
	// epg parameters
	epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
	std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
	std::string const linkedFeatIdName = context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString();

	ign::feature::FeatureFilter filterCountry(countryCodeName + " = '" + countryCode + "'");
	ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterCountry);
	size_t numArea2load = context->getDataBaseManager().numFeatures(*_fsArea, filterCountry);
	boost::progress_display displayGrapLoad(numArea2load, std::cout, "[ GENERATE CUTTING POINTS " + countryCode + " ]\n");
	while (itArea->hasNext()) {
		++displayGrapLoad;
		ign::feature::Feature const& fArea = itArea->next();
		ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
		std::string idOrigin = fArea.getId();
	}

}