#ifndef _APP_STEP_SETMERGEDAREASATTRIBUTES_H_
#define _APP_STEP_SETMERGEDAREASATTRIBUTES_H_

#include <epg/step/StepBase.h>
#include <app/params/ThemeParameters.h>

namespace app{
namespace step{

	class SetMergedAreasAttributes : public epg::step::StepBase< app::params::ThemeParametersS > {

	public:

		/// \brief
		int getCode() { return 360; };

		/// \brief
		std::string getName() { return "SetMergedAreasAttributes"; };

		/// \brief
		void onCompute( bool );

		/// \brief
		void init();

	};

}
}

#endif