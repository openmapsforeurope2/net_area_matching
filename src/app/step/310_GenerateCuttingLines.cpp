#include <app/step/310_GenerateCuttingLines.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <ome2/utils/CopyTableUtils.h>

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
		addWorkingEntity(AREA_TABLE_INIT);
		addWorkingEntity(CUTL_TABLE);
	}

	///
	///
	///
	void GenerateCuttingLines::onCompute( bool verbose = false )
	{
		//--
		std::string countryCodeW = _themeParams.getValue(COUNTRY_CODE_W).toString();
		std::string clRefTableName = _themeParams.getValue(CUTL_TABLE).toString();

		//--
		ome2::utils::CopyTableUtils::copyLineStringTable(
			getLastWorkingTableName(CUTL_TABLE),
			getCurrentWorkingTableName(CUTL_TABLE),
			"", false, true
		);

		//--
		_themeParams.setParameter(CUTL_TABLE, ign::data::String(getCurrentWorkingTableName(CUTL_TABLE)));
		_epgParams.setParameter(AREA_TABLE, ign::data::String(getCurrentWorkingTableName(AREA_TABLE_INIT)));

		//copie table AREA
		ome2::utils::CopyTableUtils::copyAreaTable(getLastWorkingTableName(AREA_TABLE_INIT), "", false, true, true);

		//--
		app::calcul::GenerateCuttingLinesOp::Compute(countryCodeW, verbose);

		//--
		_themeParams.setParameter(CUTL_TABLE, ign::data::String(clRefTableName));
	}

}
}
