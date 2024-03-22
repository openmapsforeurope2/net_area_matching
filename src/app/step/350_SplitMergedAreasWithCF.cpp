#include <app/step/350_SplitMergedAreasWithCF.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/utils/CopyTableUtils.h>


//APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/CfSplitterOp.h>

namespace app {
namespace step {

	///
	///
	///
	void SplitMergedAreasWithCF::init()
	{

	}

	///
	///
	///
	void SplitMergedAreasWithCF::onCompute( bool verbose = false )
	{
		app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
		std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();

		//--
		app::calcul::CfSplitterOp::compute(verbose);
	}

}
}
