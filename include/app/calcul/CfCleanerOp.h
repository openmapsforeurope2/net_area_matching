#ifndef _APP_CALCUL_CFCLEANEROP_H_
#define _APP_CALCUL_CFCLEANEROP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>


//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>


namespace app{
namespace calcul{

	class CfCleanerOp {

	public:

	
		/// @brief 
		/// @param verbose 
		CfCleanerOp(
            bool verbose
        );

        /// @brief 
        ~CfCleanerOp();

        /// \brief
		static void compute(
			bool verbose
		);

    private:
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsArea;
        //--
		ign::feature::sql::FeatureStorePostgis*                  _fsCp;
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsCl;
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
		void _compute() const;
    };
}
}

#endif