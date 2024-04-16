// APP
#include <app/calcul/SetAttributeMergedAreasOp.h>
#include <app/params/ThemeParameters.h>
#include <app/tools/zTools.h>

// BOOST
#include <boost/progress.hpp>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <epg/sql/tools/numFeatures.h>
#include <epg/sql/DataBaseManager.h>
#include <epg/tools/StringTools.h>
#include <epg/tools/TimeTools.h>
#include <epg/tools/FilterTools.h>


namespace app
{
    namespace calcul
    {
        ///
        ///
        ///
        SetAttributeMergedAreasOp::SetAttributeMergedAreasOp(
            std::string borderCode,
            bool verbose
        ) : 
            _verbose(verbose),
            _borderCode(borderCode)
        {
            _init();
        }

        ///
        ///
        ///
        SetAttributeMergedAreasOp::~SetAttributeMergedAreasOp()
        {
        }

        ///
        ///
        ///
        void SetAttributeMergedAreasOp::compute(
            std::string borderCode,
			bool verbose
		) {
            SetAttributeMergedAreasOp setAttributeMergedAreasOp(borderCode, verbose);
			setAttributeMergedAreasOp._compute();
        }

        ///
        ///
        ///
        void SetAttributeMergedAreasOp::_init()
        {
            //--
            _logger = epg::log::EpgLoggerS::getInstance();
            _logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const areaTableName = epgParams.getValue(AREA_TABLE).toString();
            std::string const idName = epgParams.getValue(ID).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            //--
            _fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);

			//--
			app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
			std::string areaTableNameInit = themeParameters->getParameter(AREA_TABLE_INIT).getValue().toString();
			_fsAreaInit = context->getDataBaseManager().getFeatureStore(areaTableNameInit, idName, geomName);


            //--
            _logger->log(epg::log::INFO, "[END] initialization: " + epg::tools::TimeTools::getTime());

            //--
		    epg::tools::StringTools::Split(_borderCode, "#", _vCountry);

			std::string listAttr2concatName = themeParameters->getValue(LIST_ATTR_TO_CONCAT).toString();
			std::string listAttrWName = themeParameters->getValue(LIST_ATTR_W).toString();
			std::string listAttrJsonName = themeParameters->getValue(LIST_ATTR_JSON).toString();
			ome2::calcul::utils::AttributeMerger* _attrMergerOnBorder = new ome2::calcul::utils::AttributeMerger(listAttr2concatName, listAttrWName, listAttrJsonName, "/");


        }


        ///
        ///
        ///
        void SetAttributeMergedAreasOp::_compute() {

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const idName = epgParams.getValue(ID).toString();

            ign::feature::FeatureFilter filterArea(countryCodeName+"='%#%'");

            int numFeatures = epg::sql::tools::numFeatures(*_fsArea, filterArea);
            boost::progress_display display(numFeatures, std::cout, "[ set attribute merged areas % complete ]\n");

            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);

            std::list<std::set<std::string>> lsAreas2Merge;
            std::set<std::string> sTreatedArea;
            while (itArea->hasNext())
            {
                ++display;

                ign::feature::Feature const& fArea = itArea->next();
                ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
                std::string idOrigin = fArea.getId();
				//boucler sur les mp ?
				ign::geometry::Polygon geomArea = fArea.getGeometry().asPolygon();

				ign::feature::Feature featCountry1, featCountry2;

				ign::feature::FeatureFilter filterArroundAreaFromCountry1(countryCodeName + "='" + _vCountry[0] + "'");
				filterArroundAreaFromCountry1.setExtent(geomArea.getEnvelope());

				bool hasAttr1 = _getAreaMergedByCountry(geomArea,filterArroundAreaFromCountry1, featCountry1);

				ign::feature::FeatureFilter filterArroundAreaFromCountry2(countryCodeName + "='"+ _vCountry[1] +"'");
				filterArroundAreaFromCountry2.setExtent(geomArea.getEnvelope());

				bool hasAttr2 = _getAreaMergedByCountry(geomArea,filterArroundAreaFromCountry2, featCountry2);

				if (!hasAttr1 && !hasAttr2) {
					//pas d'attribut trouvé
					continue;
				}

				//si un seul des deux alors prendre celui là et suppr # dans code country et sinon on fusionne

				//boucle sur liste des attr
				//cf ce qui est fait dans tn

            }

        }



		bool SetAttributeMergedAreasOp::_getAreaMergedByCountry(ign::geometry::Polygon& geomAreaMerged, ign::feature::FeatureFilter& filterArroundAreaFromCountry, ign::feature::Feature& fMergedInit)
		{

			std::vector<ign::feature::Feature> vIntersectedArea;
			//recup fs table source -> table init sans step
			//filtre sur les feat de la table source
			ign::feature::FeatureIteratorPtr itAreaInit = _fsAreaInit->getFeatures(filterArroundAreaFromCountry);

			while (itAreaInit->hasNext())
			{

				ign::feature::Feature const& fAreaInit = itAreaInit->next();
				ign::geometry::MultiPolygon const& mp = fAreaInit.getGeometry().asMultiPolygon();
				std::string idOriginInit = fAreaInit.getId();
				//boucler sur les mp ?
				ign::geometry::Polygon geomAreaInit = fAreaInit.getGeometry().asPolygon();

				//si dist >0 continue sinon on stocke dans une map, id, feat
				if (geomAreaInit.distance(geomAreaMerged) > 0)
					continue;
				vIntersectedArea.push_back(fAreaInit);

			}

			//si un seul obj on le prend, sinon on regarde la plus gde surf d'intersection
			if (vIntersectedArea.size() == 1) {
				fMergedInit = vIntersectedArea[0];
				return true;
			}
			else if (vIntersectedArea.size() == 0) {
				fMergedInit.clear();
				return false;
			}

			double maxAreaIntersected = 0;
			for (size_t i = 0; i < vIntersectedArea.size(); ++i) {
				ign::geometry::Geometry* geomIntersectedArea = vIntersectedArea[i].getGeometry().Intersection(geomAreaMerged);
				double areaIntersected;
				if (geomIntersectedArea->isMultiPolygon())
					areaIntersected = geomIntersectedArea->asMultiPolygon().area();
				else if (geomIntersectedArea->isPolygon())
					areaIntersected = geomIntersectedArea->asPolygon().area();
				if (areaIntersected > maxAreaIntersected) {
					maxAreaIntersected = areaIntersected;
					fMergedInit = vIntersectedArea[i];
				}
			}
			return true;
		}



    }
}