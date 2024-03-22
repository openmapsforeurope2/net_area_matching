#include <app/step/310_GenerateCuttingFeatures.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/utils/CopyTableUtils.h>


//APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/GenerateCuttingLinesOp.h>
#include <app/calcul/GenerateCuttingPointsOp.h>


namespace app {
namespace step {

	///
	///
	///
	void GenerateCuttingFeatures::init()
	{

	}

	///
	///
	///
	void GenerateCuttingFeatures::onCompute( bool verbose = false )
	{
		epg::Context* context = epg::ContextS::getInstance();
		epg::log::EpgLogger* logger = epg::log::EpgLoggerS::getInstance();
		app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
		std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();
		std::string areaTableName = context->getEpgParameters().getValue(AREA_TABLE).toString();
		std::string const idName = context->getEpgParameters().getValue(ID).toString();
		std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();
		std::string countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();



		app::calcul::GenerateCuttingLinesOp generateCuttingLinesOp(countryCodeW, verbose);
		generateCuttingLinesOp.generateCutL();

		app::calcul::GenerateCuttingPointsOp generateCuttingPointsOp(countryCodeW, verbose);
		generateCuttingPointsOp.generateCutP();

	}

}
}
