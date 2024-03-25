#ifndef _APP_STEP_CLEANCUTTINGLINES_H_
#define _APP_STEP_CLEANCUTTINGLINES_H_

#include <epg/step/StepBase.h>
#include <app/params/ThemeParameters.h>

namespace app{
namespace step{

	class CleanCuttingLines : public epg::step::StepBase< app::params::ThemeParametersS > {

	public:

		/// \brief
		int getCode() { return 330; };

		/// \brief
		std::string getName() { return "CleanCuttingLines"; };

		/// \brief
		void onCompute( bool );

		/// \brief
		void init();

	};

}
}

#endif