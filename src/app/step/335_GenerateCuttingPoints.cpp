#include <app/step/335_GenerateCuttingPoints.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>

//APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/GenerateCuttingPointsOp.h>


namespace app {
namespace step {

	///
	///
	///
	void GenerateCuttingPoints::init()
	{

	}

	///
	///
	///
	void GenerateCuttingPoints::onCompute( bool verbose = false )
	{
		//--
		app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();

		//--
		_epgParams.setParameter(AREA_TABLE, ign::data::String(getLastWorkingTableName(AREA_TABLE_INIT)));
		
		std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();

		app::calcul::GenerateCuttingPointsOp::computeByCountry(countryCodeW, verbose);


		//--
		std::string intAreaTableName = themeParameters->getValue(INTERSECTION_AREA_TABLE).toString();
		if (intAreaTableName == "") {
			std::string const intAreaSuffix = themeParameters->getValue(INTERSECTION_AREA_TABLE_SUFFIX).toString();
			intAreaTableName = themeParameters->getParameter(AREA_TABLE_INIT).getValue().toString() + intAreaSuffix;
		}

		_epgParams.setParameter(AREA_TABLE, ign::data::String(intAreaTableName));

		app::calcul::GenerateCuttingPointsOp::compute(verbose, false);
	}

}
}
