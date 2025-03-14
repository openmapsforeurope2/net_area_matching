#ifndef _APP_CALCUL_POLYGONMERGEROP_H_
#define _APP_CALCUL_POLYGONMERGEROP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>
#include <ign/tools/stringtools.h>

//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>


namespace app{
namespace calcul{

	/// @brief 
	class PolygonMergerOp {

	public:

	
        /// @brief 
        /// @param verbose 
        PolygonMergerOp(
            bool verbose
        );

        /// @brief 
        ~PolygonMergerOp();


		/// \brief
		static void Compute(
			bool verbose
		);


	private:
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsArea;
		//--
		epg::log::EpgLogger*                                     _logger;
		//--
		epg::log::ShapeLogger*                                   _shapeLogger;
		//--
		bool                                                     _verbose;

	private:

		//--
		void _init();

        //--
        std::list<std::string> _getAllIdentifiers( std::string const& tableName ) const;

		//--
		void _compute() const;

    };

}
}

#endif