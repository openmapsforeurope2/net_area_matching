#include <app/step/330_CleanCuttingLines.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>


//APP
#include <app/calcul/CuttingLineCleanerOp.h>
#include <app/utils/CopyTableUtils.h>

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
		app::calcul::CuttingLineCleanerOp::compute(verbose);
	}

}
}
