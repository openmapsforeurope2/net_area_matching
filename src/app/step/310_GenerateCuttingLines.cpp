#include <app/step/310_GenerateCuttingLines.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/utils/CopyTableUtils.h>


//APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/GenerateCuttingLinesOp.h>


namespace app {
namespace step {

	///
	///
	///
	void GenerateCuttingLines::init()
	{

	}

	///
	///
	///
	void GenerateCuttingLines::onCompute( bool verbose = false )
	{
		app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
		std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();

		app::calcul::GenerateCuttingLinesOp generateCuttingLinesOp(countryCodeW, verbose);
		generateCuttingLinesOp.generateCutL();
	}

}
}
