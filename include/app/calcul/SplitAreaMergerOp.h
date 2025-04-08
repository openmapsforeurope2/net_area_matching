#ifndef _APP_CALCUL_SPLITAREAMERGEROP_H_
#define _APP_CALCUL_SPLITAREAMERGEROP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>


//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>
#include <epg/tools/MultiLineStringTool.h>


namespace app{
namespace calcul{

	/// @brief Classe utilisée pour fusionner les découpes superflues
	/// réalisées par l'opérateur CfSplitterOp.
	class SplitAreaMergerOp {

	public:
	
		/// @brief Constructeur
		/// @param verbose Mode verbeux
		SplitAreaMergerOp(
            bool verbose
        );

        /// @brief Destructeur
        ~SplitAreaMergerOp();

		/// @brief Lance la fusion. Les surfaces traitées sont celles dont l'attribut w_tag 
		/// n'est pas NULL. Ce champs a été renseigné par l'opérateur SetAttributeMergedAreasOp
		/// pour tracer les surfaces issues de la découpe réalisée par l'opérateur CfSplitterOp
		/// (car suite à l'affectation des attributs par SetAttributeMergedAreasOp le code pays 
		/// des surfaces issues de la découpe n'est pas forcément en '#').
		/// Les petites surfaces (surfaces dont l'aire est inférieur à un seuil) 
		/// sont fusionnés à la surface voisine de même identifiant national ayant
		/// la plus grande aire, sinon à la surface voisine ayant la plus grande aire.
		/// Les surfaces de même identifiant national sont fusionnées.
		/// @param verbose Mode verbeux
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
		void _compute() const;

        //--
        bool _computeEngine( bool mergeByNatId = true ) const;

		//--
		void _mergeGroups(std::vector<std::map<std::string, ign::feature::Feature>> & vmAreas) const;

		//--
		bool _addAreas(
            ign::feature::Feature const& feat1,
            ign::feature::Feature const& feat2,
            std::vector<std::map<std::string, ign::feature::Feature>> & vmAreas
        ) const;

		/// @brief Recherche les objets intersectant l'objet fourni en paramètre
		/// @param fArea Objet source dont on recherche les voisins
		/// @param filterArea_ Filtre sur les voisins recherchés
		/// @return Liste des voisins ordonnés selon leur aire
		std::map<double, ign::feature::Feature> _getNeighboursWithArea(
            ign::feature::Feature const& fArea,
            ign::feature::FeatureFilter const& filterArea_
        ) const;

		//--
		std::vector<ign::feature::Feature> _getNeighbours(
			ign::feature::Feature const& fArea,
            ign::feature::FeatureFilter const& filterArea_
		) const;

		//--
		double _getLength(
	        ign::geometry::Polygon const& poly
        ) const;

		//--
		double _getLength(
	        ign::geometry::MultiPolygon const& mp
        ) const;

    };
}
}

#endif