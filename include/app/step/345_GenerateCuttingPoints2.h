#ifndef _APP_STEP_GENERATECUTTINGPOINTS2_H_
#define _APP_STEP_GENERATECUTTINGPOINTS2_H_

#include <epg/step/StepBase.h>
#include <app/params/ThemeParameters.h>

namespace app{
namespace step{

	class GenerateCuttingPoints2 : public epg::step::StepBase< app::params::ThemeParametersS > {

	public:

		/// \brief
		int getCode() { return 345; };

		/// \brief
		std::string getName() { return "GenerateCuttingPoints2"; };

		/// \brief
		void onCompute( bool );

		/// \brief
		void init();

	};

}
}

#endif