#include <app/step/330_MergeAreas.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/utils/CopyTableUtils.h>


//APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/IntersectingAreasMergerOp.h>

namespace app {
namespace step {

	///
	///
	///
	void MergeAreas::init()
	{

	}

	///
	///
	///
	void MergeAreas::onCompute( bool verbose = false )
	{
		app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
		std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();

		//--
		app::calcul::IntersectingAreasMergerOp::compute(countryCodeW, verbose);
	}

}
}
