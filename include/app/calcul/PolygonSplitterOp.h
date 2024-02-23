#ifndef _APP_CALCUL_POLYGONSPLITTEROP_H_
#define _APP_CALCUL_POLYGONSPLITTEROP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>
#include <ign/tools/stringtools.h>

//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>
#include <epg/tools/geometry/SegmentIndexedGeometry.h>


namespace app{
namespace calcul{

	class PolygonSplitterOp {

	public:

	
        /// @brief 
        /// @param borderCode 
        /// @param verbose 
        PolygonSplitterOp(
            std::string borderCode,
            bool verbose
        );

        /// @brief 
        ~PolygonSplitterOp();


		/// \brief
		static void compute(
			std::string borderCode, 
			bool verbose
		);


	private:
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsArea;
		//--
		std::map<std::string, epg::tools::geometry::SegmentIndexedGeometryInterface*>    _mCountryCuttingIndx;
		//--
		std::map<std::string, ign::geometry::MultiLineString*>   _mCountryCuttingGeom;
		//--
		epg::log::EpgLogger*                                     _logger;
		//--
		epg::log::ShapeLogger*                                   _shapeLogger;
		//--
		std::string                                              _countryCode;
		//--
		bool                                                     _verbose;

	private:

		//--
		void _init();

        //--
        void _addLs(ign::geometry::Geometry const& geom, ign::geometry::MultiLineString & mls) const;

		//--
		void _compute() const;

    };

}
}

#endif