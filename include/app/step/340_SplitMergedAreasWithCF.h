#ifndef _APP_STEP_SPLITMERGEDAREASWITHCF_H_
#define _APP_STEP_SPLITMERGEDAREASWITHCF_H_

#include <epg/step/StepBase.h>
#include <app/params/ThemeParameters.h>

namespace app{
namespace step{

	class SplitMergedAreasWithCF : public epg::step::StepBase< app::params::ThemeParametersS > {

	public:

		/// \brief
		int getCode() { return 340; };

		/// \brief
		std::string getName() { return "SplitMergedAreasWithCF"; };

		/// \brief
		void onCompute( bool );

		/// \brief
		void init();

	};

}
}

#endif