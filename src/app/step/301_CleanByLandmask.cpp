#include <app/step/301_CleanByLandmask.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/utils/CopyTableUtils.h>


//APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/PolygonSplitterOp.h>
#include <app/calcul/PolygonCleanerOp.h>
#include <app/calcul/PolygonMergerOp.h>

namespace app {
namespace step {

	///
	///
	///
	void CleanByLandmask::init()
	{

	}

	///
	///
	///
	void CleanByLandmask::onCompute( bool verbose = false )
	{
		app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
		std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();

		//--
		// app::calcul::PolygonSplitterOp::compute(countryCodeW, verbose);

		//--
		// app::calcul::PolygonCleanerOp::compute(countryCodeW, verbose);

		//--
		app::calcul::PolygonMergerOp::compute(verbose);
	}

}
}
