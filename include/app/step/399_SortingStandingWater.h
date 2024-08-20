#ifndef _APP_STEP_SORTINGSTANDINGWATER_H_
#define _APP_STEP_SORTINGSTANDINGWATER_H_

#include <epg/step/StepBase.h>
#include <app/params/ThemeParameters.h>

namespace app{
namespace step{

	class SortingStandingWater : public epg::step::StepBase< app::params::ThemeParametersS > {

	public:

		/// \brief
		int getCode() { return 399; };

		/// \brief
		std::string getName() { return "SortingStandingWater"; };

		/// \brief
		void onCompute( bool );

		/// \brief
		void init();

	};

}
}

#endif