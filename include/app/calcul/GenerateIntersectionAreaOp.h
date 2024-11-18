#ifndef _APP_CALCUL_GENERATEINTERSECTIONAREAOP_H_
#define _APP_CALCUL_GENERATEINTERSECTIONAREAOP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>

//EPG
#include <epg/log/EpgLogger.h>



namespace app{
namespace calcul{

	class GenerateIntersectionAreaOp {

	public:

	
        /// @brief 
        /// @param borderCode 
        /// @param verbose 
		GenerateIntersectionAreaOp(
            std::string borderCode,
            bool verbose
        );

        /// @brief 
        ~GenerateIntersectionAreaOp();


		/// \brief
		static void compute(
			std::string borderCode,
            bool verbose
		);


	private:
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsArea;
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsIntArea;
		//--
		epg::log::EpgLogger*                                     _logger;
		//--
		std::string                                              _borderCode;
		//--
		std::vector<std::string>                                 _vCountry;
		//--
		bool                                                     _verbose;

	private:

		//--
		void _init();

		//--
		void _compute() const;

		//--
		ign::geometry::Geometry* _getIntersectingAreas(
			ign::geometry::Geometry const& geom,
			size_t countryIndex
		) const;

		//--
		void _persistGeom( ign::geometry::Geometry const& geom ) const;
    };

}
}

#endif