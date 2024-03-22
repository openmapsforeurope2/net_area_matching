#ifndef _APP_STEP_CLEANCUTTINGFEATURES_H_
#define _APP_STEP_CLEANCUTTINGFEATURES_H_

#include <epg/step/StepBase.h>
#include <app/params/ThemeParameters.h>

namespace app{
namespace step{

	class CleanCuttingFeatures : public epg::step::StepBase< app::params::ThemeParametersS > {

	public:

		/// \brief
		int getCode() { return 330; };

		/// \brief
		std::string getName() { return "CleanCuttingFeatures"; };

		/// \brief
		void onCompute( bool );

		/// \brief
		void init();

	};

}
}

#endif