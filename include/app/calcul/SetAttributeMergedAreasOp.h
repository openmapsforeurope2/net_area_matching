#ifndef _APP_CALCUL_SETATTRIBUTEMERGEDAREASOP_H_
#define _APP_CALCUL_SETATTRIBUTEMERGEDAREASOP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>
#include <ign/tools/stringtools.h>

//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>


#include <ome2/calcul/utils/attributeMerger.h>


namespace app{
namespace calcul{

	class SetAttributeMergedAreasOp {

	public:

	
        /// @brief 
        /// @param borderCode 
        /// @param verbose 
		SetAttributeMergedAreasOp(
			std::string borderCode,
            bool verbose
        );

        /// @brief 
        ~SetAttributeMergedAreasOp();


		/// \brief
		static void compute(
			std::string borderCode,
			bool verbose
		);


	private:
		//--
		std::string                                              _borderCode;
		//--
		std::vector<std::string>                                 _vCountry;
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsArea;
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsAreaInit;
		//--
		epg::log::EpgLogger*                                     _logger;
		//--
		epg::log::ShapeLogger*                                   _shapeLogger;
		//--
		bool                                                     _verbose;

		ome2::calcul::utils::AttributeMerger*					_attrMergerOnBorder;

	private:

		//--
		void _init();

		//--
		void _compute() ;


		bool _getAreaMergedByCountry(ign::geometry::Polygon& geomAreaMerged, ign::feature::FeatureFilter& filterArroundAreaFromCountry, ign::feature::Feature& fMergedInit);

    };

}
}

#endif