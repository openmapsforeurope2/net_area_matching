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
#include <epg/tools/geometry/getArea.h>


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

			std::string listAttrWName = themeParameters->getValue(LIST_ATTR_W).toString();
			std::string listAttrJsonName = themeParameters->getValue(LIST_ATTR_JSON).toString();
			_attrMergerOnBorder.setLists(listAttrWName, listAttrJsonName, "/");

			_thresholdAreaAttr = themeParameters->getValue(THRESHOLD_AREA_ATTR).toDouble();
        }


        ///
        ///
        ///
        void SetAttributeMergedAreasOp::_compute() const {

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const idName = epgParams.getValue(ID).toString();
			//--
			app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
			std::string const wTagName = themeParameters->getParameter(W_TAG).getValue().toString();
			std::string separator = "#";

            ign::feature::FeatureFilter filterArea(countryCodeName+" like '%#%'");

            int numFeatures = epg::sql::tools::numFeatures(*_fsArea, filterArea);
            boost::progress_display display(numFeatures, std::cout, "[ set attribute merged areas % complete ]\n");

            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);
            while (itArea->hasNext())
            {
                ++display;

                ign::feature::Feature fArea = itArea->next();
                std::string idOrigin = fArea.getId();
				//boucler sur les mp ?
				ign::geometry::MultiPolygon geomArea = fArea.getGeometry().asMultiPolygon();

				ign::feature::Feature featCountry1, featCountry2;

				ign::feature::FeatureFilter filterArroundAreaFromCountry1(countryCodeName + "='" + _vCountry[0] + "'");
				filterArroundAreaFromCountry1.setExtent(geomArea.getEnvelope());

				double area1 = _getAreaMergedByCountry(geomArea, filterArroundAreaFromCountry1, featCountry1);

				ign::feature::FeatureFilter filterArroundAreaFromCountry2(countryCodeName + "='"+ _vCountry[1] +"'");
				filterArroundAreaFromCountry2.setExtent(geomArea.getEnvelope());

				double area2 = _getAreaMergedByCountry(geomArea, filterArroundAreaFromCountry2, featCountry2);

				if ( area1 < 0 && area2 < 0 ) {
					//pas d'attribut trouve
					fArea.setAttribute(wTagName, ign::data::String("modif_attr"));
					_fsArea->modifyFeature(fArea);
					continue;
				}
				else if ( area2 < 0.1*area1 ) {
					fArea = featCountry1;
				}
				else if ( area1 < 0.1*area2 ) {
					fArea = featCountry2;
				}
				else {
					fArea = featCountry1;
					_attrMergerOnBorder.mergeFeatAttribute(fArea,featCountry2, separator);
				}
				fArea.setId(idOrigin);
				fArea.setGeometry(geomArea);
				fArea.setAttribute(wTagName, ign::data::String("modif_attr")); 
				fArea.setAttribute("xy_source", ign::data::String("ome2")); 
				fArea.setAttribute("z_source", ign::data::String("ome2"));
				_fsArea->modifyFeature(fArea);
            }
        }


		double SetAttributeMergedAreasOp::_getAreaMergedByCountry(
			ign::geometry::MultiPolygon& geomAreaMerged,
			ign::feature::FeatureFilter& filterArroundAreaFromCountry,
			ign::feature::Feature& fMergedInit
		) const {

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

				ign::geometry::GeometryPtr geomIntersectedArea(geomAreaInit.Intersection(geomAreaMerged));
				double areaIntersected = epg::tools::geometry::getArea(*geomIntersectedArea);

				if ( areaIntersected == 0 )
					continue;
				
				mIntersectedArea[areaIntersected] = fAreaInit;
			}

			if (mIntersectedArea.size() == 0) {
				fMergedInit.clear();
				return -1;
			}
			else {
				fMergedInit = mIntersectedArea.rbegin()->second;
				return mIntersectedArea.rbegin()->first;
			}
		}
    }
}