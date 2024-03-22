#ifndef _APP_STEP_CLEANBYLANDMASK_H_
#define _APP_STEP_CLEANBYLANDMASK_H_

#include <epg/step/StepBase.h>
#include <app/params/ThemeParameters.h>

namespace app{
namespace step{

	class CleanByLandmask : public epg::step::StepBase< app::params::ThemeParametersS > {

	public:

		/// \brief
		int getCode() { return 320; };

		/// \brief
		std::string getName() { return "CleanByLandmask"; };

		/// \brief
		void onCompute( bool );

		/// \brief
		void init();

	};

}
}

#endif