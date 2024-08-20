#ifndef _APP_STEP_ADDSTANDINGWATER_H_
#define _APP_STEP_ADDSTANDINGWATER_H_

#include <epg/step/StepBase.h>
#include <app/params/ThemeParameters.h>

namespace app{
namespace step{

	class AddStandingWater : public epg::step::StepBase< app::params::ThemeParametersS > {

	public:

		/// \brief
		int getCode() { return 301; };

		/// \brief
		std::string getName() { return "AddStandingWater"; };

		/// \brief
		void onCompute( bool );

		/// \brief
		void init();

	};

}
}

#endif