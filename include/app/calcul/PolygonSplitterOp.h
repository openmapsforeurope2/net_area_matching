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

	/// @brief Classe utilisée pour découper ('clipper') les surfaces par intersection 
	/// avec la surface du pays augmentée d'un buffer (dont la distance est paramétrée).
	/// A utiliser conjointement avec les classes PolygonMergerOp et PolygonCleanerOp
	class PolygonSplitterOp {

	public:

	
        /// @brief Constructeur
        /// @param borderCode Code frontère (code double)
        /// @param verbose Mode verbeux
        PolygonSplitterOp(
            std::string borderCode,
            bool verbose
        );

        /// @brief Destructeur
        ~PolygonSplitterOp();


		/// @brief Lance la découpe des surfaces
		/// @param borderCode Code frontère (code double)
        /// @param verbose Mode verbeux
		static void Compute(
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