#ifndef _APP_STEP_GENERATECUTTINGFEATURES_H_
#define _APP_STEP_GENERATECUTTINGFEATURES_H

#include <epg/step/StepBase.h>
#include <app/params/ThemeParameters.h>

namespace app{
namespace step{

	class GenerateCuttingFeatures : public epg::step::StepBase< app::params::ThemeParametersS > {

	public:

		/// \brief
		int getCode() { return 310; };

		/// \brief
		std::string getName() { return "GenerateCuttingFeatures"; };

		/// \brief
		void onCompute( bool );

		/// \brief
		void init();

	};

}
}

#endif