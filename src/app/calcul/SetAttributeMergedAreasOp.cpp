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
			std::string areaTableNameInitCleaned = themeParameters->getParameter(AREA_TABLE_INIT_CLEANED).getValue().toString();
			_fsAreaInitCleaned = context->getDataBaseManager().getFeatureStore(areaTableNameInitCleaned, idName, geomName);


            //--
            _logger->log(epg::log::INFO, "[END] initialization: " + epg::tools::TimeTools::getTime());

            //--
		    epg::tools::StringTools::Split(_borderCode, "#", _vCountry);

			std::string listAttr2concatName = themeParameters->getValue(LIST_ATTR_TO_CONCAT).toString();
			std::string listAttrWName = themeParameters->getValue(LIST_ATTR_W).toString();
			std::string listAttrJsonName = themeParameters->getValue(LIST_ATTR_JSON).toString();
			_attrMergerOnBorder.setLists(listAttr2concatName, listAttrWName, listAttrJsonName, "/");

			_thresholdAreaAttr = themeParameters->getValue(THRESHOLD_AREA_ATTR).toDouble();
        }


        ///
        ///
        ///
        void SetAttributeMergedAreasOp::_compute() {

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const idName = epgParams.getValue(ID).toString();
			//--
			app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
			std::string const wTagName = themeParameters->getParameter(W_TAG).getValue().toString();

            ign::feature::FeatureFilter filterArea(countryCodeName+" like '%#%'");

			epg::ContextS::getInstance()->getDataBaseManager().setValueColumn(_fsArea->getTableName(), wTagName, "modif_attr", countryCodeName + " like '%#%'");

            int numFeatures = epg::sql::tools::numFeatures(*_fsArea, filterArea);
            boost::progress_display display(numFeatures, std::cout, "[ set attribute merged areas % complete ]\n");

            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);

			std::vector < ign::feature::Feature> vArea2modify;

            while (itArea->hasNext())
            {
                ++display;

                ign::feature::Feature fArea = itArea->next();
                //ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
                std::string idOrigin = fArea.getId();
				//boucler sur les mp ?
				ign::geometry::MultiPolygon geomArea = fArea.getGeometry().asMultiPolygon();

				ign::feature::Feature featCountry1, featCountry2;

				ign::feature::FeatureFilter filterArroundAreaFromCountry1(countryCodeName + "='" + _vCountry[0] + "'");
				filterArroundAreaFromCountry1.setExtent(geomArea.getEnvelope());

				bool hasAttr1 = _getAreaMergedByCountry(geomArea,filterArroundAreaFromCountry1, featCountry1);

				ign::feature::FeatureFilter filterArroundAreaFromCountry2(countryCodeName + "='"+ _vCountry[1] +"'");
				filterArroundAreaFromCountry2.setExtent(geomArea.getEnvelope());

				bool hasAttr2 = _getAreaMergedByCountry(geomArea,filterArroundAreaFromCountry2, featCountry2);

				if (!hasAttr1 && !hasAttr2) {
					//pas d'attribut trouve
					continue;
				}
				else if (hasAttr1 && !hasAttr2) {
					fArea = featCountry1;
				}
				else if (hasAttr2 && !hasAttr1) {
					fArea = featCountry2;
				}
				else {
					fArea = featCountry1;
					_attrMergerOnBorder.addFeatAttributeMerger(fArea,featCountry2,"#");
				}
				fArea.setId(idOrigin);
				fArea.setGeometry(geomArea);
				fArea.setGeometry(geomArea);
				//fArea.setAttribute(wTagName, ign::data::String("modif_attr")); 
				fArea.setAttribute("xy_source", ign::data::String("ome2")); 
				fArea.setAttribute("z_source", ign::data::String("ome2"));
				vArea2modify.push_back(fArea);				
            }
			for( size_t i = 0; i < vArea2modify.size(); ++i)
				_fsArea->modifyFeature(vArea2modify[i]);
        }


		bool SetAttributeMergedAreasOp::_getAreaMergedByCountry(ign::geometry::MultiPolygon& geomAreaMerged, ign::feature::FeatureFilter& filterArroundAreaFromCountry, ign::feature::Feature& fMergedInit)
		{

			std::map<double, ign::feature::Feature> mIntersectedArea;
			//recup fs table source -> table init sans step
			//filtre sur les feat de la table source
			ign::feature::FeatureIteratorPtr itAreaInit = _fsAreaInitCleaned->getFeatures(filterArroundAreaFromCountry);

			while (itAreaInit->hasNext())
			{

				ign::feature::Feature const& fAreaInit = itAreaInit->next();
				ign::geometry::MultiPolygon geomAreaInit = fAreaInit.getGeometry().asMultiPolygon();
				std::string idOriginInit = fAreaInit.getId();

				//si dist >0 continue sinon on stocke dans une map, id, feat
				if (geomAreaInit.distance(geomAreaMerged) > 0)
					continue;

				ign::geometry::Geometry* geomIntersectedArea = geomAreaInit.Intersection(geomAreaMerged);
				double areaIntersected = 0;
				if (geomIntersectedArea->isMultiPolygon())
					areaIntersected = geomIntersectedArea->asMultiPolygon().area();
				else if (geomIntersectedArea->isPolygon())
					areaIntersected = geomIntersectedArea->asPolygon().area();
				else if (geomIntersectedArea->isGeometryCollection()) {
					ign::geometry::GeometryCollection collection = geomIntersectedArea->asGeometryCollection();
					for (size_t i = 0; i < collection.numGeometries(); ++i) {
						if (collection.geometryN(i).isMultiPolygon())
							areaIntersected = collection.geometryN(i).asMultiPolygon().area();
						else if (collection.geometryN(i).isPolygon())
							areaIntersected = collection.geometryN(i).asPolygon().area();
					}
				}

				if (areaIntersected < _thresholdAreaAttr )
					continue;
				
				mIntersectedArea[areaIntersected] = fAreaInit;

			}

			if (mIntersectedArea.size() == 0) {
				fMergedInit.clear();
				return false;
			}
			else {
				fMergedInit = mIntersectedArea.rbegin()->second;
				return true;
			}

		}

    }
}