#ifndef _APP_CALCUL_SETATTRIBUTEMERGEDAREASOP_H_
#define _APP_CALCUL_SETATTRIBUTEMERGEDAREASOP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>
#include <ign/tools/stringtools.h>

//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>


#include <ome2/calcul/utils/AttributeMerger.h>


namespace app{
namespace calcul{

	/// @brief 
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


		/// @brief 
		/// @param borderCode 
		/// @param verbose 
		static void Compute(
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
		ign::feature::sql::FeatureStorePostgis*                  _fsAreaInitCleaned;
		//--
		epg::log::EpgLogger*                                     _logger;
		//--
		epg::log::ShapeLogger*                                   _shapeLogger;
		//--
		bool                                                     _verbose;
		//--
		ome2::calcul::utils::AttributeMerger                     _attrMergerOnBorder;
		//--
		double													 _thresholdAreaAttr;

	private:

		//--
		void _init();

		//--
		void _compute() const;

		//--
		double _getAreaMergedByCountry(
			ign::geometry::MultiPolygon& geomAreaMerged,
			ign::feature::FeatureFilter& filterArroundAreaFromCountry,
			ign::feature::Feature& fMergedInit
		) const;

    };

}
}

#endif