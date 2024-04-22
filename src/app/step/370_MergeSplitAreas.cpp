#include <app/step/370_MergeSplitAreas.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>

//APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/SplitAreaMergerOp.h>
#include <app/utils/CopyTableUtils.h>


namespace app {
namespace step {

	///
	///
	///
	void MergeSplitAreas::init()
	{
		addWorkingEntity(AREA_TABLE_INIT);
	}

	///
	///
	///
	void MergeSplitAreas::onCompute( bool verbose = false )
	{
		//copie table AREA
		_epgParams.setParameter(AREA_TABLE, ign::data::String(getCurrentWorkingTableName(AREA_TABLE_INIT)));
		app::utils::CopyTableUtils::copyAreaTable(getLastWorkingTableName(AREA_TABLE_INIT), "", false, true, true);

		app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
		std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();

		//--l
		app::calcul::SplitAreaMergerOp::compute(verbose);
	}

}
}
