#ifndef _APP_STEP_GENERATECUTTINGLINES_H_
#define _APP_STEP_GENERATECUTTINGLINES_H

#include <epg/step/StepBase.h>
#include <app/params/ThemeParameters.h>

namespace app{
namespace step{

	class GenerateCuttingLines : public epg::step::StepBase< app::params::ThemeParametersS > {

	public:

		/// \brief
		int getCode() { return 310; };

		/// \brief
		std::string getName() { return "GenerateCuttingLines"; };

		/// \brief
		void onCompute( bool );

		/// \brief
		void init();

	};

}
}

#endif