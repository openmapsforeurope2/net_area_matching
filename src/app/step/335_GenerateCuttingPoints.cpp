#include <app/step/335_GenerateCuttingPoints.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/utils/CopyTableUtils.h>


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
		app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
		std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();

		app::calcul::GenerateCuttingPointsOp generateCuttingPointsOp(countryCodeW, verbose);
		generateCuttingPointsOp.generateCutP();
	}

}
}
