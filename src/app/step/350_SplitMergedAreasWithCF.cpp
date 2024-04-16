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
		addWorkingEntity(AREA_TABLE_INIT);
	}

	///
	///
	///
	void SplitMergedAreasWithCF::onCompute( bool verbose = false )
	{
		//copie table AREA
		_epgParams.setParameter(AREA_TABLE, ign::data::String(getCurrentWorkingTableName(AREA_TABLE_INIT)));
		epg::utils::CopyTableUtils::copyAreaTable(getLastWorkingTableName(AREA_TABLE_INIT), "", false, true, true, false);

		app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
		std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();

		//--
		app::calcul::CfSplitterOp::compute(verbose);
	}

}
}
