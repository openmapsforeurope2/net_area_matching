#include <app/step/370_MergeSplitAreas.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <ome2/utils/CopyTableUtils.h>

//APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/SplitAreaMergerOp.h>


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
		ome2::utils::CopyTableUtils::copyAreaTable(getLastWorkingTableName(AREA_TABLE_INIT), "", false, true, true);

		std::string countryCodeW = _themeParams.getValue(COUNTRY_CODE_W).toString();

		//--
		app::calcul::SplitAreaMergerOp::Compute(verbose);
	}

}
}
