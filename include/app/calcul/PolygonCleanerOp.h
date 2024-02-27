#ifndef _APP_CALCUL_POLYGONCLEANEROP_H_
#define _APP_CALCUL_POLYGONCLEANEROP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>
#include <ign/tools/stringtools.h>

//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>
#include <epg/tools/MultiLineStringTool.h>


namespace app{
namespace calcul{

	class PolygonCleanerOp {

	public:

	
        /// @brief 
        /// @param borderCode 
        /// @param verbose 
        PolygonCleanerOp(
            std::string borderCode,
            bool verbose
        );

        /// @brief 
        ~PolygonCleanerOp();


		/// \brief
		static void compute(
			std::string borderCode, 
			bool verbose
		);


	private:
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsArea;
		//--
		epg::tools::MultiLineStringTool*                         _boundaryTool;
		//--
		std::map<std::string, ign::geometry::GeometryPtr>        _mCountryGeomPtr;
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
		void _init();

        //--
        void _addLs(ign::geometry::Geometry const& geom, ign::geometry::MultiLineString & mls) const;

		//--
		void _compute() const;

    };

}
}

#endif