#include <app/step/345_GenerateCuttingPoints2.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <ome2/utils/CopyTableUtils.h>

//APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/GenerateCuttingPointsOp.h>


namespace app {
namespace step {

	///
	///
	///
	void GenerateCuttingPoints2::init()
	{
		addWorkingEntity(CUTP_TABLE);
	}

	///
	///
	///
	void GenerateCuttingPoints2::onCompute( bool verbose = false )
	{
		//--
		std::string cutpRefTableName = _themeParams.getValue(CUTP_TABLE).toString();
		std::string countryCodeW = _themeParams.getValue(COUNTRY_CODE_W).toString();

		//--
		ome2::utils::CopyTableUtils::copyPointTable(
			getLastWorkingTableName(CUTP_TABLE),
			getCurrentWorkingTableName(CUTP_TABLE),
			"", false, true
		);

		//--
		_themeParams.setParameter(CUTP_TABLE, ign::data::String(getCurrentWorkingTableName(CUTP_TABLE)));

		//--
		_epgParams.setParameter(AREA_TABLE, ign::data::String(getLastWorkingTableName(AREA_TABLE_INIT)));
		app::calcul::GenerateCuttingPointsOp::Compute(verbose);

		//--
		_themeParams.setParameter(CUTP_TABLE, ign::data::String(cutpRefTableName));
	}

}
}
