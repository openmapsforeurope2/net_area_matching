#ifndef _APP_STEP_GENERATECUTTINGPOINTS_H_
#define _APP_STEP_GENERATECUTTINGPOINTS_H_

#include <epg/step/StepBase.h>
#include <app/params/ThemeParameters.h>

namespace app{
namespace step{

	class GenerateCuttingPoints : public epg::step::StepBase< app::params::ThemeParametersS > {

	public:

		/// \brief
		int getCode() { return 335; };

		/// \brief
		std::string getName() { return "GenerateCuttingPoints"; };

		/// \brief
		void onCompute( bool );

		/// \brief
		void init();

	};

}
}

#endif