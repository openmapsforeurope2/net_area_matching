#ifndef _APP_STEP_MERGESPLITAREAS_H_
#define _APP_STEP_MERGESPLITAREAS_H_

#include <epg/step/StepBase.h>
#include <app/params/ThemeParameters.h>

namespace app{
namespace step{

	class MergedSplitAreas : public epg::step::StepBase< app::params::ThemeParametersS > {

	public:

		/// \brief
		int getCode() { return 370; };

		/// \brief
		std::string getName() { return "MergeSplitAreas"; };

		/// \brief
		void onCompute( bool );

		/// \brief
		void init();

	};

}
}

#endif