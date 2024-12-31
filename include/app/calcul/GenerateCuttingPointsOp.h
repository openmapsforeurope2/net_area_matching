#ifndef _APP_CALCUL_GENERATECUTTINGPOINTSOP_H_
#define _APP_CALCUL_GENERATECUTTINGPOINTSOP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>

//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>



namespace app{
namespace calcul{

	class GenerateCuttingPointsOp {

	public:

	
        /// @brief 
        /// @param borderCode 
        /// @param verbose 
		GenerateCuttingPointsOp(
            std::string const& borderCode,
            bool verbose,
			bool resetCpTable = true
        );

        /// @brief 
        ~GenerateCuttingPointsOp();


		/// \brief
		static void computeByCountry(
			std::string const& borderCode,
            bool verbose,
			bool resetCpTable = true
		);

		/// \brief
		static void compute(
            bool verbose,
			bool resetCpTable = true
		);


	private:
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsArea;
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsCutP;
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsCutL;
		//--
		epg::log::EpgLogger*                                     _logger;
		//--
		epg::log::ShapeLogger*                                   _shapeLogger;
		//--
		std::string                                              _borderCode;
		//--
		bool                                                     _verbose;

	private:

		//--
		void _init(bool resetCpTable);

		//--
		void _computeByCountry() const;

		//--
		void _generateCutp(
			ign::feature::FeatureFilter filter = ign::feature::FeatureFilter()
		) const;

		//--
		std::vector<std::pair<ign::geometry::Point, ign::math::Vec2d>> _getEndingVectors(
			ign::geometry::Polygon const& poly
		) const;

		//--
		ign::math::Vec2d _getLsEndingVector(
			ign::geometry::LineString const& ls,
			ign::geometry::Point const& endingPoint
		) const;

		//--
		double _getAngle(
			ign::math::Vec2d const& vRef,
			ign::math::Vec2d const& v
		) const;

		//--
		bool _hasCutLArroundEndingPt(	
			ign::feature::FeatureFilter const& filterArroundEndPt,
			ign::geometry::Point const& ptEndPt,
			ign::geometry::Polygon const& polyArea
		) const;

    };

}
}

#endif