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
    std::string countryCode,
    bool verbose
) : 
    _countryCode(countryCode),
    _verbose(verbose)
{
    _init();
}

///
///
///
app::calcul::GenerateCuttingPointsOp::~GenerateCuttingPointsOp()
{
    //_shapeLogger->closeShape("");
}

///
///
///
void app::calcul::GenerateCuttingPointsOp::compute(
	std::string countryCode, 
	bool verbose
) {

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

