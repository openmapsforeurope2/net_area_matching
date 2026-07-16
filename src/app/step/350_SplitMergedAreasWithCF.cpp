#include <app/step/350_SplitMergedAreasWithCF.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <ome2/utils/CopyTableUtils.h>

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
		ome2::utils::CopyTableUtils::copyAreaTable(getLastWorkingTableName(AREA_TABLE_INIT), "", false, true, true);

		//--
		_themeParams.setParameter(CUTP_TABLE, ign::data::String(getLastWorkingTableName(CUTP_TABLE)));
		_themeParams.setParameter(CUTL_TABLE, ign::data::String(getLastWorkingTableName(CUTL_TABLE)));

		//--
		app::calcul::CfSplitterOp::Compute(verbose);
	}

}
}
