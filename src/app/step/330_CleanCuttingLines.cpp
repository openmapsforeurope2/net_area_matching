#include <app/step/330_CleanCuttingLines.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>

//APP
#include <app/calcul/CuttingLineCleanerOp.h>


namespace app {
namespace step {

	///
	///
	///
	void CleanCuttingLines::init()
	{

	}

	///
	///
	///
	void CleanCuttingLines::onCompute( bool verbose = false )
	{
		//--
		_epgParams.setParameter(AREA_TABLE, ign::data::String(getLastWorkingTableName(AREA_TABLE_INIT)));
		//--
		app::calcul::CuttingLineCleanerOp::compute(verbose);
	}

}
}
