#include <app/step/330_CleanCuttingLines.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <ome2/utils/CopyTableUtils.h>

//APP
#include <app/calcul/CuttingLineCleanerOp.h>


namespace app {
namespace step {

	///
	///
	///
	void CleanCuttingLines::init()
	{
		addWorkingEntity(CUTL_TABLE);
	}

	///
	///
	///
	void CleanCuttingLines::onCompute( bool verbose = false )
	{
		//--
		std::string clRefTableName = _themeParams.getValue(CUTL_TABLE).toString();

		//--
		ome2::utils::CopyTableUtils::copyLineStringTable(
			getLastWorkingTableName(CUTL_TABLE),
			getCurrentWorkingTableName(CUTL_TABLE),
			"", false, true
		);

		//--
		_themeParams.setParameter(CUTL_TABLE, ign::data::String(getCurrentWorkingTableName(CUTL_TABLE)));
		_epgParams.setParameter(AREA_TABLE, ign::data::String(getLastWorkingTableName(AREA_TABLE_INIT)));
		
		//--
		app::calcul::CuttingLineCleanerOp::Compute(verbose);

		//--
		_themeParams.setParameter(CUTL_TABLE, ign::data::String(clRefTableName));
	}

}
}
